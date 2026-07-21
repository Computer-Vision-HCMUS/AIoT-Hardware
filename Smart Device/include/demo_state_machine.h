/**
 * @file demo_state_machine.h
 * @brief State machine for EmotiCare UI navigation (spec 005)
 *
 * Maps DemoState values to ScreenId equivalents used by demo_app.cpp.
 */

#ifndef DEMO_STATE_MACHINE_H
#define DEMO_STATE_MACHINE_H

#include <cstdint>

/**
 * @enum DemoState
 * @brief States matching the 8 screens defined in spec 005
 */
enum class DemoState : uint8_t {
    HOME           = 0,  ///< Landing menu screen
    CHECK_IN       = 1,  ///< Emotion detection checkpoint
    SUPPORT        = 2,  ///< Emotion-based activity recommendation
    DISCOVER       = 3,  ///< Content selection hub (Music / Podcast)
    MUSIC_LIST     = 4,  ///< Scrollable music recommendations
    PODCAST_LIST   = 5,  ///< Scrollable podcast recommendations
    COMPANION_CHAT = 6,  ///< Voice conversation interface (UI simulation)
    INSIGHTS       = 7,  ///< Emotion statistics by time period
    MIC_TEST       = 8,  ///< Live audio passthrough: INMP441 → MAX98357
    ERROR_STATE    = 9   ///< Fault / error fallback
};

/**
 * @class DemoStateMachine
 * @brief Lightweight wrapper for current DemoState with transition helpers
 */
class DemoStateMachine {
public:
    DemoStateMachine();

    bool init();

    DemoState getCurrentState() const;
    void transitionTo(DemoState new_state);
    void reset();

    const char* getStateName() const;
    bool isReady() const;

private:
    bool      initialized_;
    DemoState current_state_;
};

#endif  // DEMO_STATE_MACHINE_H
