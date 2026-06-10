#pragma once

#include <Arduino.h>

#define LOG_INIT() Serial.begin(115200)
#define LOG_PRINTLN(msg) Serial.println(msg)
#define LOG_PRINTF(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)

inline void logInfo(const char *message) {
    Serial.println(message);
}

inline void logInfo(const String &message) {
    Serial.println(message);
}
