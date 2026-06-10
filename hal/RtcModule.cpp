#include "hal/RtcModule.h"
#include <Wire.h>
#include "utils/Logger.h"

RtcModule::RtcModule() {}

bool RtcModule::begin() {
    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
    if (!_rtc.begin()) {
        logInfo("RTC: DS3231 init failed");
        return false;
    }
    if (_rtc.lostPower()) {
        logInfo("RTC lost power, setting to compile time");
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    return true;
}

DateTime RtcModule::now() {
    return _rtc.now();
}
