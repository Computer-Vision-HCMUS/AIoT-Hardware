/**
 * @file demo_state_machine.cpp
 * @brief Demo state machine implementation (spec 005)
 */

#include "demo_state_machine.h"

DemoStateMachine::DemoStateMachine()
    : initialized_(false), current_state_(DemoState::HOME) {}

bool DemoStateMachine::init() {
    if (initialized_) return true;
    current_state_ = DemoState::HOME;
    initialized_   = true;
    return true;
}

DemoState DemoStateMachine::getCurrentState() const {
    return current_state_;
}

void DemoStateMachine::transitionTo(DemoState new_state) {
    current_state_ = new_state;
}

void DemoStateMachine::reset() {
    current_state_ = DemoState::HOME;
}

const char* DemoStateMachine::getStateName() const {
    switch (current_state_) {
        case DemoState::HOME:           return "HOME";
        case DemoState::CHECK_IN:       return "CHECK_IN";
        case DemoState::SUPPORT:        return "SUPPORT";
        case DemoState::DISCOVER:       return "DISCOVER";
        case DemoState::MUSIC_LIST:     return "MUSIC_LIST";
        case DemoState::PODCAST_LIST:   return "PODCAST_LIST";
        case DemoState::COMPANION_CHAT: return "COMPANION_CHAT";
        case DemoState::INSIGHTS:       return "INSIGHTS";
        case DemoState::MIC_TEST:       return "MIC_TEST";
        case DemoState::ERROR_STATE:    return "ERROR";
        default:                        return "UNKNOWN";
    }
}

bool DemoStateMachine::isReady() const {
    return initialized_;
}
