/**
 * @file demo_app.cpp
 * @brief Main application loop for EmotiCare UI Shell (spec 005)
 *
 * Coordinates:
 *  - DisplayController (TFT rendering)
 *  - ButtonManager     (GPIO debounce)
 *  - DemoStateMachine  (thin state wrapper)
 *  - AppState / navigation (9-screen state machine, context-sensitive buttons)
 *  - AudioManager      (I2S passthrough: INMP441 → MAX98357, MIC_TEST screen)
 *  - Screen handler draw functions (one per screen)
 */

#include "core/demo_app.h"
#include "ui/demo_rendering.h"
#include "navigation.h"
#include "network_manager.h"
#include "edge_api_client.h"
#include "service.h"

#include "screens/home_screen.h"
#include "screens/checkin_screen.h"
#include "screens/support_screen.h"
#include "screens/discover_screen.h"
#include "screens/music_list_screen.h"
#include "screens/podcast_list_screen.h"
#include "screens/companion_chat_screen.h"
#include "screens/insights_screen.h"
#include "screens/mic_test_screen.h"
#include "screens/wifi_screen.h"
#include "screens/button_test_screen.h"

#include <Arduino.h>
#include <esp_timer.h>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
DemoApp::DemoApp()
    : display_(nullptr),
      buttons_(nullptr),
      state_machine_(nullptr),
      network_(nullptr),
      edge_api_(nullptr),
      audio_(nullptr),
      demo_running_(false),
      boot_time_ms_(0),
      last_transition_ms_(0),
      last_rendered_state_(DemoState::HOME),
      needs_redraw_(true) {}

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
bool DemoApp::init() {
    if (demo_running_) return true;

    boot_time_ms_ = esp_timer_get_time();

    display_       = new DisplayController();
    buttons_       = new ButtonManager();
    state_machine_ = new DemoStateMachine();
    audio_         = new AudioManager();

    if (!display_ || !display_->init()) {
        DemoRendering::renderErrorScreen(*display_, "Display init failed!");
        return false;
    }

    if (!buttons_ || !buttons_->init()) {
        display_->clear();
        display_->setColor(255, 0, 0);
        display_->drawText(20, 20, "Button init failed!", 2);
        return false;
    }

    if (!state_machine_ || !state_machine_->init()) {
        display_->clear();
        display_->setColor(255, 0, 0);
        display_->drawText(20, 20, "SM init failed!", 2);
        return false;
    }

    network_ = new NetworkManager();
    if (network_) {
        network_->begin();
        edge_api_ = new EdgeApiClient(*network_);
        serviceConfigureEdgeApi(edge_api_);
    }

    // AudioManager: init I2S drivers.
    // Non-fatal if it fails (device may not have audio hardware yet).
    if (audio_) {
        if (!audio_->init()) {
            Serial.println("[App] WARNING: AudioManager init failed — mic test unavailable");
            delete audio_;
            audio_ = nullptr;
        }
    }
    serviceConfigureAudioManager(audio_);

    // ── Initialise AppState ──
    app_state_.currentScreen  = ScreenId::HOME;
    app_state_.previousScreen = ScreenId::HOME;
    app_state_.historySize    = 0;

    app_state_.homeMenuIndex      = 0;
    app_state_.discoverIndex      = 0;
    app_state_.musicScrollIndex   = 0;
    app_state_.podcastScrollIndex = 0;
    app_state_.supportActivityIndex = 0;
    app_state_.supportShowingDetail = false;
    app_state_.supportActivities.clear();
    app_state_.checkInAnalyzing   = true;
    app_state_.checkInRecording   = false;
    app_state_.checkInHasRecording = false;
    app_state_.checkInProcessing  = false;
    app_state_.checkInInferencePending = false;
    app_state_.checkInConfirmed = false;
    app_state_.checkInUncertain   = false;
    app_state_.checkInRecordingStartMs = 0;
    app_state_.checkInStatus = "Press REC, then say a short sentence.";
    app_state_.checkInDetectedEmotion.clear();
    app_state_.checkInDetectedConfidence = 0;
    app_state_.wifiSetupMenuIndex = 0;

    app_state_.sharedContext.lastEmotion         = "Neutral";
    app_state_.sharedContext.confidence          = 0;
    app_state_.sharedContext.isRecording         = false;
    app_state_.sharedContext.companionRecordingReady = false;
    app_state_.sharedContext.companionSending    = false;
    app_state_.sharedContext.companionSendRequested = false;
    app_state_.sharedContext.recordingStartMs    = 0;
    app_state_.sharedContext.companionStatus.clear();
    app_state_.sharedContext.insightsPeriodIndex = 0;
    app_state_.sharedContext.insightsShowingAiAssessment = false;
    app_state_.sharedContext.insightsAiAssessment.clear();
    app_state_.sharedContext.micPeakLevel        = 0;
    app_state_.sharedContext.audioActive         = false;
    app_state_.sharedContext.mediaPlaying        = false;
    app_state_.sharedContext.mediaPlayRequested  = false;
    app_state_.sharedContext.mediaStopRequested  = false;
    app_state_.sharedContext.mediaTitle.clear();
    app_state_.sharedContext.mediaStatus.clear();
    app_state_.sharedContext.deviceStatus        = network_ ? network_->statusLabel().c_str() : "Offline";
    app_state_.sharedContext.buttonPressCounts.fill(0);
    app_state_.sharedContext.buttonPressed.fill(false);
    app_state_.sharedContext.lastButtonId = 0;
    std::string savedEmotion;
    uint8_t savedConfidence = 0;
    if (loadConfirmedEmotion(savedEmotion, savedConfidence)) {
        app_state_.sharedContext.lastEmotion = savedEmotion;
        app_state_.sharedContext.confidence = savedConfidence;
        Serial.printf("[CheckIn] Restored confirmed emotion: %s (%u%%)\n",
                      savedEmotion.c_str(), savedConfidence);
    }
    last_companion_render_ms_ = 0;
    last_checkin_render_ms_ = 0;

    demo_running_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// update() — called from loop()
// ---------------------------------------------------------------------------
bool DemoApp::update() {
    if (!demo_running_) return false;

    if (network_) {
        network_->update();
        app_state_.sharedContext.deviceStatus = network_->statusLabel().c_str();
    }

    if (audio_) {
        const bool wasStreaming = audio_->isStreaming();
        audio_->update();
        app_state_.sharedContext.mediaPlaying = audio_->isStreaming();
        if (wasStreaming && !app_state_.sharedContext.mediaPlaying) {
            app_state_.sharedContext.mediaStatus = "Playback finished";
            needs_redraw_ = true;
        }
    }

    // The long Whisper → LLM → TTS request runs in a dedicated FreeRTOS task;
    // polling here keeps the TFT and buttons responsive while it is running.
    if (app_state_.sharedContext.companionSending) {
        std::string transcript;
        std::string reply;
        bool ok = false;
        bool audioStarted = false;
        if (takeCompanionVoiceResult(transcript, reply, ok, audioStarted)) {
            app_state_.sharedContext.companionSending = false;
            if (ok) {
            const uint32_t now = millis();
            app_state_.sharedContext.chatHistory.push_back({"user", transcript, now});
            app_state_.sharedContext.chatHistory.push_back({"ai", reply, now + 1});
            app_state_.sharedContext.companionStatus = audioStarted ? "Reply playing" : "Reply text only";
            } else {
                app_state_.sharedContext.companionStatus = "Voice request failed";
            }
            needs_redraw_ = true;
        }
    }

    // ── Button input ──
    if (buttons_) {
        buttons_->update();
        for (uint8_t i = 0; i < 5; ++i) {
            const ButtonState buttonState = buttons_->getButtonState(static_cast<ButtonID>(i));
            app_state_.sharedContext.buttonPressCounts[i] = buttonState.press_count;
            app_state_.sharedContext.buttonPressed[i] = buttonState.currently_pressed;
        }

        ButtonId pressed = ButtonId::MODE;
        bool any = false;

        if      (buttons_->wasPressed(ButtonID::MODE))   { pressed = ButtonId::MODE;   any = true; }
        else if (buttons_->wasPressed(ButtonID::ACTION)) { pressed = ButtonId::ACTION; any = true; }
        else if (buttons_->wasPressed(ButtonID::START))  { pressed = ButtonId::START;  any = true; }
        else if (buttons_->wasPressed(ButtonID::NEXT))   { pressed = ButtonId::NEXT;   any = true; }
        else if (buttons_->wasPressed(ButtonID::BACK))   { pressed = ButtonId::BACK;   any = true; }

        if (any) {
            app_state_.sharedContext.lastButtonId = static_cast<uint8_t>(pressed);
            handleButtonPress(app_state_, pressed);

            if (app_state_.sharedContext.mediaStopRequested) {
                app_state_.sharedContext.mediaStopRequested = false;
                if (audio_) audio_->stopStream();
                app_state_.sharedContext.mediaPlaying = false;
                app_state_.sharedContext.mediaStatus = "Playback stopped";
            }

            if (app_state_.sharedContext.mediaPlayRequested) {
                app_state_.sharedContext.mediaPlayRequested = false;

                std::string title;
                std::string url;
                if (app_state_.currentScreen == ScreenId::MUSIC_LIST) {
                    const auto songs = getRecommendedMusic();
                    if (app_state_.musicScrollIndex < songs.size()) {
                        const Song& selected = songs[app_state_.musicScrollIndex];
                        title = selected.title;
                        url = selected.sourceUrl;
                    }
                } else if (app_state_.currentScreen == ScreenId::PODCAST_LIST) {
                    const auto episodes = getRecommendedPodcast();
                    if (app_state_.podcastScrollIndex < episodes.size()) {
                        const PodcastEpisode& selected = episodes[app_state_.podcastScrollIndex];
                        title = selected.title;
                        url = selected.sourceUrl;
                    }
                }

                if (title.empty() || url.empty()) {
                    app_state_.sharedContext.mediaStatus = "No playable item from server";
                    app_state_.sharedContext.mediaPlaying = false;
                } else if (audio_ && audio_->startStream(url)) {
                    app_state_.sharedContext.mediaTitle = title;
                    app_state_.sharedContext.mediaStatus = "Starting: " + title;
                    app_state_.sharedContext.mediaPlaying = true;
                } else {
                    app_state_.sharedContext.mediaStatus = "Audio output unavailable";
                    app_state_.sharedContext.mediaPlaying = false;
                }
            }

            needs_redraw_ = true;
            Serial.print("[App] Button -> Screen: ");
            Serial.println(screenIdToString(app_state_.currentScreen));
        }
    }

    // ── Sync DemoStateMachine with AppState ──
    if (state_machine_) {
        DemoState target = DemoState::HOME;
        switch (app_state_.currentScreen) {
            case ScreenId::HOME:           target = DemoState::HOME;           break;
            case ScreenId::CHECK_IN:       target = DemoState::CHECK_IN;       break;
            case ScreenId::SUPPORT:        target = DemoState::SUPPORT;        break;
            case ScreenId::DISCOVER:       target = DemoState::DISCOVER;       break;
            case ScreenId::MUSIC_LIST:     target = DemoState::MUSIC_LIST;     break;
            case ScreenId::PODCAST_LIST:   target = DemoState::PODCAST_LIST;   break;
            case ScreenId::COMPANION_CHAT: target = DemoState::COMPANION_CHAT; break;
            case ScreenId::INSIGHTS:       target = DemoState::INSIGHTS;       break;
            case ScreenId::MIC_TEST:       target = DemoState::MIC_TEST;       break;
            case ScreenId::WIFI_SETUP:     target = DemoState::WIFI_SETUP;     break;
            case ScreenId::BUTTON_TEST:    target = DemoState::BUTTON_TEST;    break;
        }

        DemoState current = target;
        bool stateChanged = (current != last_rendered_state_);

        state_machine_->transitionTo(current);

        // ── Handle screen transitions for audio ──
        if (stateChanged) {
            // Media lists always open in a predictable stopped state. Playback
            // only starts after the user presses Play for a selected item.
            if (current == DemoState::MUSIC_LIST || current == DemoState::PODCAST_LIST) {
                if (audio_) audio_->stopStream();
                app_state_.sharedContext.mediaPlaying = false;
                app_state_.sharedContext.mediaPlayRequested = false;
                app_state_.sharedContext.mediaStopRequested = false;
                app_state_.sharedContext.mediaTitle.clear();
                app_state_.sharedContext.mediaStatus = "Select an item, then press Play";
            }
            // Leaving MIC_TEST → stop audio
            if (last_rendered_state_ == DemoState::MIC_TEST && audio_) {
                audio_->stopPassthrough();
                app_state_.sharedContext.audioActive  = false;
                app_state_.sharedContext.micPeakLevel = 0;
            }
            // Entering MIC_TEST → start audio
            if (current == DemoState::MIC_TEST && audio_) {
                audio_->startPassthrough();
                app_state_.sharedContext.audioActive = true;
            }
            // Returning to HOME → reset check-in state
            if (current == DemoState::HOME) {
                if (app_state_.checkInRecording) pauseAudioCapture();
                app_state_.checkInAnalyzing = true;
                app_state_.checkInRecording = false;
                app_state_.checkInHasRecording = false;
                app_state_.checkInProcessing = false;
                app_state_.checkInInferencePending = false;
                app_state_.checkInConfirmed = false;
                app_state_.checkInUncertain = false;
                app_state_.checkInStatus = "Press REC, then say a short sentence.";
                app_state_.checkInDetectedEmotion.clear();
                app_state_.checkInDetectedConfidence = 0;
            }
        }

        // ── WiFi Setup: handle toggle sentinel ──
        if (current == DemoState::WIFI_SETUP &&
            app_state_.wifiSetupMenuIndex == 0xFF && network_) {
            app_state_.wifiSetupMenuIndex = 0;  // clear sentinel

            if (network_->isProvisioning()) {
                // Currently in AP mode → toggle ON: connect to saved WiFi
                Serial.println("[App] WiFi toggle ON: reconnecting to saved WiFi");
                network_->reconnectWifi();
            } else {
                // Currently connected (or offline) → toggle OFF: go to AP mode
                Serial.println("[App] WiFi toggle OFF: switching to AP provisioning");
                network_->startProvisioningAp();
            }

            // Sync deviceStatus immediately after WiFi state change so the
            // TFT render below reflects the new mode, not the stale value
            // that was captured at the top of this update() call.
            app_state_.sharedContext.deviceStatus = network_->statusLabel().c_str();
            needs_redraw_ = true;
        }

        // ── Live updates without button press ──
        // MIC_TEST: always redraw so VU meter animates
        if (current == DemoState::MIC_TEST) {
            if (audio_) {
                app_state_.sharedContext.micPeakLevel = audio_->getPeakLevel();
                app_state_.sharedContext.audioActive  = audio_->isActive();
            }
            needs_redraw_ = true;
        }

        // COMPANION_CHAT: end recording at 10 seconds. It remains available
        // for the user to send explicitly with S2 or S3.
        if (current == DemoState::COMPANION_CHAT &&
            app_state_.sharedContext.isRecording) {
            const uint32_t now = millis();
            if (now - app_state_.sharedContext.recordingStartMs >= 10000) {
                pauseAudioCapture();
                app_state_.sharedContext.isRecording = false;
                app_state_.sharedContext.companionRecordingReady = true;
                app_state_.sharedContext.companionStatus = "10s recorded. Press S2/S3 to send.";
                needs_redraw_ = true;
            } else if (now - last_companion_render_ms_ >= 250) {
                // Full TFT redraw is expensive. The timer needs only a 4 Hz update.
                last_companion_render_ms_ = now;
                needs_redraw_ = true;
            }
        }

        if (current == DemoState::CHECK_IN && app_state_.checkInRecording) {
            constexpr uint32_t kCheckInLimitMs = 10000;
            const uint32_t now = millis();
            if (now - app_state_.checkInRecordingStartMs >= kCheckInLimitMs) {
                // Time limit only ends capture.  The user must explicitly
                // press EXEC before an emotion result is displayed/saved.
                pauseAudioCapture();
                app_state_.checkInRecording = false;
                app_state_.checkInHasRecording = true;
                app_state_.checkInStatus = "Limit reached. Press EXEC.";
                needs_redraw_ = true;
            } else if (now - last_checkin_render_ms_ >= 250) {
                last_checkin_render_ms_ = now;
                needs_redraw_ = true;
            }
        }

        // Show the processing frame first, then run local SER synchronously on
        // the following loop. This avoids a stuck FreeRTOS worker state while
        // still giving the user visible feedback before inference begins.
        if (current == DemoState::CHECK_IN && app_state_.checkInProcessing &&
            app_state_.checkInInferencePending &&
            millis() - app_state_.checkInProcessingStartMs >= 100) {
            EmotionResult result;
            bool uncertain = false;
            app_state_.checkInInferencePending = false;
            const bool success = finishCheckInCapture(result, uncertain);
            app_state_.checkInProcessing = false;
            if (success) {
                    app_state_.checkInDetectedEmotion = result.label;
                    app_state_.checkInDetectedConfidence = result.confidence;
                    app_state_.checkInUncertain = uncertain;
                    app_state_.checkInAnalyzing = false;
                    app_state_.checkInConfirmed = false;
                    app_state_.checkInStatus = uncertain ? "Low confidence - confirm result."
                                                         : "Confirm to save emotion.";
            } else {
                app_state_.checkInHasRecording = false;
                app_state_.checkInStatus = "Audio too quiet/short. Please try again.";
            }
            needs_redraw_ = true;
        }

        // ── Render if state changed or input received ──
        if (stateChanged || needs_redraw_) {
            needs_redraw_ = false;

            switch (current) {
                case DemoState::HOME:
                    ScreenHandlers::drawHomeScreen(*display_, app_state_);
                    break;
                case DemoState::CHECK_IN:
                    ScreenHandlers::drawCheckInScreen(*display_, app_state_);
                    break;
                case DemoState::SUPPORT:
                    ScreenHandlers::drawSupportScreen(*display_, app_state_);
                    break;
                case DemoState::DISCOVER:
                    ScreenHandlers::drawDiscoverScreen(*display_, app_state_);
                    break;
                case DemoState::MUSIC_LIST:
                    ScreenHandlers::drawMusicListScreen(*display_, app_state_);
                    break;
                case DemoState::PODCAST_LIST:
                    ScreenHandlers::drawPodcastListScreen(*display_, app_state_);
                    break;
                case DemoState::COMPANION_CHAT:
                    ScreenHandlers::drawCompanionChatScreen(*display_, app_state_);
                    break;
                case DemoState::INSIGHTS:
                    ScreenHandlers::drawInsightsScreen(*display_, app_state_);
                    break;
                case DemoState::MIC_TEST:
                    ScreenHandlers::drawMicTestScreen(*display_, app_state_);
                    break;
                case DemoState::WIFI_SETUP:
                    ScreenHandlers::drawWifiScreen(*display_, app_state_);
                    break;
                case DemoState::BUTTON_TEST:
                    ScreenHandlers::drawButtonTestScreen(*display_, app_state_);
                    break;
                case DemoState::ERROR_STATE:
                    DemoRendering::renderErrorScreen(*display_, "System Error!");
                    state_machine_->transitionTo(DemoState::HOME);
                    break;
                default:
                    ScreenHandlers::drawHomeScreen(*display_, app_state_);
                    break;
            }

            last_rendered_state_ = current;
        }

        // Render Thinking... first, then run the voice API sequentially.
        if (current == DemoState::COMPANION_CHAT &&
            app_state_.sharedContext.companionSendRequested) {
            app_state_.sharedContext.companionSendRequested = false;
            if (!beginCompanionVoiceRequest()) {
                app_state_.sharedContext.companionSending = false;
                app_state_.sharedContext.companionStatus = "Cannot send recording";
                needs_redraw_ = true;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
bool DemoApp::isRunning() const { return demo_running_; }

bool DemoApp::isProvisioning() const {
    return network_ != nullptr && network_->isProvisioning();
}

void DemoApp::stop() {
    if (audio_) {
        audio_->stopPassthrough();
        audio_->deinit();
        delete audio_;
        audio_ = nullptr;
    }
    demo_running_ = false;
    serviceConfigureEdgeApi(nullptr);
    serviceConfigureAudioManager(nullptr);
    if (edge_api_)      { delete edge_api_;      edge_api_      = nullptr; }
    if (network_)       { delete network_;       network_       = nullptr; }
    if (display_)       { delete display_;       display_       = nullptr; }
    if (buttons_)       { delete buttons_;       buttons_       = nullptr; }
    if (state_machine_) { delete state_machine_; state_machine_ = nullptr; }
}
