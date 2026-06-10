#include "hal/SoundSensor.h"
#include <Arduino.h>
#include "config.h"
#include "utils/Logger.h"

SoundSensor::SoundSensor() {}

bool SoundSensor::begin() {
    pinMode(PIN_SOUND_MIC, INPUT);
    return true;
}

float SoundSensor::readLevel() {
    int raw = analogRead(PIN_SOUND_MIC);
    float voltage = raw * (3.3f / 4095.0f);
    return voltage * 100.0f;
}
