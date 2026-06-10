#include "ui/SleepScreen.h"
#include "config.h"
#include "utils/Logger.h"

SleepScreen::SleepScreen(Adafruit_ILI9341 &display, SleepMonitor &monitor)
    : _display(display), _monitor(monitor), _dirty(true) {}

void SleepScreen::begin() {
    _display.fillScreen(ILI9341_BLACK);
    _display.setTextColor(ILI9341_WHITE);
    _display.setTextSize(2);
}

void SleepScreen::onEnter() {
    _dirty = true;
    drawStatus();
}

void SleepScreen::update() {
    if (_dirty) {
        drawStatus();
        _dirty = false;
    }
}

void SleepScreen::onButtonPress(int buttonId) {
    switch (buttonId) {
        case PIN_BUTTON_SELECT:
            if (_monitor.isSleeping()) {
                _monitor.stopSleep();
            } else {
                _monitor.startSleep();
            }
            _dirty = true;
            break;
        default:
            break;
    }
}

void SleepScreen::drawStatus() {
    _display.fillScreen(ILI9341_BLACK);
    _display.setCursor(10, 10);
    _display.setTextSize(2);
    _display.print("Sleep Monitor");

    _display.setCursor(10, 50);
    _display.print("State: ");
    _display.print(_monitor.isSleeping() ? "Sleeping" : "Idle");

    _display.setCursor(10, 80);
    _display.print("Duration: ");
    _display.print(_monitor.getSleepDurationMinutes());
    _display.print("m");

    _display.setCursor(10, 110);
    _display.print("Light: ");
    _display.print(_monitor.getAverageLight(), 1);

    _display.setCursor(10, 140);
    _display.print("Noise: ");
    _display.print(_monitor.getAverageNoise(), 1);

    _display.setCursor(10, 170);
    _display.print("Score: ");
    _display.print(_monitor.getQualityScore());

    _display.setCursor(10, 200);
    _display.print(_monitor.getRecommendation());
}
