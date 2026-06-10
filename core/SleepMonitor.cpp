#include "core/SleepMonitor.h"
#include "config.h"
#include "utils/Logger.h"
#include "utils/TimeUtils.h"

SleepMonitor::SleepMonitor(LightSensor &lightSensor, SoundSensor &soundSensor, RtcModule &rtc)
    : _lightSensor(lightSensor), _soundSensor(soundSensor), _rtc(rtc), _sleeping(false),
      _startTimestamp(0), _lightSum(0), _noiseSum(0), _sampleCount(0), _lastSampleMs(0), _lastScore(0),
      _lastRecommendation("---") {}

bool SleepMonitor::begin() {
    return true;
}

void SleepMonitor::update(uint32_t nowMs) {
    if (!_sleeping) {
        return;
    }
    if (_lastSampleMs == 0 || nowMs - _lastSampleMs >= SLEEP_SAMPLE_INTERVAL_MS) {
        float light = sampleLight();
        float noise = sampleNoise();
        _lightSum += (uint32_t)light;
        _noiseSum += (uint32_t)noise;
        _sampleCount++;
        _lastSampleMs = nowMs;
        computeQuality();
    }
}

bool SleepMonitor::isSleeping() const {
    return _sleeping;
}

void SleepMonitor::startSleep() {
    if (_sleeping) {
        return;
    }
    _sleeping = true;
    _startTimestamp = _rtc.now().unixtime();
    _lightSum = 0;
    _noiseSum = 0;
    _sampleCount = 0;
    _lastSampleMs = millis();
    _lastScore = 0;
    _lastRecommendation = "Recording...";
    TimeUtils::saveActiveSleep(1, _startTimestamp, _lightSum, _noiseSum, _sampleCount);
}

void SleepMonitor::stopSleep() {
    if (!_sleeping) {
        return;
    }
    _sleeping = false;
    uint32_t endTs = _rtc.now().unixtime();
    computeQuality();
    TimeUtils::saveSleepHistory(_startTimestamp, endTs, getAverageLight(), getAverageNoise(), _lastScore);
    TimeUtils::clearActiveSleep();
}

int SleepMonitor::getSleepDurationMinutes() const {
    if (!_sleeping) {
        return 0;
    }
    uint32_t nowTs = _rtc.now().unixtime();
    return (nowTs > _startTimestamp) ? (int)((nowTs - _startTimestamp) / 60u) : 0;
}

float SleepMonitor::getAverageLight() const {
    return _sampleCount ? (float)_lightSum / _sampleCount : 0.0f;
}

float SleepMonitor::getAverageNoise() const {
    return _sampleCount ? (float)_noiseSum / _sampleCount : 0.0f;
}

uint8_t SleepMonitor::getQualityScore() const {
    return _lastScore;
}

String SleepMonitor::getRecommendation() const {
    return _lastRecommendation;
}

float SleepMonitor::sampleLight() {
    return _lightSensor.readLux();
}

float SleepMonitor::sampleNoise() {
    return _soundSensor.readLevel();
}

void SleepMonitor::computeQuality() {
    int durationMin = getSleepDurationMinutes();
    float durationScore = min(100.0f, (durationMin / 480.0f) * 100.0f);
    float noiseAverage = getAverageNoise();
    float noiseScore = noiseAverage <= 50 ? 100 : max(0.0f, 100.0f - (noiseAverage - 50.0f) * 2.0f);
    float lightAverage = getAverageLight();
    float lightScore = lightAverage <= 30 ? 100 : max(0.0f, 100.0f - (lightAverage - 30.0f) * 1.5f);
    float score = durationScore * 0.6f + noiseScore * 0.2f + lightScore * 0.2f;
    _lastScore = (uint8_t)constrain(round(score), 0, 100);
    if (_lastScore >= 80) {
        _lastRecommendation = "Sleep is good.";
    } else if (_lastScore >= 60) {
        _lastRecommendation = "Sleep is fair.";
    } else {
        _lastRecommendation = "Sleep needs improvement.";
    }
}
