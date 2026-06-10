#pragma once

#include "ui/Screen.h"
#include "core/SleepMonitor.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class SleepScreen : public Screen {
public:
    SleepScreen(Adafruit_ILI9341 &display, SleepMonitor &monitor);
    void begin() override;
    void onEnter() override;
    void update() override;
    void onButtonPress(int buttonId) override;

private:
    Adafruit_ILI9341 &_display;
    SleepMonitor &_monitor;
    bool _dirty;
    void drawStatus();
};