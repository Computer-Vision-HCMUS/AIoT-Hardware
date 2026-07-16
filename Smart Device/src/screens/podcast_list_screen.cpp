/**
 * @file podcast_list_screen.cpp
 * @brief PODCAST_LIST screen renderer — scrollable list of 6+ episodes
 *
 * AI-recommended episodes render with distinct cyan color + [AI] badge.
 *
 * Button legend:
 *   MODE(1)   = BACK
 *   ACTION(2) = --
 *   START(3)  = --
 *   NEXT(4)   = DOWN
 *   BACK(5)   = UP / BACK
 */

#include "screens/podcast_list_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {

void drawPodcastListScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    const auto episodes = getRecommendedPodcast();
    const uint8_t total = (uint8_t)episodes.size();

    UICommon::drawScreenFrame(display, "Podcast", "AI Recommendations");

    constexpr uint8_t kVisible = 4;
    const uint16_t startY = 50;
    const uint16_t itemH  = 46;
    const uint16_t W      = display.getWidth();

    uint8_t winStart = 0;
    if (state.podcastScrollIndex >= kVisible) {
        winStart = state.podcastScrollIndex - kVisible + 1;
    }

    for (uint8_t vi = 0; vi < kVisible; ++vi) {
        uint8_t idx = winStart + vi;
        if (idx >= total) break;

        const PodcastEpisode& ep = episodes[idx];
        bool     selected = (idx == state.podcastScrollIndex);
        uint16_t y        = startY + vi * itemH;

        if (selected) {
            display.setColor(25, 55, 95);
            display.drawRectangle(0, y, W, itemH, true);
            display.setColor(80, 160, 255);
            display.drawRectangle(0, y, 4, itemH, true);
        } else {
            display.setColor(14, 20, 30);
            display.drawRectangle(0, y, W, itemH, true);
        }

        // AI badge
        if (ep.isAiRecommended) {
            display.setColor(180, 130, 0);
            display.drawRectangle(W - 26, y + 6, 22, 12, true);
            display.setColor(255, 240, 0);
            display.drawText(W - 24, y + 8, "AI", 1);

            display.setColor(0, 220, 200);  // Distinct cyan for AI-recommended
        } else {
            display.setColor(selected ? 255 : 200,
                             selected ? 255 : 200,
                             selected ? 255 : 200);
        }

        display.drawText(8, y + 5, ep.title, 1);

        char buf[48];
        snprintf(buf, sizeof(buf), "%s  %s", ep.creator.c_str(), ep.duration.c_str());
        display.setColor(100, 120, 145);
        display.drawText(8, y + 21, buf, 1);
    }

    if (total > 0) {
        char info[24];
        snprintf(info, sizeof(info), "%u / %u", state.podcastScrollIndex + 1, total);
        UICommon::drawLabel(display, W - 36, 34, info, 1, 100, 130, 160);
    }

    UICommon::drawButtonLegend(display,
        /*S1*/ "BACK",
        /*S2*/ "--",
        /*S3*/ "--",
        /*S4*/ "DOWN",
        /*S5*/ "UP/BCK");
}

}  // namespace ScreenHandlers
