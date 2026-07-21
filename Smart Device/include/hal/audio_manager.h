/**
 * @file audio_manager.h
 * @brief I2S audio passthrough: INMP441 microphone → MAX98357 speaker
 *
 * Uses two ESP32 I2S peripherals:
 *   I2S_NUM_1 (I2S_MIC_PORT)  — RX, 32-bit, reads INMP441
 *   I2S_NUM_0 (I2S_SPK_PORT)  — TX, 16-bit, drives MAX98357
 *
 * A FreeRTOS task continuously reads mic samples, down-converts to 16-bit,
 * and writes to the speaker. The peak level is updated each buffer cycle
 * for display on the VU meter.
 */

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class AudioManager {
public:
    AudioManager();

    /**
     * @brief Install I2S drivers for both speaker and microphone.
     * @return true on success, false if driver install failed.
     */
    bool init();

    /**
     * @brief Uninstall I2S drivers and free resources.
     */
    void deinit();

    /**
     * @brief Start the FreeRTOS audio passthrough task.
     *        Enables MAX98357 via SD_MODE pin.
     */
    void startPassthrough();

    /**
     * @brief Stop the passthrough task and silence the speaker.
     */
    void stopPassthrough();

    /** @return true while passthrough task is running */
    bool isActive() const;

    /**
     * @return Current peak level 0–100 (updated every DMA buffer cycle)
     *         for VU meter display. Thread-safe read.
     */
    uint16_t getPeakLevel() const;

private:
    bool        initialized_;
    bool        passthrough_active_;
    TaskHandle_t task_handle_;
    volatile uint16_t peak_level_;

    static void audioTask(void* arg);
    void        runAudioLoop();
};

#endif  // AUDIO_MANAGER_H
