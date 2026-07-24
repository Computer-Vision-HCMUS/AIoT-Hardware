/**
 * @file audio_manager.h
 * @brief I2S audio for microphone passthrough and server media playback.
 */

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <cstdint>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class AudioManager {
public:
    AudioManager();

    bool init();
    void deinit();

    void startPassthrough();
    void stopPassthrough();
    bool isActive() const;
    uint16_t getPeakLevel() const;

    /** Start 16 kHz mono PCM playback from the server-provided media URL. */
    bool startStream(const std::string& url, const std::string& deviceToken = "");

    /** Stop the current server audio stream immediately and mute the speaker. */
    void stopStream();

    /** Kept for the application loop; PCM playback runs in its own task. */
    void update();
    bool isStreaming() const;

    /** Record 16-bit little-endian mono PCM into SPIFFS for Companion Chat. */
    bool startRecording(bool append = false);
    void pauseRecording();
    void stopRecording();
    bool isRecording() const;
    size_t recordedBytes() const;
    const char* recordingPath() const;

private:
    bool        initialized_;
    bool        passthrough_active_;
    TaskHandle_t task_handle_;
    volatile uint16_t peak_level_;

    volatile bool stream_active_;
    TaskHandle_t stream_task_handle_;
    std::string stream_url_;
    std::string stream_device_token_;

    volatile bool recording_active_;
    TaskHandle_t recording_task_handle_;
    volatile size_t recording_bytes_;

    static void audioTask(void* arg);
    void runAudioLoop();
    static void streamTask(void* arg);
    void runStreamLoop();
    static void recordingTask(void* arg);
    void runRecordingLoop();
};

#endif  // AUDIO_MANAGER_H
