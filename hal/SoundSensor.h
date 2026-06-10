#pragma once

#include "hal/ISensor.h"

class SoundSensor : public ISensor {
public:
    SoundSensor();
    bool begin() override;
    float readLevel();
};