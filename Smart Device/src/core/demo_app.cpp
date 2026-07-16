/**
 * @file demo_app.cpp
 * @brief Main application loop for EmotiCare UI Shell (spec 005)
 *
 * Coordinates:
 *  - DisplayController (TFT rendering)
 *  - ButtonManager (GPIO debounce)
 *  - DemoStateMachine (thin state wrapper)
 *  - AppState / navigation (8-screen state machine, context-sensitive buttons)
 *  - Screen handler draw functions (one per screen)
 */

#include "core/demo_app.h"
#include "ui/demo_rendering.h"
#include "demo.h"
#include "navigation.h"

// New screen handlers
#include "screens/home_screen.h"
#include "screens/checkin_screen.h"
#include "screens/support_screen.h"
#include "screens/discover_screen.h"
#include "screens/music_list_screen.h"
#include "screens/podcast_list_screen.h"
#include "screens/companion_chat_screen.h"
#include "screens/insights_screen.h"

#include <Arduino.h>
#include <esp_timer.h>

// ---------------------------------------------------------------------------
// Constructor / init / teardown
// ---------------------------------------------------------------------------
DemoApp::DemoApp()
    : display_(nullptr),
      buttons_(nullptr),
      state_machine_(nullptr),
      demo_running_(false),
      boot_time_ms_(0),
      last_transition_ms_(0),
      last_rendered_state_(DemoState::HOME),
      needs_redraw_(true) {}

bool DemoApp::init() {
    if (demo_running_) return true;

    boot_time_ms_ = esp_timer_get_time();

    display_       = new DisplayController();
    buttons_       = new ButtonManager();
    state_machine_ = new DemoStateMachine();

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

    // ---------- initialise app state ----------
    app_state_.currentScreen  = ScreenId::HOME;
    app_state_.previousScreen = ScreenId::HOME;
    app_state_.historySize    = 0;

    app_state_.homeMenuIndex     = 0;
    app_state_.discoverIndex     = 0;
    app_state_.musicScrollIndex  = 0;
    app_state_.podcastScrollIndex= 0;
    app_state_.checkInAnalyzing  = true;

    app_state_.sharedContext.lastEmotion         = "Neutral";
    app_state_.sharedContext.confidence          = 0;
    app_state_.sharedContext.isRecording         = false;
    app_state_.sharedContext.recordingStartMs    = 0;
    app_state_.sharedContext.insightsPeriodIndex = 0;
    app_state_.sharedContext.deviceStatus        = "Device Ready";

    demo_running_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// Main update loop (called from loop() in main.cpp)
// ---------------------------------------------------------------------------
bool DemoApp::update() {
    if (!demo_running_) return false;

    // ---- Process button input ----
    if (buttons_) {
        buttons_->update();

        ButtonId pressed = ButtonId::MODE;
        bool any = false;

        if (buttons_->wasPressed(ButtonID::MODE))   { pressed = ButtonId::MODE;   any = true; }
        else if (buttons_->wasPressed(ButtonID::ACTION)) { pressed = ButtonId::ACTION; any = true; }
        else if (buttons_->wasPressed(ButtonID::START))  { pressed = ButtonId::START;  any = true; }
        else if (buttons_->wasPressed(ButtonID::NEXT))   { pressed = ButtonId::NEXT;   any = true; }
        else if (buttons_->wasPressed(ButtonID::BACK))   { pressed = ButtonId::BACK;   any = true; }

        if (any) {
            handleButtonPress(app_state_, pressed);
            needs_redraw_ = true;  // always redraw on any button press
            Serial.print("[App] Button -> Screen: ");
            Serial.println(screenIdToString(app_state_.currentScreen));
        }
    }

    // ---- Sync state machine with app_state ----
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
        }
        state_machine_->transitionTo(target);
    }

    // ---- Render if state changed ----
    if (state_machine_) {
        DemoState current = state_machine_->getCurrentState();
        if (current != last_rendered_state_ || needs_redraw_) {
            needs_redraw_ = false;
            switch (current) {
                case DemoState::HOME:
                    // Reset check-in state when returning to home
                    app_state_.checkInAnalyzing = true;
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

        // ---- Live update for recording timer (redraw when recording) ----
        if (app_state_.currentScreen == ScreenId::COMPANION_CHAT &&
            app_state_.sharedContext.isRecording) {
            ScreenHandlers::drawCompanionChatScreen(*display_, app_state_);
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
bool DemoApp::isRunning() const { return demo_running_; }

void DemoApp::stop() {
    demo_running_ = false;
    if (display_)       { delete display_;       display_       = nullptr; }
    if (buttons_)       { delete buttons_;       buttons_       = nullptr; }
    if (state_machine_) { delete state_machine_; state_machine_ = nullptr; }
}
