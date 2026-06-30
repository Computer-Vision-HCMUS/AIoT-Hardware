/**
 * @file demo_state_machine.cpp
 * @brief Demo state machine implementation
 */

#include "demo_state_machine.h"

// ============================================================================
// DemoStateMachine Implementation
// ============================================================================

DemoStateMachine::DemoStateMachine()
    : initialized_(false),
      current_state_(DemoState::WELCOME) {}

bool DemoStateMachine::init() {
    if (initialized_) {
        return true;
    }

    current_state_ = DemoState::WELCOME;
    initialized_ = true;
    return true;
}

DemoState DemoStateMachine::getCurrentState() const {
    return current_state_;
}

void DemoStateMachine::transitionNext() {
    current_state_ = getNextState(current_state_);
}

void DemoStateMachine::transitionTo(DemoState new_state) {
    current_state_ = new_state;
}

void DemoStateMachine::reset() {
    current_state_ = DemoState::WELCOME;
}

const char* DemoStateMachine::getStateName() const {
    switch (current_state_) {
        case DemoState::WELCOME:
            return "WELCOME";
        case DemoState::DEVICE_INFO:
            return "DEVICE_INFO";
        case DemoState::ERROR_STATE:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

bool DemoStateMachine::isReady() const {
    return initialized_;
}

DemoState DemoStateMachine::getNextState(DemoState current) {
    switch (current) {
        case DemoState::WELCOME:
            return DemoState::DEVICE_INFO;
        case DemoState::DEVICE_INFO:
            return DemoState::WELCOME;
        case DemoState::ERROR_STATE:
            return DemoState::WELCOME;  // Return to welcome after error
        default:
            return DemoState::WELCOME;
    }
}
