#ifndef PODCAST_LIST_SCREEN_H
#define PODCAST_LIST_SCREEN_H

#include "display_controller.h"
#include "navigation.h"

namespace ScreenHandlers {
void drawPodcastListScreen(DisplayController& display, const AppState& state);
}  // namespace ScreenHandlers

#endif  // PODCAST_LIST_SCREEN_H
