/**
 * @file audio_manager.cpp
 * @brief I2S audio passthrough implementation
 *
 * I2S peripheral assignments:
 *   I2S_NUM_0 (I2S_SPK_PORT) — TX master, 16-bit stereo frame, drives MAX98357
 *   I2S_NUM_1 (I2S_MIC_PORT) — RX master, 32-bit frame, reads INMP441
 *
 * INMP441 data format:
 *   32-bit I2S frame, MSB-justified. Bit 31 = MSB of 24-bit sample.
 *   Convert to 16-bit: sample16 = (int16_t)(sample32 >> 16)
 *
 * Latency: (AUDIO_DMA_BUF_LEN / AUDIO_SAMPLE_RATE) * AUDIO_DMA_BUF_COUNT
 *         = (512 / 16000) * 4 ≈ 128 ms worst case; typical ~32 ms per buffer.
 */

#include "hal/audio_manager.h"
#include "pins_config.h"

#include <driver/i2s.h>
#include <driver/gpio.h>
#include <Arduino.h>
#include <cstring>

// ---------------------------------------------------------------------------
// Private constants
// ---------------------------------------------------------------------------
static constexpr uint32_t kSampleRate   = AUDIO_SAMPLE_RATE;
static constexpr size_t   kDmaBufLen    = AUDIO_DMA_BUF_LEN;
static constexpr int      kDmaBufCount  = AUDIO_DMA_BUF_COUNT;
static constexpr uint32_t kTaskStackSz  = 4096;
static constexpr UBaseType_t kTaskPrio  = 5;  // High priority for audio

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AudioManager::AudioManager()
    : initialized_(false),
      passthrough_active_(false),
      task_handle_(nullptr),
      peak_level_(0) {}

// ---------------------------------------------------------------------------
// init() — install I2S drivers for both ports
// ---------------------------------------------------------------------------
bool AudioManager::init() {
    if (initialized_) return true;

    // ── SD_MODE pin: pull HIGH to enable MAX98357 ──
    gpio_set_direction((gpio_num_t)I2S_SPK_SD_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);  // start muted

    // ─────────────────────────────────────────────
    // Speaker (I2S_NUM_0) — TX, 16-bit
    // ─────────────────────────────────────────────
    const i2s_config_t spk_cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = kSampleRate,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = kDmaBufCount,
        .dma_buf_len          = (int)kDmaBufLen,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,  // silence on underflow
    };

    if (i2s_driver_install(I2S_SPK_PORT, &spk_cfg, 0, nullptr) != ESP_OK) {
        Serial.println("[Audio] ERROR: speaker i2s_driver_install failed");
        return false;
    }

    const i2s_pin_config_t spk_pins = {
        .bck_io_num   = I2S_SPK_BCLK_PIN,
        .ws_io_num    = I2S_SPK_LRCLK_PIN,
        .data_out_num = I2S_SPK_DOUT_PIN,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };

    if (i2s_set_pin(I2S_SPK_PORT, &spk_pins) != ESP_OK) {
        Serial.println("[Audio] ERROR: speaker i2s_set_pin failed");
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    // ─────────────────────────────────────────────
    // Microphone (I2S_NUM_1) — RX, 32-bit
    // ─────────────────────────────────────────────
    const i2s_config_t mic_cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = kSampleRate,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = kDmaBufCount,
        .dma_buf_len          = (int)kDmaBufLen,
        .use_apll             = false,
        .tx_desc_auto_clear   = false,
    };

    if (i2s_driver_install(I2S_MIC_PORT, &mic_cfg, 0, nullptr) != ESP_OK) {
        Serial.println("[Audio] ERROR: mic i2s_driver_install failed");
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    const i2s_pin_config_t mic_pins = {
        .bck_io_num   = I2S_MIC_SCK_PIN,
        .ws_io_num    = I2S_MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = I2S_MIC_SD_PIN,
    };

    if (i2s_set_pin(I2S_MIC_PORT, &mic_pins) != ESP_OK) {
        Serial.println("[Audio] ERROR: mic i2s_set_pin failed");
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        return false;
    }

    initialized_ = true;
    Serial.println("[Audio] init OK — SPK: I2S0, MIC: I2S1");
    return true;
}

// ---------------------------------------------------------------------------
// deinit() — uninstall drivers
// ---------------------------------------------------------------------------
void AudioManager::deinit() {
    stopPassthrough();
    if (initialized_) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        initialized_ = false;
    }
}

// ---------------------------------------------------------------------------
// startPassthrough() — enable amp and start FreeRTOS audio task
// ---------------------------------------------------------------------------
void AudioManager::startPassthrough() {
    if (!initialized_ || passthrough_active_) return;

    // Enable MAX98357 amplifier
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 1);

    // Clear any stale data in the DMA buffers
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    i2s_zero_dma_buffer(I2S_MIC_PORT);

    passthrough_active_ = true;
    peak_level_ = 0;

    xTaskCreate(
        AudioManager::audioTask,
        "audio_pt",
        kTaskStackSz,
        this,
        kTaskPrio,
        &task_handle_
    );

    Serial.println("[Audio] passthrough started");
}

// ---------------------------------------------------------------------------
// stopPassthrough() — signal task to stop, mute amp
// ---------------------------------------------------------------------------
void AudioManager::stopPassthrough() {
    if (!passthrough_active_) return;

    passthrough_active_ = false;

    // Give the task time to finish its current buffer iteration
    vTaskDelay(pdMS_TO_TICKS(150));

    // Force-delete if still running
    if (task_handle_ != nullptr) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }

    // Silence speaker DMA buffer then mute the amp
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);

    peak_level_ = 0;
    Serial.println("[Audio] passthrough stopped");
}

// ---------------------------------------------------------------------------
bool AudioManager::isActive()      const { return passthrough_active_; }
uint16_t AudioManager::getPeakLevel() const { return peak_level_; }

// ---------------------------------------------------------------------------
// audioTask — static trampoline
// ---------------------------------------------------------------------------
void AudioManager::audioTask(void* arg) {
    static_cast<AudioManager*>(arg)->runAudioLoop();
}

// ---------------------------------------------------------------------------
// runAudioLoop — main passthrough loop (runs in FreeRTOS task)
// ---------------------------------------------------------------------------
void AudioManager::runAudioLoop() {
    // Allocate buffers on the heap to avoid overflowing task stack
    int32_t* readBuf  = new int32_t[kDmaBufLen];
    int16_t* writeBuf = new int16_t[kDmaBufLen];

    if (!readBuf || !writeBuf) {
        Serial.println("[Audio] ERROR: buffer alloc failed");
        delete[] readBuf;
        delete[] writeBuf;
        task_handle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    const size_t kReadBytes  = kDmaBufLen * sizeof(int32_t);
    const size_t kWriteBytes = kDmaBufLen * sizeof(int16_t);

    while (passthrough_active_) {
        // ── Read from INMP441 (32-bit frames) ──
        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_MIC_PORT, readBuf, kReadBytes,
                                 &bytesRead, pdMS_TO_TICKS(100));

        if (err != ESP_OK || bytesRead == 0) {
            vTaskDelay(1);
            continue;
        }

        const size_t samples = bytesRead / sizeof(int32_t);
        int32_t peakAcc = 0;

        // ── Convert 32-bit INMP441 → 16-bit for MAX98357 ──
        // INMP441: 24-bit sample in upper 24 bits of 32-bit frame
        // Shift right 16 to get top 16 bits (discards 8 LSBs of 24-bit)
        for (size_t i = 0; i < samples; ++i) {
            const int16_t s = static_cast<int16_t>(readBuf[i] >> 16);
            writeBuf[i]     = s;
            const int32_t a = s < 0 ? -s : s;
            if (a > peakAcc) peakAcc = a;
        }

        // Update peak level 0–100 for VU meter
        uint16_t lvl = static_cast<uint16_t>((peakAcc * 100) / 32767);
        if (lvl > 100) lvl = 100;
        peak_level_ = lvl;

        // ── Write to MAX98357 (16-bit frames) ──
        size_t bytesWritten = 0;
        i2s_write(I2S_SPK_PORT, writeBuf, samples * sizeof(int16_t),
                  &bytesWritten, pdMS_TO_TICKS(100));
    }

    delete[] readBuf;
    delete[] writeBuf;

    task_handle_ = nullptr;
    vTaskDelete(nullptr);
}
