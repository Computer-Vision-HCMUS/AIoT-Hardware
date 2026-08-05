/**
 * @file podcast_list_screen.cpp
 * @brief PODCAST_LIST screen renderer — scrollable list of 6+ episodes
 *
 * AI-recommended episodes render with distinct cyan color + [AI] badge.
 *
 * Button legend:
 *   MODE(1)   = BACK
 *   S2 = STOP playback
 *   S3 = PLAY selected server episode
 *   NEXT(4)   = DOWN
 *   BACK(5)   = UP / BACK
 */

#include "screens/podcast_list_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cctype>
#include <cstdio>

namespace ScreenHandlers {

namespace {
std::string truncateForTft(const std::string& text, size_t maxBytes) {
    if (text.size() <= maxBytes) return text;

    size_t end = maxBytes > 3 ? maxBytes - 3 : 0;
    while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
    return text.substr(0, end) + "...";
}

std::string categoryLabel(const std::string& category) {
    if (category.empty()) return "Podcast";

    std::string label = category;
    label[0] = static_cast<char>(toupper(static_cast<unsigned char>(label[0])));
    for (char& ch : label) {
        if (ch == '_') ch = ' ';
    }
    return label;
}
}  // namespace

void drawPodcastListScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    const auto& episodes = getRecommendedPodcast();
    const uint8_t total = (uint8_t)episodes.size();

    UICommon::drawScreenFrame(display, "Podcast", "40 episodes - AI picks first");

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

        display.drawText(8, y + 5,
                         truncateForTft(ep.title, ep.isAiRecommended ? 31 : 37), 1);

        display.setColor(100, 120, 145);
        display.drawText(8, y + 21,
                         truncateForTft(categoryLabel(ep.creator) + " | " + ep.duration, 37), 1);
    }

    if (total > 0) {
        char info[24];
        snprintf(info, sizeof(info), "%u / %u", state.podcastScrollIndex + 1, total);
        UICommon::drawLabel(display, W - 36, 34, info, 1, 100, 130, 160);
    }

    UICommon::drawButtonLegend(display,
        /*S1*/ "BACK",
        /*S2*/ "STOP",
        /*S3*/ "PLAY",
        /*S4*/ "DOWN",
        /*S5*/ "UP/BCK");

    if (!state.sharedContext.mediaPlaying && state.sharedContext.mediaStatus.empty()) {
        UICommon::drawLabel(display, 4, display.getHeight() - 42,
                            "Stopped - choose an item and press PLAY", 1, 100, 130, 160);
    } else if (!state.sharedContext.mediaStatus.empty()) {
        UICommon::drawLabel(display, 4, display.getHeight() - 42,
                            state.sharedContext.mediaStatus.c_str(), 1, 0, 220, 200);
    }
}

}  // namespace ScreenHandlers
