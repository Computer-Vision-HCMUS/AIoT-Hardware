/**
 * @file navigation.cpp
 * @brief Navigation state machine for EmotiCare UI Shell (spec 005)
 *
 * Each screen handler defines context-sensitive button-to-function mappings.
 * Button IDs are physical constants; their semantic meaning changes per screen.
 *
 * Default button roles across most screens:
 *   MODE(0)  = contextual (menu, skip, etc.)
 *   ACTION(1)= SELECT / CONFIRM
 *   START(2) = secondary action
 *   NEXT(3)  = DOWN / scroll forward
 *   BACK(4)  = UP / scroll back / return
 */

#include "navigation.h"
#include "service.h"
#include <Arduino.h>

namespace {
// Push a screen onto the back-navigation history stack
void pushHistory(AppState& state, ScreenId screen) {
    if (state.historySize < AppState::kMaxHistory) {
        state.history[state.historySize++] = screen;
    } else {
        // Shift history left to make room
        for (uint8_t i = 1; i < AppState::kMaxHistory; ++i) {
            state.history[i - 1] = state.history[i];
        }
        state.history[AppState::kMaxHistory - 1] = screen;
    }
}
}  // namespace

// ---------------------------------------------------------------------------
// Entry point: route button presses to the active screen handler
// ---------------------------------------------------------------------------
void handleButtonPress(AppState& state, ButtonId button) {
    switch (state.currentScreen) {
        case ScreenId::HOME:           handleHomeInput(state, button);          break;
        case ScreenId::CHECK_IN:       handleCheckInInput(state, button);       break;
        case ScreenId::SUPPORT:        handleSupportInput(state, button);       break;
        case ScreenId::DISCOVER:       handleDiscoverInput(state, button);      break;
        case ScreenId::MUSIC_LIST:     handleMusicListInput(state, button);     break;
        case ScreenId::PODCAST_LIST:   handlePodcastListInput(state, button);   break;
        case ScreenId::COMPANION_CHAT:
            handleCompanionChatInput(state, button, (uint32_t)(millis()));
            break;
        case ScreenId::INSIGHTS:       handleInsightsInput(state, button);      break;
        case ScreenId::MIC_TEST:       handleMicTestInput(state, button);       break;
        case ScreenId::WIFI_SETUP:     handleWifiSetupInput(state, button);     break;
        case ScreenId::BUTTON_TEST:    handleButtonTestInput(state, button);    break;
    }
}

// ---------------------------------------------------------------------------
// HOME — menu cursor navigation + confirm selection
//
// Button map:
//   ACTION(1) = SELECT (enter highlighted menu item)
//   START(2)  = HOME shortcut (noop here, already home)
//   NEXT(3)   = DOWN (move cursor down)
//   BACK(4)   = UP   (move cursor up)
//   MODE(0)   = unused on HOME
// ---------------------------------------------------------------------------
void handleHomeInput(AppState& state, ButtonId button) {
    constexpr uint8_t kMenuItems = 7;
    if (button == ButtonId::NEXT) {
        if (state.homeMenuIndex + 1 < kMenuItems) {
            state.homeMenuIndex++;
        }
    } else if (button == ButtonId::BACK) {
        if (state.homeMenuIndex > 0) {
            state.homeMenuIndex--;
        }
    } else if (button == ButtonId::ACTION || button == ButtonId::START) {
        switch (state.homeMenuIndex) {
            case 0: pushScreen(state, ScreenId::CHECK_IN);       break;
            case 1: pushScreen(state, ScreenId::DISCOVER);       break;
            case 2: pushScreen(state, ScreenId::COMPANION_CHAT); break;
            case 3: pushScreen(state, ScreenId::INSIGHTS);       break;
            case 4: pushScreen(state, ScreenId::MIC_TEST);       break;
            case 5:
                state.sharedContext.buttonPressCounts.fill(0);
                state.sharedContext.buttonPressed.fill(false);
                state.sharedContext.lastButtonId = 0;
                pushScreen(state, ScreenId::BUTTON_TEST);
                break;
            case 6:
                state.wifiSetupMenuIndex = 0;
                pushScreen(state, ScreenId::WIFI_SETUP);
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// CHECK_IN — shows analyzing state, then reveals mock emotion result
//
// Button map:
//   ACTION(1) = CONFIRM / proceed to SUPPORT
//   BACK(4)   = cancel → HOME
// ---------------------------------------------------------------------------
void handleCheckInInput(AppState& state, ButtonId button) {
    Serial.printf("[CheckIn] Button=%u recording=%d captured=%d processing=%d\n",
                  static_cast<unsigned>(button), state.checkInRecording,
                  state.checkInHasRecording, state.checkInProcessing);
    if (state.checkInProcessing) return;
    if (button == ButtonId::MODE && state.checkInAnalyzing && !state.checkInRecording) {
        if (startAudioCapture(false)) {
            state.checkInRecording = true;
            state.checkInHasRecording = true;
            state.checkInRecordingStartMs = millis();
            state.checkInStatus = "Listening... speak naturally.";
        } else {
            state.checkInStatus = "Microphone unavailable. Try again.";
        }
    // Both physical S2 (ACTION) and S3 (START) execute local SER.
    } else if ((button == ButtonId::ACTION || button == ButtonId::START) &&
               state.checkInAnalyzing) {
        if (!state.checkInHasRecording) {
            state.checkInStatus = "Press REC before EXEC.";
            return;
        }
        if (state.checkInRecording) pauseAudioCapture();
        state.checkInRecording = false;
        state.checkInStatus = "Processing on device...";
        state.checkInProcessing = true;
        state.checkInInferencePending = true;
        state.checkInProcessingStartMs = millis();
    } else if ((button == ButtonId::ACTION || button == ButtonId::START) &&
               !state.checkInAnalyzing && !state.checkInConfirmed) {
        bool synced = false;
        if (confirmCheckInEmotion(state.checkInDetectedEmotion,
                                  state.checkInDetectedConfidence, synced)) {
            state.checkInConfirmed = true;
            state.sharedContext.lastEmotion = state.checkInDetectedEmotion;
            state.sharedContext.confidence = state.checkInDetectedConfidence;
            state.checkInStatus = synced ? "Saved & synced. Press S2/S3."
                                        : "Saved local. Press S2/S3.";
        } else {
            state.checkInStatus = "Save failed. Confirm again.";
        }
    } else if ((button == ButtonId::ACTION || button == ButtonId::START) &&
               !state.checkInAnalyzing && state.checkInConfirmed) {
        state.supportActivities = getRecommendedActivities(state.sharedContext.lastEmotion);
        state.supportActivityIndex = 0;
        state.supportShowingDetail = false;
        pushScreen(state, ScreenId::SUPPORT);
    } else if (button == ButtonId::BACK) {
        if (state.checkInRecording) {
            pauseAudioCapture();
            state.checkInRecording = false;
        }
        state.checkInHasRecording = false;
        goBack(state);
    }
}

// ---------------------------------------------------------------------------
// SUPPORT — show recommended activity for the detected emotion
//
// Button map:
//   ACTION(1) = open selected activity
//   NEXT(3)   = next activity
//   BACK(4)   = previous activity / leave detail
// ---------------------------------------------------------------------------
void handleSupportInput(AppState& state, ButtonId button) {
    const uint8_t count = static_cast<uint8_t>(state.supportActivities.size());

    if (button == ButtonId::MODE) {
        resetToHome(state);
    } else if (state.supportShowingDetail) {
        if (button == ButtonId::ACTION || button == ButtonId::BACK) {
            state.supportShowingDetail = false;
        }
    } else if (button == ButtonId::ACTION || button == ButtonId::START) {
        if (count > 0) {
            state.supportShowingDetail = true;
        }
    } else if (button == ButtonId::NEXT) {
        if (state.supportActivityIndex + 1 < count) {
            ++state.supportActivityIndex;
        }
    } else if (button == ButtonId::BACK) {
        if (state.supportActivityIndex > 0) {
            --state.supportActivityIndex;
        } else {
            goBack(state);
        }
    }
}

// ---------------------------------------------------------------------------
// DISCOVER — Music / Podcast submenu
//
// Button map:
//   ACTION(1) = SELECT highlighted option
//   NEXT(3)   = move cursor DOWN
//   BACK(4)   = move cursor UP / return HOME
// ---------------------------------------------------------------------------
void handleDiscoverInput(AppState& state, ButtonId button) {
    constexpr uint8_t kDiscoverItems = 2;  // Music, Podcast
    if (button == ButtonId::NEXT) {
        if (state.discoverIndex + 1 < kDiscoverItems) {
            state.discoverIndex++;
        }
    } else if (button == ButtonId::BACK) {
        if (state.discoverIndex > 0) {
            state.discoverIndex--;
        } else {
            goBack(state);
        }
    } else if (button == ButtonId::ACTION || button == ButtonId::START) {
        if (state.discoverIndex == 0) {
            state.musicScrollIndex = 0;
            pushScreen(state, ScreenId::MUSIC_LIST);
        } else {
            state.podcastScrollIndex = 0;
            pushScreen(state, ScreenId::PODCAST_LIST);
        }
    }
}

// ---------------------------------------------------------------------------
// MUSIC_LIST — scrollable list of 8+ songs
//
// Button map:
//   MODE(0)   = BACK to DISCOVER
//   START(2)  = play selected server track (matches the physical S2 wiring)
//   ACTION(1) = stop playback (matches the physical S3 wiring)
//   NEXT(3)   = scroll DOWN
//   BACK(4)   = scroll UP (or return to DISCOVER when at top)
// ---------------------------------------------------------------------------
void handleMusicListInput(AppState& state, ButtonId button) {
    constexpr uint8_t kMusicCount = 8;
    if (button == ButtonId::NEXT) {
        if (state.musicScrollIndex + 1 < kMusicCount) {
            state.musicScrollIndex++;
        }
    } else if (button == ButtonId::BACK) {
        if (state.musicScrollIndex > 0) {
            state.musicScrollIndex--;
        } else {
            goBack(state);
        }
    } else if (button == ButtonId::MODE) {
        goBack(state);
    } else if (button == ButtonId::START) {
        state.sharedContext.mediaPlayRequested = true;
    } else if (button == ButtonId::ACTION) {
        state.sharedContext.mediaStopRequested = true;
    }
}

// ---------------------------------------------------------------------------
// PODCAST_LIST — scrollable list of 6+ episodes
//
// Button map:
//   MODE(0)   = BACK to DISCOVER
//   START(2)  = play selected server episode (matches the physical S2 wiring)
//   ACTION(1) = stop playback (matches the physical S3 wiring)
//   NEXT(3)   = scroll DOWN
//   BACK(4)   = scroll UP (or return to DISCOVER when at top)
// ---------------------------------------------------------------------------
void handlePodcastListInput(AppState& state, ButtonId button) {
    constexpr uint8_t kPodcastCount = 6;
    if (button == ButtonId::NEXT) {
        if (state.podcastScrollIndex + 1 < kPodcastCount) {
            state.podcastScrollIndex++;
        }
    } else if (button == ButtonId::BACK) {
        if (state.podcastScrollIndex > 0) {
            state.podcastScrollIndex--;
        } else {
            goBack(state);
        }
    } else if (button == ButtonId::MODE) {
        goBack(state);
    } else if (button == ButtonId::START) {
        state.sharedContext.mediaPlayRequested = true;
    } else if (button == ButtonId::ACTION) {
        state.sharedContext.mediaStopRequested = true;
    }
}

// ---------------------------------------------------------------------------
// COMPANION_CHAT — RECORD / STOP recording simulation + chat bubbles
//
// Button map:
//   MODE(0)   = RECORD (start recording)
//   ACTION(1) = STOP   (stop recording, get mock reply)
//   BACK(4)   = return to HOME
// ---------------------------------------------------------------------------
void handleCompanionChatInput(AppState& state, ButtonId button, uint32_t nowMs) {
    Serial.printf("[Companion] Button=%u recording=%d sending=%d\n",
                  (unsigned)button, state.sharedContext.isRecording,
                  state.sharedContext.companionSending);
    if (state.sharedContext.companionSending) return;
    // S3 is intentionally unused in the simplified Companion flow.
    if (button == ButtonId::ACTION) return;
    if (button == ButtonId::MODE) {
        // RECORD — only start if not already recording
        if (!state.sharedContext.isRecording) {
            if (startAudioCapture(false)) {
                state.sharedContext.isRecording = true;
                state.sharedContext.recordingStartMs = nowMs;
                state.sharedContext.companionStatus = "Recording...";
            } else {
                state.sharedContext.companionStatus = "Mic unavailable";
            }
        }
    } else if (button == ButtonId::START) {
        // Physical S2 (confirmed by Button=2 logs): stop and send.
        if (!state.sharedContext.isRecording) {
            state.sharedContext.companionStatus = "Press REC first";
            return;
        }
        pauseAudioCapture();
        state.sharedContext.isRecording = false;
        state.sharedContext.companionSending = true;
        state.sharedContext.companionStatus = "Thinking...";
        if (!beginCompanionVoiceRequest()) {
            state.sharedContext.companionSending = false;
            state.sharedContext.companionStatus = "Cannot send recording";
        }
    } else if (button == ButtonId::ACTION) {
        // STOP — end recording session; add mock messages
        if (state.sharedContext.isRecording) {
            pauseAudioCapture();
            state.sharedContext.isRecording = false;
        }
        if (!state.sharedContext.isRecording) {
            state.sharedContext.companionSending = true;
            state.sharedContext.companionStatus = "Thinking...";
            if (!beginCompanionVoiceRequest()) {
                state.sharedContext.companionSending = false;
                state.sharedContext.companionStatus = "Cannot send recording";
            }
        }
    } else if (button == ButtonId::BACK) {
        pauseAudioCapture();
        state.sharedContext.isRecording = false;
        goBack(state);
    }
}

// ---------------------------------------------------------------------------
// INSIGHTS — emotion statistics with period cycling
//
// Button map:
//   ACTION(1) = NEXT PERIOD (cycle Day→Week→Month→Day)
//   START(2)  = PREV PERIOD
//   BACK(4)   = return HOME
// ---------------------------------------------------------------------------
void handleInsightsInput(AppState& state, ButtonId button) {
    constexpr uint8_t kPeriods = 3;  // Day, Week, Month
    if (button == ButtonId::ACTION || button == ButtonId::NEXT) {
        // Move to next period
        state.sharedContext.insightsPeriodIndex =
            (state.sharedContext.insightsPeriodIndex + 1) % kPeriods;
    } else if (button == ButtonId::START) {
        // Move to previous period
        if (state.sharedContext.insightsPeriodIndex > 0) {
            state.sharedContext.insightsPeriodIndex--;
        } else {
            state.sharedContext.insightsPeriodIndex = kPeriods - 1;
        }
    } else if (button == ButtonId::BACK) {
        goBack(state);
    }
}

// ---------------------------------------------------------------------------
// MIC_TEST — audio passthrough (started/stopped by demo_app on screen enter/leave)
//
// Button map:
//   BACK(4) = return HOME (demo_app will call stopPassthrough)
// ---------------------------------------------------------------------------
void handleMicTestInput(AppState& state, ButtonId button) {
    if (button == ButtonId::BACK) {
        goBack(state);
    }
}

void handleButtonTestInput(AppState& state, ButtonId button) {
    if (button == ButtonId::MODE) goBack(state);
}

// ---------------------------------------------------------------------------
// WIFI_SETUP — single toggle: ON = connected WiFi, OFF = AP provisioning
//
// Button map:
//   ACTION(1) = flip toggle (demo_app handles actual WiFi switching)
//   BACK(4)   = return to previous screen
// ---------------------------------------------------------------------------
void handleWifiSetupInput(AppState& state, ButtonId button) {
    if (button == ButtonId::ACTION || button == ButtonId::START) {
        // 0xFF = sentinel: demo_app should execute the WiFi toggle
        state.wifiSetupMenuIndex = 0xFF;
    } else if (button == ButtonId::BACK) {
        goBack(state);
    }
}

// ---------------------------------------------------------------------------
// Stack operations
// ---------------------------------------------------------------------------
void pushScreen(AppState& state, ScreenId nextScreen) {
    if (state.currentScreen != nextScreen) {
        pushHistory(state, state.currentScreen);
        state.previousScreen = state.currentScreen;
        state.currentScreen  = nextScreen;
    }
}

void goBack(AppState& state) {
    if (state.historySize == 0) {
        state.currentScreen = ScreenId::HOME;
        return;
    }
    ScreenId prev = state.history[--state.historySize];
    state.previousScreen = state.currentScreen;
    state.currentScreen  = prev;
}

void resetToHome(AppState& state) {
    state.previousScreen = state.currentScreen;
    state.currentScreen  = ScreenId::HOME;
    state.historySize    = 0;
}

// ---------------------------------------------------------------------------
// Debug helper
// ---------------------------------------------------------------------------
const char* screenIdToString(ScreenId screen) {
    switch (screen) {
        case ScreenId::HOME:           return "HOME";
        case ScreenId::CHECK_IN:       return "CHECK_IN";
        case ScreenId::SUPPORT:        return "SUPPORT";
        case ScreenId::DISCOVER:       return "DISCOVER";
        case ScreenId::MUSIC_LIST:     return "MUSIC_LIST";
        case ScreenId::PODCAST_LIST:   return "PODCAST_LIST";
        case ScreenId::COMPANION_CHAT: return "COMPANION_CHAT";
        case ScreenId::INSIGHTS:       return "INSIGHTS";
        case ScreenId::MIC_TEST:       return "MIC_TEST";
        case ScreenId::WIFI_SETUP:     return "WIFI_SETUP";
        case ScreenId::BUTTON_TEST:    return "BUTTON_TEST";
        default:                       return "UNKNOWN";
    }
}
