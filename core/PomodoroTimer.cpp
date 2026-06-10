#include "core/PomodoroTimer.h"
#include <Arduino.h>
#include "config.h"
#include "utils/Logger.h"
#include "utils/TimeUtils.h"

PomodoroTimer::PomodoroTimer()
    : _state(POMO_IDLE), _focusMinutes(DEFAULT_FOCUS_MINUTES), _breakMinutes(DEFAULT_BREAK_MINUTES),
      _remainingMs(DEFAULT_FOCUS_MINUTES * 60000u), _lastUpdateMs(0), _completedSessions(0), _rtc(nullptr) {
}

bool PomodoroTimer::begin(RtcModule &rtc) {
    _rtc = &rtc;
    restoreActiveState();
    return true;
}

void PomodoroTimer::update(uint32_t nowMs) {
    if (_state != POMO_RUNNING) {
        _lastUpdateMs = nowMs;
        return;
    }
    if (_lastUpdateMs == 0) {
        _lastUpdateMs = nowMs;
        return;
    }
    uint32_t delta = nowMs - _lastUpdateMs;
    if (delta == 0) {
        return;
    }
    if (delta >= _remainingMs) {
        _remainingMs = 0;
    } else {
        _remainingMs -= delta;
    }
    _lastUpdateMs = nowMs;
    if (_remainingMs == 0) {
        completeSession();
    }
    persistState();
}

void PomodoroTimer::start() {
    if (_state == POMO_BREAK) {
        _state = POMO_RUNNING;
    } else {
        _state = POMO_RUNNING;
    }
    _lastUpdateMs = millis();
    persistState();
}

void PomodoroTimer::pause() {
    if (_state == POMO_RUNNING) {
        _state = POMO_PAUSED;
        persistState();
    }
}

void PomodoroTimer::reset() {
    _state = POMO_IDLE;
    _remainingMs = _focusMinutes * 60000u;
    _lastUpdateMs = 0;
    TimeUtils::clearActivePomodoro();
}

void PomodoroTimer::setFocusMinutes(int minutes) {
    _focusMinutes = constrain(minutes, MIN_FOCUS_MINUTES, MAX_FOCUS_MINUTES);
    if (_state != POMO_RUNNING) {
        _remainingMs = _focusMinutes * 60000u;
    }
    persistState();
}

void PomodoroTimer::setBreakMinutes(int minutes) {
    _breakMinutes = constrain(minutes, MIN_BREAK_MINUTES, MAX_BREAK_MINUTES);
    persistState();
}

int PomodoroTimer::getFocusMinutes() const {
    return _focusMinutes;
}

int PomodoroTimer::getBreakMinutes() const {
    return _breakMinutes;
}

int PomodoroTimer::getRemainingMinutes() const {
    return (_remainingMs + 59999u) / 60000u;
}

bool PomodoroTimer::isRunning() const {
    return _state == POMO_RUNNING;
}

uint16_t PomodoroTimer::getCompletedSessions() const {
    return _completedSessions;
}

String PomodoroTimer::stateName() const {
    switch (_state) {
        case POMO_RUNNING:
            return "Running";
        case POMO_PAUSED:
            return "Paused";
        case POMO_BREAK:
            return "Break";
        case POMO_IDLE:
        default:
            return "Idle";
    }
}

void PomodoroTimer::completeSession() {
    if (_state == POMO_RUNNING) {
        _completedSessions++;
        TimeUtils::savePomodoroHistory((uint32_t)(_rtc ? _rtc->now().unixtime() : 0), _focusMinutes, _breakMinutes);
        tone(PIN_SPEAKER, 2000, 500);
        _state = POMO_BREAK;
        _remainingMs = _breakMinutes * 60000u;
    } else if (_state == POMO_BREAK) {
        tone(PIN_SPEAKER, 1200, 500);
        _state = POMO_IDLE;
        _remainingMs = _focusMinutes * 60000u;
    }
    _lastUpdateMs = millis();
    persistState();
}

void PomodoroTimer::persistState() {
    TimeUtils::saveActivePomodoro((uint8_t)_state, _remainingMs, _focusMinutes, _breakMinutes);
}

void PomodoroTimer::restoreActiveState() {
    uint8_t state;
    uint32_t remainingMs;
    uint16_t focusMin;
    uint16_t breakMin;
    if (TimeUtils::loadActivePomodoro(state, remainingMs, focusMin, breakMin)) {
        _state = (PomodoroState)state;
        _remainingMs = remainingMs;
        _focusMinutes = focusMin;
        _breakMinutes = breakMin;
    }
}
