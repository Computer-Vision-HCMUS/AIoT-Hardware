/**
 * @file discover_screen.cpp
 * @brief DISCOVER screen renderer — Music / Podcast selection hub
 *
 * Shows emotion-aware context: "Good for: [emotion]" or "Good for: Neutral (default)"
 * if no check-in has been done yet.
 *
 * Button legend:
 *   S1(MODE)   = --
 *   S2(ACTION) = OK
 *   S3(START)  = OK
 *   S4(NEXT)   = DOWN
 *   S5(BACK)   = UP/BCK
 */

#include "screens/discover_screen.h"
#include "service.h"
#include "ui_common.h"
#include <algorithm>
#include <cstdio>

namespace ScreenHandlers {

void drawDiscoverScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    display.setBackgroundColor(10, 15, 25);
    display.clear();
    UICommon::drawScreenBorder(display);

    // ---- Title ----
    display.setColor(255, 255, 255);
    display.drawText(UICommon::kCornerTextPadding, 10, "Discover", 2);

    UICommon::drawDivider(display, 31);

    // ---- Emotion context line ----
    const bool hasEmotion = !state.sharedContext.lastEmotion.empty() &&
                            state.sharedContext.lastEmotion != "Neutral";

    if (hasEmotion) {
        // Detected emotion — personalised line
        char buf[48];
        snprintf(buf, sizeof(buf), "Good for: %s (%u%%)",
                 state.sharedContext.lastEmotion.c_str(),
                 state.sharedContext.confidence);
        display.setColor(255, 200, 60);
        display.drawText(UICommon::kScreenPadding, 36, buf, 1);

        display.setColor(100, 170, 240);
        display.drawText(UICommon::kScreenPadding, 48, "AI picks tuned to your mood", 1);
    } else {
        // No check-in yet — default neutral line
        display.setColor(130, 150, 175);
        display.drawText(UICommon::kScreenPadding, 36, "Good for: Neutral (default)", 1);

        display.setColor(80, 100, 125);
        display.drawText(UICommon::kScreenPadding, 48, "Do Check-In for mood-based picks", 1);
    }

    UICommon::drawDivider(display, 61);

    // ---- Menu: Music / Podcast ----
    static const char* kItems[] = { "Music", "Podcast" };
    const auto& songs = getRecommendedMusic();
    const auto& episodes = getRecommendedPodcast();
    const uint8_t musicAiCount = static_cast<uint8_t>(std::count_if(
        songs.begin(), songs.end(), [](const Song& song) { return song.isAiRecommended; }));
    const uint8_t podcastAiCount = static_cast<uint8_t>(std::count_if(
        episodes.begin(), episodes.end(),
        [](const PodcastEpisode& episode) { return episode.isAiRecommended; }));
    char subDescriptions[2][48];
    snprintf(subDescriptions[0], sizeof(subDescriptions[0]), "%u tracks | %u AI first",
             static_cast<unsigned>(songs.size()), musicAiCount);
    snprintf(subDescriptions[1], sizeof(subDescriptions[1]), "%u episodes | %u AI first",
             static_cast<unsigned>(episodes.size()), podcastAiCount);
    constexpr uint8_t kCount = 2;

    const uint16_t startY = 66;
    const uint16_t itemH  = 48;
    const uint16_t W      = display.getWidth();

    for (uint8_t i = 0; i < kCount; ++i) {
        uint16_t y        = startY + i * itemH;
        bool     selected = (i == state.discoverIndex);

        if (selected) {
            display.setColor(28, 58, 112);
            display.drawRoundedRectangle(6, y, W - 12, itemH - 2, 6, true);
            display.setColor(80, 160, 255);
            display.drawRectangle(8, y + 4, 3, itemH - 10, true);
            display.setColor(255, 255, 255);
        } else {
            display.setColor(18, 26, 38);
            display.drawRoundedRectangle(6, y, W - 12, itemH - 2, 6, true);
            display.setColor(140, 155, 175);
        }

        display.drawText(UICommon::kScreenPadding, y + 10, kItems[i], 1);

        // Personalised sub-description when selected
        if (selected) {
            char subdesc[64];
            if (hasEmotion) {
                snprintf(subdesc, sizeof(subdesc), "%s for %s",
                         subDescriptions[i],
                         state.sharedContext.lastEmotion.c_str());
            } else {
                snprintf(subdesc, sizeof(subdesc), "%s (neutral default)", subDescriptions[i]);
            }
            display.setColor(140, 200, 255);
            display.drawText(UICommon::kScreenPadding, y + 26, subdesc, 1);
        } else {
            display.setColor(80, 95, 115);
            display.drawText(UICommon::kScreenPadding, y + 26, subDescriptions[i], 1);
        }
    }

    // ---- Button legend ----
    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ "OK",
        /*S3*/ "OK",
        /*S4*/ "DOWN",
        /*S5*/ "UP/BCK");
}

}  // namespace ScreenHandlers
