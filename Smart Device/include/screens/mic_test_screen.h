#ifndef MIC_TEST_SCREEN_H
#define MIC_TEST_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawMicTestScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // MIC_TEST_SCREEN_H
