#pragma once

#include "hal/ISensor.h"

class LightSensor : public ISensor {
public:
    LightSensor();
    bool begin() override;
    float readLux();

private:
    float _lastLux;
};