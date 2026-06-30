#include "core/demo_app.h"
#include "ui/demo_rendering.h"
#include "demo.h"

#include <Arduino.h>
#include <esp_timer.h>

DemoApp::DemoApp()
    : display_(nullptr),
      buttons_(nullptr),
      state_machine_(nullptr),
      demo_running_(false),
      boot_time_ms_(0),
      last_transition_ms_(0),
      last_rendered_state_(DemoState::WELCOME) {}

bool DemoApp::init() {
    if (demo_running_) {
        return true;
    }

    boot_time_ms_ = esp_timer_get_time();

    display_ = new DisplayController();
    buttons_ = new ButtonManager();
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
        display_->drawText(20, 20, "State machine init failed!", 2);
        return false;
    }

    demo_running_ = true;
    return true;
}

bool DemoApp::update() {
    if (!demo_running_) {
        return false;
    }

    if (buttons_) {
        buttons_->update();

        if (buttons_->wasPressed(ButtonID::BUTTON_B)) {
            uint64_t now_ms = esp_timer_get_time() / 1000;
            if (now_ms - last_transition_ms_ > MIN_TRANSITION_INTERVAL_MS) {
                if (state_machine_) {
                    state_machine_->transitionNext();
                    last_transition_ms_ = now_ms;
                    Serial.println("[Demo] Button B: State changed");
                }
            }
        }
    }

    if (state_machine_) {
        DemoState current_state = state_machine_->getCurrentState();
        if (current_state != last_rendered_state_) {
            switch (current_state) {
                case DemoState::WELCOME:
                    DemoRendering::renderWelcomeScreen(*display_, boot_time_ms_);
                    break;
                case DemoState::DEVICE_INFO:
                    DemoRendering::renderDeviceInfoScreen(*display_);
                    break;
                case DemoState::ERROR_STATE:
                    DemoRendering::renderErrorScreen(*display_, "System Error Detected!");
                    state_machine_->transitionTo(DemoState::WELCOME);
                    break;
                default:
                    DemoRendering::renderWelcomeScreen(*display_, boot_time_ms_);
                    break;
            }
            last_rendered_state_ = current_state;
        }
    }

    return true;
}

bool DemoApp::isRunning() const {
    return demo_running_;
}

void DemoApp::stop() {
    demo_running_ = false;

    if (display_) {
        delete display_;
        display_ = nullptr;
    }

    if (buttons_) {
        delete buttons_;
        buttons_ = nullptr;
    }

    if (state_machine_) {
        delete state_machine_;
        state_machine_ = nullptr;
    }
}
