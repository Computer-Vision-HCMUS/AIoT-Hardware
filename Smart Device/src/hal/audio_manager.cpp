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
#include <Audio.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <cstring>

// ---------------------------------------------------------------------------
// Private constants
// ---------------------------------------------------------------------------
static constexpr uint32_t kSampleRate   = AUDIO_SAMPLE_RATE;
static constexpr size_t   kDmaBufLen    = AUDIO_DMA_BUF_LEN;
static constexpr int      kDmaBufCount  = AUDIO_DMA_BUF_COUNT;
static constexpr uint32_t kTaskStackSz  = 4096;
static constexpr UBaseType_t kTaskPrio  = 5;  // High priority for audio
static constexpr char kMediaCachePath[] = "/media-cache.mp3";
static constexpr size_t kTransferBufferSize = 2048;
static constexpr size_t kMinPlayableBytes = 1024;
static constexpr size_t kCacheSafetyMargin = 4096;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AudioManager::AudioManager()
    : initialized_(false),
      passthrough_active_(false),
      task_handle_(nullptr),
      peak_level_(0),
      stream_audio_(nullptr),
      stream_active_(false) {}

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
    stopStream();
    if (initialized_) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        initialized_ = false;
    }
}

// ---------------------------------------------------------------------------
// Cached MP3 playback. Downloading completes before decoding begins, preventing
// Wi-Fi receive work from starving I2S and causing the audible stutter seen
// with direct HTTP decoding on the ESP32.
// ---------------------------------------------------------------------------
bool AudioManager::startStream(const std::string& url) {
    if (url.empty()) {
        Serial.println("[Audio] ERROR: media URL is empty");
        return false;
    }

    stopStream();
    stopPassthrough();

    if (initialized_) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        initialized_ = false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Audio] ERROR: WiFi unavailable");
        init();
        return false;
    }

    // This filesystem is used only as a disposable media cache. A failed mount
    // (for example after an interrupted flash write) is therefore safe to
    // repair by formatting it once before downloading the next track.
    if (!LittleFS.begin(false)) {
        Serial.println("[Audio] LittleFS mount failed; formatting media cache");
        if (!LittleFS.format() || !LittleFS.begin(false)) {
            Serial.println("[Audio] ERROR: LittleFS unavailable after format");
            init();
            return false;
        }
    }

    // FILE_WRITE truncates the previous cache, releasing its space before we
    // compute the maximum safe Range length for this download.
    File cache = LittleFS.open(kMediaCachePath, FILE_WRITE);
    if (!cache) {
        Serial.println("[Audio] ERROR: cannot create media cache");
        init();
        return false;
    }

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    if (!http.begin(url.c_str())) {
        Serial.printf("[Audio] ERROR: cannot open download URL: %s\n", url.c_str());
        cache.close();
        init();
        return false;
    }

    const size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (freeBytes <= kCacheSafetyMargin) {
        Serial.printf("[Audio] ERROR: media cache has only %u bytes free\n",
                      static_cast<unsigned>(freeBytes));
        cache.close();
        http.end();
        init();
        return false;
    }

    // Always use one Range request. It returns the full file when it fits and
    // a playable prefix otherwise; importantly, it avoids aborting a first
    // full-file response, which resets the ESP32's next TCP connection.
    const size_t maxCacheBytes = freeBytes - kCacheSafetyMargin;
    http.addHeader("Range", "bytes=0-" + String(maxCacheBytes - 1));
    const int status = http.GET();
    const int contentLength = http.getSize();
    const bool partialDownload = status == HTTP_CODE_PARTIAL_CONTENT &&
                                 contentLength >= static_cast<int>(maxCacheBytes);

    if (status != HTTP_CODE_OK && status != HTTP_CODE_PARTIAL_CONTENT) {
        Serial.printf("[Audio] ERROR: media range request HTTP %d\n", status);
        cache.close();
        http.end();
        init();
        return false;
    }
    if (contentLength > 0 && static_cast<size_t>(contentLength) > maxCacheBytes) {
        Serial.println("[Audio] ERROR: server ignored requested media range");
        cache.close();
        http.end();
        init();
        return false;
    }

    WiFiClient* input = http.getStreamPtr();
    uint8_t transfer[kTransferBufferSize];
    size_t downloaded = 0;
    int remaining = contentLength;
    while ((http.connected() || input->available()) &&
           (remaining > 0 || contentLength == -1)) {
        const size_t available = input->available();
        if (available == 0) {
            delay(1);
            continue;
        }
        const size_t toRead = available < sizeof(transfer) ? available : sizeof(transfer);
        const size_t read = input->readBytes(transfer, toRead);
        if (read == 0 || cache.write(transfer, read) != read) break;
        downloaded += read;
        if (contentLength > 0) remaining -= static_cast<int>(read);
    }
    cache.close();
    http.end();

    if (downloaded < kMinPlayableBytes || (contentLength > 0 && remaining > 0)) {
        Serial.printf("[Audio] ERROR: incomplete media download (%u bytes)\n",
                      static_cast<unsigned>(downloaded));
        LittleFS.remove(kMediaCachePath);
        init();
        return false;
    }

    stream_audio_ = new Audio();
    if (!stream_audio_) {
        Serial.println("[Audio] ERROR: unable to allocate stream decoder");
        init();
        return false;
    }

    stream_audio_->setPinout(I2S_SPK_BCLK_PIN, I2S_SPK_LRCLK_PIN, I2S_SPK_DOUT_PIN);
    stream_audio_->setVolume(12);  // safe default for the MAX98357 amplifier
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 1);

    if (!stream_audio_->connecttoFS(LittleFS, kMediaCachePath)) {
        Serial.println("[Audio] ERROR: cannot decode cached media file");
        delete stream_audio_;
        stream_audio_ = nullptr;
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        init();
        return false;
    }

    stream_active_ = true;
    Serial.printf("[Audio] playing cached media (%u bytes%s)\n",
                  static_cast<unsigned>(downloaded),
                  partialDownload ? ", prefix" : "");
    return true;
}

void AudioManager::stopStream() {
    if (stream_audio_) {
        stream_audio_->stopSong();
        delete stream_audio_;
        stream_audio_ = nullptr;
    }
    stream_active_ = false;
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);

    // The streaming library releases I2S0 with its object. Restore the normal
    // mic-test drivers only when they were previously released for a stream.
    if (!initialized_) init();
}

void AudioManager::update() {
    if (!stream_audio_ || !stream_active_) return;

    stream_audio_->loop();
    if (!stream_audio_->isRunning()) {
        stream_active_ = false;
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        Serial.println("[Audio] stream finished");
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
bool AudioManager::isStreaming() const { return stream_active_; }

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
