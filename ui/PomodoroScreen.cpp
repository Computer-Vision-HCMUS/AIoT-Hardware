#include "ui/PomodoroScreen.h"
#include "config.h"
#include "utils/Logger.h"

PomodoroScreen::PomodoroScreen(Adafruit_ILI9341 &display, PomodoroTimer &timer)
    : _display(display), _timer(timer), _dirty(true) {}

void PomodoroScreen::begin() {
    _display.fillScreen(ILI9341_BLACK);
    _display.setTextColor(ILI9341_WHITE);
    _display.setTextSize(2);
}

void PomodoroScreen::onEnter() {
    _dirty = true;
    drawStatus();
}

void PomodoroScreen::update() {
    if (_dirty) {
        drawStatus();
        _dirty = false;
    }
}

void PomodoroScreen::onButtonPress(int buttonId) {
    switch (buttonId) {
        case PIN_BUTTON_UP:
            _timer.setFocusMinutes(_timer.getFocusMinutes() + 1);
            _dirty = true;
            break;
        case PIN_BUTTON_DOWN:
            _timer.setFocusMinutes(_timer.getFocusMinutes() - 1);
            _dirty = true;
            break;
        case PIN_BUTTON_SELECT:
            if (_timer.isRunning()) {
                _timer.pause();
            } else {
                _timer.start();
            }
            _dirty = true;
            break;
        case PIN_BUTTON_BACK:
            _timer.reset();
            _dirty = true;
            break;
        default:
            break;
    }
}

void PomodoroScreen::drawStatus() {
    _display.fillScreen(ILI9341_BLACK);
    _display.setCursor(10, 10);
    _display.setTextSize(2);
    _display.print("Pomodoro");

    _display.setCursor(10, 50);
    _display.setTextSize(2);
    _display.print("Focus: ");
    _display.print(_timer.getFocusMinutes());
    _display.print("m");

    _display.setCursor(10, 80);
    _display.print("Break: ");
    _display.print(_timer.getBreakMinutes());
    _display.print("m");

    _display.setCursor(10, 120);
    _display.print("State: ");
    _display.print(_timer.stateName());

    _display.setCursor(10, 150);
    _display.print("Remaining: ");
    _display.print(_timer.getRemainingMinutes());
    _display.print("m");

    _display.setCursor(10, 180);
    _display.print("Done: ");
    _display.print(_timer.getCompletedSessions());
