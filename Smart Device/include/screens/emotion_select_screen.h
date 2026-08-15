#ifndef EMOTION_SELECT_SCREEN_H
#define EMOTION_SELECT_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawEmotionSelectScreen(DisplayController& display, const AppState& state);
void drawPostCheckInMenuScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // EMOTION_SELECT_SCREEN_H
