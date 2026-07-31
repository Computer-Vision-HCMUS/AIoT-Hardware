#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "mock_data.h"

// ---------------------------------------------------------------------------
// Screen identifiers (8 screens per spec)
// ---------------------------------------------------------------------------
enum class ScreenId : uint8_t {
    HOME           = 0,
    BUTTON_TEST    = 10,
    CHECK_IN       = 1,
    SUPPORT        = 2,
    DISCOVER       = 3,
    MUSIC_LIST     = 4,
    PODCAST_LIST   = 5,
    COMPANION_CHAT = 6,
    INSIGHTS       = 7,
    MIC_TEST       = 8,  ///< Audio passthrough test (INMP441 → MAX98357)
    WIFI_SETUP     = 9   ///< WiFi mode toggle (connected ↔ AP provisioning)
};

// ---------------------------------------------------------------------------
// Physical button identifiers — labels change per screen
// Order matches GPIO assignment: MODE(0), ACTION(1), START(2), NEXT(3), BACK(4)
// ---------------------------------------------------------------------------
enum class ButtonId : uint8_t {
    MODE   = 0,  // Button 1
    ACTION = 1,  // Button 2
    START  = 2,  // Button 3
    NEXT   = 3,  // Button 4
    BACK   = 4   // Button 5
};

// ---------------------------------------------------------------------------
// Shared application context passed between screens
// ---------------------------------------------------------------------------
struct SharedContext {
    // CHECK_IN result
    std::string lastEmotion;    // e.g. "Happy"
    uint8_t     confidence;     // 0–100

    // COMPANION_CHAT state
    bool     isRecording;
    bool     companionRecordingReady;
    bool     companionSending;
    bool     companionSendRequested; // render Thinking before sequential API call
    uint32_t recordingStartMs;  // millis() when recording began
    std::string companionStatus;
    std::vector<ChatMessage> chatHistory;

    // INSIGHTS state
    uint8_t insightsPeriodIndex; // 0=Day,1=Week,2=Month
    bool insightsShowingAiAssessment;
    std::string insightsAiAssessment;

    // MIC_TEST / Audio state
    uint16_t micPeakLevel;   // 0–100, updated each frame by demo_app
    bool     audioActive;    // true while I2S passthrough task is running

    // Server media playback state
    bool        mediaPlaying;
    bool        mediaPlayRequested;
    bool        mediaStopRequested;
    std::string mediaTitle;
    std::string mediaStatus;

    // Device status
    std::string deviceStatus;   // e.g. "Device Ready"
    std::array<uint32_t, 5> buttonPressCounts;
    std::array<bool, 5> buttonPressed;
    uint8_t lastButtonId;
};

// ---------------------------------------------------------------------------
// Full application state
// ---------------------------------------------------------------------------
struct AppState {
    ScreenId currentScreen;
    ScreenId previousScreen;

    // Navigation history stack (for BACK)
    static constexpr uint8_t kMaxHistory = 8;
    std::array<ScreenId, kMaxHistory> history;
    uint8_t historySize;

    // Per-screen selection cursors
    uint8_t homeMenuIndex;    // 0=Check-In,1=Discover,2=Chat,3=Insights,4=TestMic
    uint8_t discoverIndex;    // 0=Music, 1=Podcast
    uint8_t musicScrollIndex;
    uint8_t podcastScrollIndex;
    uint8_t supportActivityIndex;
    bool    supportShowingDetail;
    std::vector<ActivityCard> supportActivities;

    // Check-in sub-state
    bool    checkInAnalyzing; // true while showing "analyzing..." phase
    bool    checkInRecording;
    bool    checkInHasRecording;
    bool    checkInProcessing;
    bool    checkInInferencePending;
    bool    checkInUncertain;
    bool    checkInConfirmed;
    uint32_t checkInProcessingStartMs;
    uint32_t checkInRecordingStartMs;
    std::string checkInStatus;
    std::string checkInDetectedEmotion;
    uint8_t checkInDetectedConfidence;

    // WiFi Setup sub-state
    uint8_t wifiSetupMenuIndex; // 0 = toggle mode, 1 = back

    SharedContext sharedContext;
};

// ---------------------------------------------------------------------------
// Navigation function declarations
// ---------------------------------------------------------------------------
void handleButtonPress(AppState& state, ButtonId button);
void handleHomeInput(AppState& state, ButtonId button);
void handleCheckInInput(AppState& state, ButtonId button);
void handleSupportInput(AppState& state, ButtonId button);
void handleDiscoverInput(AppState& state, ButtonId button);
void handleMusicListInput(AppState& state, ButtonId button);
void handlePodcastListInput(AppState& state, ButtonId button);
void handleCompanionChatInput(AppState& state, ButtonId button, uint32_t nowMs);
void handleInsightsInput(AppState& state, ButtonId button);
void handleMicTestInput(AppState& state, ButtonId button);
void handleWifiSetupInput(AppState& state, ButtonId button);
void handleButtonTestInput(AppState& state, ButtonId button);

void pushScreen(AppState& state, ScreenId nextScreen);
void goBack(AppState& state);
void resetToHome(AppState& state);
const char* screenIdToString(ScreenId screen);

#endif  // NAVIGATION_H
