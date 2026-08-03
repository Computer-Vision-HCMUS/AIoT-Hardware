/**
 * @file audio_manager.cpp
 * @brief Local I2S audio implementation.
 */

#include "hal/audio_manager.h"
#include "pins_config.h"

#include <Arduino.h>
#include <cstring>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>

namespace {
constexpr uint32_t kSampleRate = AUDIO_SAMPLE_RATE;
constexpr size_t kDmaBufLen = AUDIO_DMA_BUF_LEN;
constexpr int kDmaBufCount = AUDIO_DMA_BUF_COUNT;
constexpr uint32_t kTaskStackSz = 4096;
// HTTPClient (especially HTTPS) needs headroom; PCM scratch is static, not on-stack.
constexpr uint32_t kStreamTaskStackSz = 12288;
constexpr uint32_t kRecordingTaskStackSz = 8192;
constexpr UBaseType_t kRecordingTaskPrio = 2;
constexpr UBaseType_t kTaskPrio = 5;
constexpr UBaseType_t kPlaybackTaskPrio = kTaskPrio + 1;
constexpr UBaseType_t kStreamTaskPrio = kTaskPrio;
constexpr char kCompanionRecordingPath[] = "/companion.pcm";
constexpr size_t kStreamBufferBytes = AUDIO_SAMPLE_RATE;  // ~0.5 s at 16-bit mono
constexpr size_t kPlaybackPrebufferBytes = kStreamBufferBytes / 2;         // ~250 ms
constexpr size_t kPlaybackChunkBytes = 2048;
constexpr size_t kPcmAccumBytes = 2048;
constexpr TickType_t kStreamSendWaitTicks = pdMS_TO_TICKS(20);
constexpr TickType_t kPlaybackReceiveWaitTicks = pdMS_TO_TICKS(20);
// Keep the recorder task stack small and predictable.  The I2S DMA buffers
// already absorb timing jitter; a 128-sample conversion chunk is enough.
constexpr size_t kRecordingChunkSamples = 128;
constexpr size_t kRecordingWriteBatchBytes = 2048;
// Keep below the board's SPIFFS capacity and the server's 30 s upload cap.
constexpr size_t kMaxCompanionRecordingBytes = 10 * AUDIO_SAMPLE_RATE * sizeof(int16_t);
}  // namespace

AudioManager::AudioManager()
    : initialized_(false),
      passthrough_active_(false),
      task_handle_(nullptr),
      peak_level_(0),
      stream_active_(false),
      stream_producer_done_(false),
      stream_task_handle_(nullptr),
      playback_task_handle_(nullptr),
      stream_buffer_(nullptr),
      recording_active_(false),
      recording_task_handle_(nullptr),
      recording_bytes_(0) {}

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
    stopRecording();
    stopPassthrough();
    if (stream_buffer_ != nullptr) {
        vStreamBufferDelete(stream_buffer_);
        stream_buffer_ = nullptr;
    }
    if (initialized_) {
        i2s_driver_uninstall(I2S_SPK_PORT);
        i2s_driver_uninstall(I2S_MIC_PORT);
        gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
        initialized_ = false;
    }
}

bool AudioManager::startStream(const std::string& url, const std::string& deviceToken) {
    if (url.empty()) return false;

    // Always tear down any prior producer/playback — even if stream_active_
    // is already false after a finished track.
    stopStream();
    stopPassthrough();

    if (!initialized_) return false;

    if (WiFi.status() != WL_CONNECTED) return false;

    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);

    // Reuse one ring buffer across plays to avoid heap fragmentation crashes.
    if (stream_buffer_ == nullptr) {
        stream_buffer_ = xStreamBufferCreate(kStreamBufferBytes, 1);
        if (stream_buffer_ == nullptr) {
            Serial.println("[Audio] PCM stream buffer create failed");
            return false;
        }
    } else {
        xStreamBufferReset(stream_buffer_);
    }

    stream_url_ = url + (url.find('?') == std::string::npos ? "?format=pcm" : "&format=pcm");
    stream_device_token_ = deviceToken;
    Serial.printf("[Audio] Starting PCM stream: %s\n", stream_url_.c_str());
    i2s_zero_dma_buffer(I2S_SPK_PORT);
    stream_active_ = true;
    stream_producer_done_ = false;

    if (xTaskCreatePinnedToCore(AudioManager::streamTask, "media_pcm", kStreamTaskStackSz,
                                 this, kStreamTaskPrio, &stream_task_handle_, APP_CPU_NUM) != pdPASS) {
        stream_active_ = false;
        return false;
    }
    if (xTaskCreatePinnedToCore(AudioManager::playbackTask, "media_pcm_pb", kTaskStackSz,
                                 this, kPlaybackTaskPrio, &playback_task_handle_, PRO_CPU_NUM) != pdPASS) {
        stream_active_ = false;
        stream_producer_done_ = true;
        for (uint8_t waited = 0; stream_task_handle_ != nullptr && waited < 100; ++waited)
            vTaskDelay(pdMS_TO_TICKS(10));
        if (stream_task_handle_ != nullptr) {
            vTaskDelete(stream_task_handle_);
            stream_task_handle_ = nullptr;
        }
        return false;
    }
    return true;
}

void AudioManager::stopStream() {
    stream_active_ = false;
    // Unblock playback if it is waiting on the prebuffer threshold.
    stream_producer_done_ = true;

    // Let producer finish http.end() and playback leave i2s_write — force
    // delete mid-call leaks sockets / wedges I2S and the next Play crashes.
    for (uint8_t waited = 0;
         (stream_task_handle_ != nullptr || playback_task_handle_ != nullptr) && waited < 250;
         ++waited) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    bool forceDeleted = false;
    if (stream_task_handle_ != nullptr) {
        Serial.println("[Audio] PCM producer stop timed out");
        vTaskDelete(stream_task_handle_);
        stream_task_handle_ = nullptr;
        forceDeleted = true;
    }
    if (playback_task_handle_ != nullptr) {
        Serial.println("[Audio] PCM playback stop timed out");
        vTaskDelete(playback_task_handle_);
        playback_task_handle_ = nullptr;
        forceDeleted = true;
    }

    if (stream_buffer_ != nullptr) {
        xStreamBufferReset(stream_buffer_);
    }

    if (forceDeleted && initialized_) {
        i2s_stop(I2S_SPK_PORT);
        i2s_zero_dma_buffer(I2S_SPK_PORT);
        i2s_start(I2S_SPK_PORT);
    } else if (initialized_) {
        i2s_zero_dma_buffer(I2S_SPK_PORT);
    }
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
    stream_producer_done_ = false;
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    WiFi.setSleep(true);
}

void AudioManager::update() {
    // PCM transfer and I2S output run in FreeRTOS tasks.
}

bool AudioManager::isStreaming() const { return stream_active_; }

void AudioManager::streamTask(void* arg) {
    static_cast<AudioManager*>(arg)->runStreamLoop();
}

void AudioManager::runStreamLoop() {
    size_t totalBytes = 0;
    bool connected = false;
    // Keep large PCM scratch off the task stack — HTTPS + HTTPClient already
    // consume most of the 12 KiB budget and overflow was crashing after a few plays.
    static uint8_t s_pcm_accum[kPcmAccumBytes];

    auto enqueuePcm = [&](const uint8_t* data, size_t len) {
        size_t offset = 0;
        while (offset < len && stream_active_ && stream_buffer_ != nullptr) {
            if (xStreamBufferBytesAvailable(stream_buffer_) > (kStreamBufferBytes * 85) / 100) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            const size_t sent = xStreamBufferSend(stream_buffer_, data + offset, len - offset,
                                                    kStreamSendWaitTicks);
            if (sent == 0) {
                vTaskDelay(pdMS_TO_TICKS(2));
                continue;
            }
            offset += sent;
            totalBytes += sent;
        }
    };

    for (uint8_t attempt = 1; stream_active_ && attempt <= 2 && !connected; ++attempt) {
        HTTPClient http;
        http.setTimeout(2000);
        http.setReuse(false);
        http.useHTTP10(true);
        if (!http.begin(stream_url_.c_str())) {
            Serial.printf("[Audio] PCM HTTP begin failed (attempt %u)\n", attempt);
        } else {
            if (!stream_device_token_.empty()) http.addHeader("X-Device-Token", stream_device_token_.c_str());
            const int status = http.GET();
            if (status != HTTP_CODE_OK) {
                Serial.printf("[Audio] PCM stream HTTP %d (attempt %u)\n", status, attempt);
            } else {
                connected = true;
                WiFiClient* input = http.getStreamPtr();
                const int contentLength = http.getSize();

                if (contentLength >= 0) {
                    // Content-Length / HTTP1.0 body: raw PCM bytes.
                    while (stream_active_ && (http.connected() || input->available())) {
                        const size_t available = input->available();
                        if (available == 0) {
                            vTaskDelay(pdMS_TO_TICKS(1));
                            continue;
                        }
                        const size_t toRead =
                            available < sizeof(s_pcm_accum) ? available : sizeof(s_pcm_accum);
                        const size_t bytesRead = input->readBytes(s_pcm_accum, toRead);
                        if (bytesRead > 0) enqueuePcm(s_pcm_accum, bytesRead);
                    }
                } else {
                    // Transfer-Encoding: chunked — parse sizes without Arduino String
                    // (unbounded String += caused heap crashes after several tracks).
                    enum ChunkState { READ_HEADER, READ_DATA, READ_CRLF, EOF_CHUNKED };
                    ChunkState chunkState = READ_HEADER;
                    size_t chunkBytesRemaining = 0;
                    char headerBuf[16];
                    size_t headerLen = 0;
                    size_t pcm_accum_len = 0;

                    while (stream_active_ && (http.connected() || input->available())) {
                        if (chunkState == READ_HEADER) {
                            while (input->available() && chunkState == READ_HEADER && stream_active_) {
                                const char c = static_cast<char>(input->read());
                                if (c == '\n') {
                                    headerBuf[headerLen] = '\0';
                                    while (headerLen > 0 &&
                                           (headerBuf[headerLen - 1] == '\r' ||
                                            headerBuf[headerLen - 1] == ' ')) {
                                        headerBuf[--headerLen] = '\0';
                                    }
                                    if (headerLen > 0) {
                                        chunkBytesRemaining = strtoul(headerBuf, nullptr, 16);
                                        chunkState = (chunkBytesRemaining == 0) ? EOF_CHUNKED
                                                                                : READ_DATA;
                                    }
                                    headerLen = 0;
                                } else if (headerLen + 1 < sizeof(headerBuf)) {
                                    headerBuf[headerLen++] = c;
                                } else {
                                    Serial.println("[Audio] PCM chunk header overflow");
                                    chunkState = EOF_CHUNKED;
                                    break;
                                }
                            }
                        } else if (chunkState == READ_DATA) {
                            while (chunkBytesRemaining > 0 && input->available() && stream_active_) {
                                size_t toRead = chunkBytesRemaining;
                                if (toRead > sizeof(s_pcm_accum) - pcm_accum_len) {
                                    toRead = sizeof(s_pcm_accum) - pcm_accum_len;
                                }
                                if (toRead > input->available()) toRead = input->available();
                                const size_t bytesRead =
                                    input->readBytes(s_pcm_accum + pcm_accum_len, toRead);
                                if (bytesRead == 0) break;
                                pcm_accum_len += bytesRead;
                                chunkBytesRemaining -= bytesRead;
                                if (pcm_accum_len >= 1024) {
                                    enqueuePcm(s_pcm_accum, pcm_accum_len);
                                    pcm_accum_len = 0;
                                }
                            }
                            if (chunkBytesRemaining == 0) chunkState = READ_CRLF;
                        } else if (chunkState == READ_CRLF) {
                            if (input->available() >= 2) {
                                input->read();
                                input->read();
                                chunkState = READ_HEADER;
                                if (pcm_accum_len > 0) {
                                    enqueuePcm(s_pcm_accum, pcm_accum_len);
                                    pcm_accum_len = 0;
                                }
                            } else {
                                vTaskDelay(pdMS_TO_TICKS(1));
                            }
                        } else if (chunkState == EOF_CHUNKED) {
                            if (pcm_accum_len > 0) {
                                enqueuePcm(s_pcm_accum, pcm_accum_len);
                                pcm_accum_len = 0;
                            }
                            break;
                        }
                        if (input->available() == 0) vTaskDelay(pdMS_TO_TICKS(1));
                    }
                    if (pcm_accum_len > 0) enqueuePcm(s_pcm_accum, pcm_accum_len);
                }
            }
            http.end();
        }
        if (!connected && stream_active_ && attempt < 2) vTaskDelay(pdMS_TO_TICKS(150));
    }
    stream_producer_done_ = true;
    stream_task_handle_ = nullptr;
    Serial.printf("[Audio] PCM stream finished: %u bytes queued\n", (unsigned)totalBytes);
    vTaskDelete(nullptr);
}

void AudioManager::playbackTask(void* arg) {
    static_cast<AudioManager*>(arg)->runPlaybackLoop();
}

void AudioManager::runPlaybackLoop() {
    uint8_t buffer[kPlaybackChunkBytes];
    bool amp_enabled = false;
    while (stream_active_) {
        StreamBufferHandle_t buf = stream_buffer_;
        if (buf == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        const size_t available = xStreamBufferBytesAvailable(buf);
        if (available < kPlaybackPrebufferBytes && !stream_producer_done_) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        size_t bytesRead = 0;
        while (bytesRead < sizeof(buffer) && stream_active_) {
            buf = stream_buffer_;
            if (buf == nullptr) break;
            const size_t chunk = xStreamBufferReceive(buf, buffer + bytesRead,
                                                       sizeof(buffer) - bytesRead,
                                                       kPlaybackReceiveWaitTicks);
            if (chunk > 0) {
                bytesRead += chunk;
                continue;
            }
            if (stream_producer_done_) break;
        }
        if (bytesRead == 0 && stream_producer_done_) break;

        // Only write what we have — avoid stuffing silence underruns into I2S
        // as full frames (extra DMA work / edge clicks under load).
        if (bytesRead == 0) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }
        if (bytesRead & 1) --bytesRead;

        if (!amp_enabled) {
            gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 1);
            amp_enabled = true;
        }

        size_t offset = 0;
        while (offset < bytesRead && stream_active_) {
            size_t bytesWritten = 0;
            if (i2s_write(I2S_SPK_PORT, buffer + offset, bytesRead - offset,
                          &bytesWritten, pdMS_TO_TICKS(100)) != ESP_OK) {
                break;
            }
            if (bytesWritten == 0) break;
            offset += bytesWritten;
        }
    }
    gpio_set_level((gpio_num_t)I2S_SPK_SD_PIN, 0);
    stream_active_ = false;
    playback_task_handle_ = nullptr;
    vTaskDelete(nullptr);
}

bool AudioManager::startRecording(bool append) {
    if (!initialized_ || recording_active_) return false;
    Serial.println("[Audio] Recorder: preparing");
    stopStream();
    stopPassthrough();
    // This board's default PlatformIO partition is SPIFFS.  LittleFS mounted
    // on that partition can panic in lfs_alloc during the first write.
    Serial.println("[Audio] Recorder: mounting SPIFFS");
    if (!SPIFFS.begin(true)) {
        Serial.println("[Audio] SPIFFS mount failed");
        return false;
    }
    if (!append) {
        // FILE_WRITE is not reliably truncating on every Arduino SPIFFS
        // version. Remove the disposable cache first so a new utterance never
        // uploads bytes from the previous one.
        Serial.println("[Audio] Recorder: truncating prior PCM");
        recording_bytes_ = 0;
        SPIFFS.remove(kCompanionRecordingPath);  // false simply means no old cache.
    }
    File check = SPIFFS.open(kCompanionRecordingPath, append ? FILE_APPEND : FILE_WRITE);
    if (!check && !append) {
        // A previous large PCM file can leave the small SPIFFS partition full
        // or fragmented.  Companion recordings are disposable cache; recover
        // the filesystem rather than leaving REC permanently unusable.
        Serial.println("[Audio] Recorder: SPIFFS full; formatting audio cache");
        SPIFFS.format();
        SPIFFS.end();
        if (SPIFFS.begin(false)) check = SPIFFS.open(kCompanionRecordingPath, FILE_WRITE);
    }
    if (!check) {
        Serial.println("[Audio] Recorder: cannot open PCM file");
        return false;
    }
    check.close();
    Serial.println("[Audio] Recorder: PCM file ready");
    i2s_zero_dma_buffer(I2S_MIC_PORT);
    recording_active_ = true;
    Serial.printf("[Audio] Recording %s -> %s\n", append ? "resumed" : "started", kCompanionRecordingPath);
    if (xTaskCreate(AudioManager::recordingTask, "chat_record", kRecordingTaskStackSz,
                    this, kRecordingTaskPrio, &recording_task_handle_) != pdPASS) {
        Serial.println("[Audio] Recorder: task create failed");
        recording_active_ = false;
        return false;
    }
    return true;
}

void AudioManager::pauseRecording() {
    recording_active_ = false;
    if (recording_task_handle_ != nullptr) {
        // Let the recorder flush its final partial PCM batch and close SPIFFS
        // before the voice request opens the same file for upload.
        for (uint8_t waited = 0; recording_task_handle_ != nullptr && waited < 50; ++waited)
            vTaskDelay(pdMS_TO_TICKS(10));
        if (recording_task_handle_ != nullptr) {
            Serial.println("[Audio] Recorder: stop timed out");
            vTaskDelete(recording_task_handle_);
            recording_task_handle_ = nullptr;
        }
    }
}

void AudioManager::stopRecording() { pauseRecording(); }
bool AudioManager::isRecording() const { return recording_active_; }
size_t AudioManager::recordedBytes() const { return recording_bytes_; }
const char* AudioManager::recordingPath() const { return kCompanionRecordingPath; }

void AudioManager::recordingTask(void* arg) { static_cast<AudioManager*>(arg)->runRecordingLoop(); }

void AudioManager::runRecordingLoop() {
    File output = SPIFFS.open(kCompanionRecordingPath, FILE_APPEND);
    if (!output) {
        recording_active_ = false;
        recording_task_handle_ = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    int32_t readBuf[kRecordingChunkSamples];
    int16_t pcmBuf[kRecordingChunkSamples];
    uint8_t writeBatch[kRecordingWriteBatchBytes];
    size_t pendingBytes = 0;
    while (recording_active_) {
        size_t bytesRead = 0;
        if (i2s_read(I2S_MIC_PORT, readBuf, sizeof(readBuf), &bytesRead,
                     pdMS_TO_TICKS(100)) != ESP_OK || bytesRead == 0) continue;
        const size_t samples = bytesRead / sizeof(int32_t);
        for (size_t i = 0; i < samples; ++i) pcmBuf[i] = static_cast<int16_t>(readBuf[i] >> 16);
        const size_t bytes = samples * sizeof(int16_t);
        if (recording_bytes_ + pendingBytes + bytes > kMaxCompanionRecordingBytes) {
            Serial.println("[Audio] Recording reached 10 second limit");
            recording_active_ = false;
            break;
        }
        memcpy(writeBatch + pendingBytes, pcmBuf, bytes);
        pendingBytes += bytes;
        if (pendingBytes < sizeof(writeBatch)) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        if (output.write(writeBatch, pendingBytes) != pendingBytes) {
            Serial.println("[Audio] recording write failed");
            recording_active_ = false;
            break;
        }
        recording_bytes_ += pendingBytes;
        pendingBytes = 0;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (pendingBytes > 0 && output.write(writeBatch, pendingBytes) == pendingBytes)
        recording_bytes_ += pendingBytes;
    output.close();
    Serial.printf("[Audio] Recording paused/stopped: %u bytes\n", (unsigned)recording_bytes_);
    recording_task_handle_ = nullptr;
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
