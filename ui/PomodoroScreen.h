#pragma once

#include "ui/Screen.h"
#include "core/PomodoroTimer.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class PomodoroScreen : public Screen {
public:
    PomodoroScreen(Adafruit_ILI9341 &display, PomodoroTimer &timer);
    void begin() override;
    void onEnter() override;
    void update() override;
    void onButtonPress(int buttonId) override;

private:
    Adafruit_ILI9341 &_display;
    PomodoroTimer &_timer;
    bool _dirty;
    void drawStatus();
};