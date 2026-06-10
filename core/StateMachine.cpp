#include "core/StateMachine.h"
#include "config.h"
#include "utils/Logger.h"

StateMachine::StateMachine(Adafruit_ILI9341 &display, Screen &pomodoro, Screen &sleepScreen)
    : _display(display), _pomodoro(pomodoro), _sleep(sleepScreen), _activeScreen(nullptr), _mode(MODE_POMODORO) {}

void StateMachine::begin() {
    _display.fillScreen(ILI9341_BLACK);
    switchMode(_mode);
}

void StateMachine::update() {
    if (_activeScreen) {
        _activeScreen->update();
    }
}

void StateMachine::onButtonPress(int buttonId) {
    if (buttonId == PIN_BUTTON_NEXT) {
        AppMode next = (_mode == MODE_POMODORO) ? MODE_SLEEP : MODE_POMODORO;
        switchMode(next);
        return;
    }
    if (_activeScreen) {
        _activeScreen->onButtonPress(buttonId);
    }
}

AppMode StateMachine::currentMode() const {
    return _mode;
}

void StateMachine::switchMode(AppMode mode) {
    _mode = mode;
    _activeScreen = (_mode == MODE_POMODORO) ? &_pomodoro : &_sleep;
    if (_activeScreen) {
        _activeScreen->onEnter();
    }
}
