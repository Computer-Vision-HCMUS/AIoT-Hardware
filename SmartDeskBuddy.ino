#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config.h"
#include "utils/Logger.h"
#include "utils/TimeUtils.h"
#include "hal/LightSensor.h"
#include "hal/SoundSensor.h"
#include "hal/RtcModule.h"
#include "core/PomodoroTimer.h"
#include "core/SleepMonitor.h"
#include "ui/PomodoroScreen.h"
#include "ui/SleepScreen.h"
#include "core/StateMachine.h"

Adafruit_ILI9341 display(TFT_CS, TFT_DC, TFT_RST);
LightSensor lightSensor;
SoundSensor soundSensor;
RtcModule rtc;
PomodoroTimer pomodoroTimer;
SleepMonitor sleepMonitor(lightSensor, soundSensor, rtc);
PomodoroScreen pomodoroScreen(display, pomodoroTimer);
SleepScreen sleepScreen(display, sleepMonitor);
StateMachine stateMachine(display, pomodoroScreen, sleepScreen);

int lastButton = 0;
uint32_t lastDebounceMs = 0;

int readButton() {
    if (digitalRead(PIN_BUTTON_NEXT) == LOW) {
        return PIN_BUTTON_NEXT;
    }
    if (digitalRead(PIN_BUTTON_SELECT) == LOW) {
        return PIN_BUTTON_SELECT;
    }
    if (digitalRead(PIN_BUTTON_UP) == LOW) {
        return PIN_BUTTON_UP;
    }
    if (digitalRead(PIN_BUTTON_DOWN) == LOW) {
        return PIN_BUTTON_DOWN;
    }
    if (digitalRead(PIN_BUTTON_BACK) == LOW) {
        return PIN_BUTTON_BACK;
    }
    return 0;
}

void setup() {
    LOG_INIT();
    delay(1000);
    logInfo("SmartDesk Buddy MVP starting...");

    pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_SELECT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
    pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
    pinMode(PIN_BUTTON_BACK, INPUT_PULLUP);

    TimeUtils::beginStorage();
    Wire.begin(PIN_LIGHT_SENSOR_SDA, PIN_LIGHT_SENSOR_SCL);

    if (!rtc.begin()) {
        logInfo("RTC init failed");
    }
    if (!lightSensor.begin()) {
        logInfo("Light sensor init failed");
    }
    if (!soundSensor.begin()) {
        logInfo("Sound sensor init failed");
    }

    display.begin();
    display.setRotation(1);
    display.fillScreen(ILI9341_BLACK);

    pomodoroScreen.begin();
    sleepScreen.begin();
    pomodoroTimer.begin(rtc);
    sleepMonitor.begin();
    stateMachine.begin();
}

void loop() {
    int button = readButton();
    uint32_t now = millis();
    if (button != lastButton) {
        lastDebounceMs = now;
    }
    if ((now - lastDebounceMs) > 50 && button != 0 && button != lastButton) {
        stateMachine.onButtonPress(button);
    }
    lastButton = button;

    pomodoroTimer.update(now);
    sleepMonitor.update(now);
    stateMachine.update();
}
