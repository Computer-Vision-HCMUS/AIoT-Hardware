#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <RTClib.h>
#include "config.h"

namespace TimeUtils {

inline String formatDuration(uint32_t seconds) {
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    minutes %= 60;
    uint32_t secs = seconds % 60;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, secs);
    return String(buffer);
}

inline String formatTime(const DateTime &dt) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", dt.hour(), dt.minute(), dt.second());
    return String(buffer);
}

struct PomodoroRecord {
    uint32_t timestamp;
    uint16_t focusMinutes;
    uint16_t breakMinutes;
};

struct SleepRecord {
    uint32_t startTimestamp;
    uint32_t endTimestamp;
    float averageLight;
    float averageNoise;
    uint8_t score;
};

inline Preferences& prefs() {
    static Preferences instance;
    return instance;
}

inline void beginStorage() {
    prefs().begin("smartclock", false);
}

inline void endStorage() {
    prefs().end();
}

inline void saveActivePomodoro(uint8_t state, uint32_t remainingMs, uint16_t focusMin, uint16_t breakMin) {
    prefs().putUChar("pomoState", state);
    prefs().putUInt("pomoRemMs", remainingMs);
    prefs().putUShort("pomoFocus", focusMin);
    prefs().putUShort("pomoBreak", breakMin);
    prefs().putULong("pomoStamp", 0);
}

inline bool loadActivePomodoro(uint8_t &state, uint32_t &remainingMs, uint16_t &focusMin, uint16_t &breakMin) {
    if (!prefs().isKey("pomoState")) {
        return false;
    }
    state = prefs().getUChar("pomoState", 0);
    remainingMs = prefs().getUInt("pomoRemMs", 0);
    focusMin = prefs().getUShort("pomoFocus", DEFAULT_FOCUS_MINUTES);
    breakMin = prefs().getUShort("pomoBreak", DEFAULT_BREAK_MINUTES);
    return true;
}

inline void clearActivePomodoro() {
    prefs().remove("pomoState");
    prefs().remove("pomoRemMs");
    prefs().remove("pomoFocus");
    prefs().remove("pomoBreak");
}

inline void saveActiveSleep(uint8_t active, uint32_t startTs, uint32_t lightSum, uint32_t noiseSum, uint16_t samples) {
    prefs().putUChar("sleepActive", active);
    prefs().putULong("sleepStart", startTs);
    prefs().putULong("sleepLight", lightSum);
    prefs().putULong("sleepNoise", noiseSum);
    prefs().putUShort("sleepSamples", samples);
}

inline bool loadActiveSleep(uint8_t &active, uint32_t &startTs, uint32_t &lightSum, uint32_t &noiseSum, uint16_t &samples) {
    if (!prefs().isKey("sleepActive")) {
        return false;
    }
    active = prefs().getUChar("sleepActive", 0);
    startTs = prefs().getULong("sleepStart", 0);
    lightSum = prefs().getULong("sleepLight", 0);
    noiseSum = prefs().getULong("sleepNoise", 0);
    samples = prefs().getUShort("sleepSamples", 0);
    return true;
}

inline void clearActiveSleep() {
    prefs().remove("sleepActive");
    prefs().remove("sleepStart");
    prefs().remove("sleepLight");
    prefs().remove("sleepNoise");
    prefs().remove("sleepSamples");
}

inline void savePomodoroHistory(uint32_t timestamp, uint16_t focusMin, uint16_t breakMin) {
    uint16_t index = prefs().getUShort("pomoIndex", 0) + 1;
    if (index > MAX_HISTORY_RECORDS) {
        index = 1;
    }
    char key[16];
    snprintf(key, sizeof(key), "pomo%04u", index);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%u,%u,%u", timestamp, focusMin, breakMin);
    prefs().putString(key, buffer);
    prefs().putUShort("pomoIndex", index);
}

inline void saveSleepHistory(uint32_t startTs, uint32_t endTs, float avgLight, float avgNoise, uint8_t score) {
    uint16_t index = prefs().getUShort("sleepIndex", 0) + 1;
    if (index > MAX_HISTORY_RECORDS) {
        index = 1;
    }
    char key[16];
    snprintf(key, sizeof(key), "sleep%04u", index);
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "%u,%u,%.1f,%.1f,%u", startTs, endTs, avgLight, avgNoise, score);
    prefs().putString(key, buffer);
    prefs().putUShort("sleepIndex", index);
}

} // namespace TimeUtils
