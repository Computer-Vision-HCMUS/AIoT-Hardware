/**
 * @file music_list_screen.cpp
 * @brief MUSIC_LIST screen renderer — scrollable list of 8+ songs
 *
 * AI-recommended songs render with distinct cyan color + [AI] badge.
 *
 * Button legend:
 *   MODE(1)   = BACK
 *   S2 = PLAY selected server track
 *   S3 = STOP playback
 *   NEXT(4)   = DOWN
 *   BACK(5)   = UP / BACK
 */

#include "screens/music_list_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {

void drawMusicListScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    const auto songs = getRecommendedMusic();
    const uint8_t total = (uint8_t)songs.size();

    UICommon::drawScreenFrame(display, "Music", "AI Recommendations");

    // Scroll window: show 4 items at a time
    constexpr uint8_t kVisible = 4;
    const uint16_t startY = 50;
    const uint16_t itemH  = 46;
    const uint16_t W      = display.getWidth();

    // Compute scroll window start
    uint8_t winStart = 0;
    if (state.musicScrollIndex >= kVisible) {
        winStart = state.musicScrollIndex - kVisible + 1;
    }

    for (uint8_t vi = 0; vi < kVisible; ++vi) {
        uint8_t idx = winStart + vi;
        if (idx >= total) break;

        const Song& song     = songs[idx];
        bool        selected = (idx == state.musicScrollIndex);
        uint16_t    y        = startY + vi * itemH;

        // Row background
        if (selected) {
            display.setColor(25, 55, 95);
            display.drawRectangle(0, y, W, itemH, true);
            display.setColor(80, 160, 255);
            display.drawRectangle(0, y, 4, itemH, true);
        } else {
            display.setColor(14, 20, 30);
            display.drawRectangle(0, y, W, itemH, true);
        }

        // AI-recommended badge + distinct colour
        if (song.isAiRecommended) {
            // Gold [AI] badge background
            display.setColor(180, 130, 0);
            display.drawRectangle(W - 26, y + 6, 22, 12, true);
            display.setColor(255, 240, 0);
            display.drawText(W - 24, y + 8, "AI", 1);

            // Title in bright cyan
            display.setColor(0, 220, 200);
        } else {
            display.setColor(selected ? 255 : 200,
                             selected ? 255 : 200,
                             selected ? 255 : 200);
        }

        // Song title
        display.drawText(8, y + 5, song.title, 1);

        // Artist + duration (muted)
        char buf[48];
        snprintf(buf, sizeof(buf), "%s  %s", song.artist.c_str(), song.duration.c_str());
        display.setColor(100, 120, 145);
        display.drawText(8, y + 21, buf, 1);
    }

    // Scroll position indicator
    if (total > 0) {
        char info[24];
        snprintf(info, sizeof(info), "%u / %u", state.musicScrollIndex + 1, total);
        UICommon::drawLabel(display, W - 36, 34, info, 1, 100, 130, 160);
    }

    UICommon::drawButtonLegend(display,
        /*S1*/ "BACK",
        /*S2*/ "PLAY",
        /*S3*/ "STOP",
        /*S4*/ "DOWN",
        /*S5*/ "UP/BCK");

    if (!state.sharedContext.mediaPlaying && state.sharedContext.mediaStatus.empty()) {
        UICommon::drawLabel(display, 4, display.getHeight() - 42,
                            "Stopped - choose a track and press PLAY", 1, 100, 130, 160);
    } else if (!state.sharedContext.mediaStatus.empty()) {
        UICommon::drawLabel(display, 4, display.getHeight() - 42,
                            state.sharedContext.mediaStatus.c_str(), 1, 0, 220, 200);
    }
}

}  // namespace ScreenHandlers
