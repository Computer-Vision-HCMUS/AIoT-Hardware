#pragma once

#include "ui/Screen.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

enum AppMode {
    MODE_POMODORO,
    MODE_SLEEP
};

class StateMachine {
public:
    StateMachine(Adafruit_ILI9341 &display, Screen &pomodoro, Screen &sleepScreen);
    void begin();
    void update();
    void onButtonPress(int buttonId);
    AppMode currentMode() const;

private:
    Adafruit_ILI9341 &_display;
    Screen &_pomodoro;
    Screen &_sleep;
    Screen *_activeScreen;
    AppMode _mode;
    void switchMode(AppMode mode);
};