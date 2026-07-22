#ifndef WIFI_SCREEN_H
#define WIFI_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawWifiScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // WIFI_SCREEN_H