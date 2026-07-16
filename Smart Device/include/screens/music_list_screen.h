#ifndef MUSIC_LIST_SCREEN_H
#define MUSIC_LIST_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawMusicListScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // MUSIC_LIST_SCREEN_H
