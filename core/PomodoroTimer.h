#pragma once

#include <Arduino.h>
#include "utils/TimeUtils.h"
#include "hal/RtcModule.h"

enum PomodoroState {
    POMO_IDLE,
    POMO_RUNNING,
    POMO_PAUSED,
    POMO_BREAK
};

class PomodoroTimer {
public:
    PomodoroTimer();
    bool begin(RtcModule &rtc);
    void update(uint32_t nowMs);
    void start();
    void pause();
    void reset();
    void setFocusMinutes(int minutes);
    void setBreakMinutes(int minutes);
    int getFocusMinutes() const;
    int getBreakMinutes() const;
    int getRemainingMinutes() const;
    bool isRunning() const;
    uint16_t getCompletedSessions() const;
    String stateName() const;
    void restoreActiveState();

private:
    PomodoroState _state;
    uint16_t _focusMinutes;
    uint16_t _breakMinutes;
    uint32_t _remainingMs;
    uint32_t _lastUpdateMs;
    uint16_t _completedSessions;
    RtcModule *_rtc;
    void completeSession();
    void persistState();
};