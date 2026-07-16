#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawHomeScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // HOME_SCREEN_H
