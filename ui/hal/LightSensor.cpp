#include "hal/LightSensor.h"
#include <Wire.h>
#include <BH1750.h>
#include "Logger.h"

static BH1750 lightSensor;

LightSensor::LightSensor() : _lastLux(0.0f) {}

bool LightSensor::begin() {
    Wire.begin(PIN_LIGHT_SENSOR_SDA, PIN_LIGHT_SENSOR_SCL);
    if (!lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        logInfo("LightSensor: BH1750 init failed");
        return false;
    }
    return true;
}

float LightSensor::readLux() {
    float lux = lightSensor.readLightLevel();
    if (lux < 0) {
        return _lastLux;
    }
    _lastLux = lux;
    return lux;
}
