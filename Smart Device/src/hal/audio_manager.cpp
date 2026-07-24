/**
 * @file audio_manager.cpp
 * @brief Local I2S audio implementation.
 */

#include "hal/audio_manager.h"
#include "pins_config.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace {
constexpr uint32_t kSampleRate = AUDIO_SAMPLE_RATE;
constexpr size_t kDmaBufLen = AUDIO_DMA_BUF_LEN;
constexpr int kDmaBufCount = AUDIO_DMA_BUF_COUNT;
constexpr uint32_t kTaskStackSz = 4096;
constexpr UBaseType_t kTaskPrio = 5;
}  // namespace

AudioManager::AudioManager()
    : initialized_(false),
      passthrough_active_(false),
      task_handle_(nullptr),
      peak_level_(0),
      stream_active_(false),
      stream_task_handle_(nullptr) {}

bool AudioManager::init() {
    if (initialized_) return true;

    gpio_set_direction((gpio_num_t)I2S_SPK_SD_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);

    const i2s_config_t spkCfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = kSampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = kDmaBufCount,
        .dma_buf_len = (int)kDmaBufLen,
        .use_apll = false,
        .tx_desc_auto_clear = true,
    };
    if (i2s_driver_install(I2S_SPK_PORT, &spkCfg, 0, nullptr) != ESP_OK) return false;

    const i2s_pin_config_t spkPins = {
        .bck_io_num = I2S_SPK_BCLK_PIN,
        .ws_io_num = I2S_SPK_LRCLK_PIN,
        .data_out_num = I2S_SPK_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
    if (i2s_set_pin(I2S_SPK_PORT, &spkPins) != ESP_OK) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    const i2s_config_t micCfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = kSampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = kDmaBufCount,
        .dma_buf_len = (int)kDmaBufLen,
        .use_apll = false,
        .tx_desc_auto_clear = false,
    };
    if (i2s_driver_install(I2S_MIC_PORT, &micCfg, 0, nullptr) != ESP_OK) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    const i2s_pin_config_t micPins = {
        .bck_io_num = I2S_MIC_SCK_PIN,
        .ws_io_num = I2S_MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD_PIN,
    };
    if (i2s_set_pin(I2S_MIC_PORT, &micPins) != ESP_OK) {
        i2s_driver_uninstall(I2S_MIC_PORT);
        i2s_driver_uninstall(I2S_SPK_PORT);
        return false;
    }

    initialized_ = true;
    return true;
}

void AudioManager::deinit() {
    stopStream();
    stopPassthrough();
    if (initialized_) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        initialized_ = false;
    }
}

bool AudioManager::startStream(const std::string& url) {
    if (url.empty()) return false;

    stopStream();
    stopPassthrough();

    if (!initialized_) return false;

    if (WiFi.status() != WL_CONNECTED) return false;

    stream_url_ = url + (url.find('?') == std::string::npos ? "?format=pcm" : "&format=pcm");
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 1);
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    stream_active_ = true;
    if (xTaskCreate(AudioManager::streamTask, "media_pcm", kTaskStackSz,
                    this, kTaskPrio, &stream_task_handle_) != pdPASS) {
        stream_active_ = false;
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        return false;
    }
    return true;
}

void AudioManager::stopStream() {
    stream_active_ = false;
    if (stream_task_handle_ != nullptr) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (stream_task_handle_ != nullptr) {
            vTaskDelete(stream_task_handle_);
            stream_task_handle_ = nullptr;
        }
    }
    if (initialized_) i2s_zero_dma_buffer(I2S_SPK_PORT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
}

void AudioManager::update() {
    // PCM transfer and I2S output run in runStreamLoop().
}

bool AudioManager::isStreaming() const { return stream_active_; }

void AudioManager::streamTask(void* arg) {
    static_cast<AudioManager*>(arg)->runStreamLoop();
}

void AudioManager::runStreamLoop() {
    HTTPClient http;
    http.setTimeout(1000);
    http.setReuse(false);
    if (!http.begin(stream_url_.c_str())) {
        Serial.println("[Audio] PCM HTTP begin failed");
        stream_active_ = false;
        stream_task_handle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        Serial.printf("[Audio] PCM stream HTTP %d\n", status);
        http.end();
        stream_active_ = false;
        stream_task_handle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    WiFiClient* input = http.getStreamPtr();
    uint8_t buffer[1024];
    while (stream_active_ && (http.connected() || input->available())) {
        const size_t available = input->available();
        if (available == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        const size_t bytesToRead = available < sizeof(buffer) ? available : sizeof(buffer);
        const size_t bytesRead = input->readBytes(buffer, bytesToRead);
        if (bytesRead == 0) continue;
        size_t bytesWritten = 0;
        i2s_write(I2S_SPK_PORT, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
    }
    http.end();
    stream_active_ = false;
    stream_task_handle_ = nullptr;
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
    vTaskDelete(nullptr);
}

void AudioManager::startPassthrough() {
    if (!initialized_ || passthrough_active_) return;
    stopStream();
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 1);
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    i2s_zero_dma_buffer(I2S_MIC_PORT);
    passthrough_active_ = true;
    peak_level_ = 0;
    xTaskCreate(AudioManager::audioTask, "audio_pt", kTaskStackSz, this,
                kTaskPrio, &task_handle_);
}

void AudioManager::stopPassthrough() {
    if (!passthrough_active_) return;
    passthrough_active_ = false;
    vTaskDelay(pdMS_TO_TICKS(40));
    if (task_handle_ != nullptr) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    if (initialized_) i2s_zero_dma_buffer(I2S_SPK_PORT);
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
    peak_level_ = 0;
}

bool AudioManager::isActive() const { return passthrough_active_; }
uint16_t AudioManager::getPeakLevel() const { return peak_level_; }

void AudioManager::audioTask(void* arg) { static_cast<AudioManager*>(arg)->runAudioLoop(); }

void AudioManager::runAudioLoop() {
    int32_t readBuf[kDmaBufLen];
    int16_t writeBuf[kDmaBufLen];
    while (passthrough_active_) {
        size_t bytesRead = 0;
        if (i2s_read(I2S_MIC_PORT, readBuf, sizeof(readBuf), &bytesRead,
                     pdMS_TO_TICKS(100)) != ESP_OK || bytesRead == 0) continue;
        const size_t samples = bytesRead / sizeof(int32_t);
        int32_t peak = 0;
        for (size_t i = 0; i < samples; ++i) {
            writeBuf[i] = static_cast<int16_t>(readBuf[i] >> 16);
            const int32_t magnitude = writeBuf[i] < 0 ? -static_cast<int32_t>(writeBuf[i])
                                                       : static_cast<int32_t>(writeBuf[i]);
            if (magnitude > peak) peak = magnitude;
        }
        peak_level_ = static_cast<uint16_t>((peak * 100L) / 32767L);
        size_t written = 0;
        i2s_write(I2S_SPK_PORT, writeBuf, samples * sizeof(int16_t), &written,
                  pdMS_TO_TICKS(100));
    }
    task_handle_ = nullptr;
    vTaskDelete(nullptr);
}
