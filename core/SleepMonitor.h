#pragma once

#include <Arduino.h>
#include "hal/LightSensor.h"
#include "hal/SoundSensor.h"
#include "hal/RtcModule.h"
#include "utils/TimeUtils.h"

class SleepMonitor {
public:
    SleepMonitor(LightSensor &lightSensor, SoundSensor &soundSensor, RtcModule &rtc);
    bool begin();
    void update(uint32_t nowMs);
    bool isSleeping() const;
    void startSleep();
    void stopSleep();
    int getSleepDurationMinutes() const;
    float getAverageLight() const;
    float getAverageNoise() const;
    uint8_t getQualityScore() const;
    String getRecommendation() const;

private:
    LightSensor &_lightSensor;
    SoundSensor &_soundSensor;
    RtcModule &_rtc;
    bool _sleeping;
    uint32_t _startTimestamp;
    uint32_t _lightSum;
    uint32_t _noiseSum;
    uint16_t _sampleCount;
    uint32_t _lastSampleMs;
    uint8_t _lastScore;
    String _lastRecommendation;
    float sampleLight();
    float sampleNoise();
    void computeQuality();
};