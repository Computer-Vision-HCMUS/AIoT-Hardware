#pragma once

#include <RTClib.h>
#include "hal/ISensor.h"

class RtcModule : public ISensor {
public:
    RtcModule();
    bool begin() override;
    DateTime now();

private:
    RTC_DS3231 _rtc;
};