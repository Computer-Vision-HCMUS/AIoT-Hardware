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
    bool startStream(const std::string& url);

    /** Stop the current server audio stream immediately and mute the speaker. */
    void stopStream();

    /** Kept for the application loop; PCM playback runs in its own task. */
    void update();
    bool isStreaming() const;

private:
    bool        initialized_;
    bool        passthrough_active_;
    TaskHandle_t task_handle_;
    volatile uint16_t peak_level_;

    volatile bool stream_active_;
    TaskHandle_t stream_task_handle_;
    std::string stream_url_;

    static void audioTask(void* arg);
    void runAudioLoop();
    static void streamTask(void* arg);
    void runStreamLoop();
};

#endif  // AUDIO_MANAGER_H
