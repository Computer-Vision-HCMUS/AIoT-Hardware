/**
 * @file support_screen.cpp
 * @brief SUPPORT screen renderer
 *
 * Displays server-provided activities and their details.
 *
 * Button legend:
 *   MODE(1)   = HOME
 *   ACTION(2) = OPEN / LIST
 *   START(3)  = OPEN
 *   NEXT(4)   = NEXT
 *   BACK(5)   = PREV / BACK
 */

#include "screens/support_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {
namespace {
constexpr uint8_t kVisibleActivities = 4;

std::string truncateText(const std::string& text, size_t maxBytes) {
    if (text.size() <= maxBytes) return text;

    size_t end = maxBytes;
    // Never split a UTF-8 character; DisplayController converts UTF-8 to the
    // ASCII font supported by the TFT.
    while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) {
        --end;
    }
    return text.substr(0, end) + "...";
}

uint16_t drawWrappedText(DisplayController& display, uint16_t x, uint16_t y,
                         const std::string& text, uint8_t maxBytesPerLine,
                         uint8_t maxLines, uint8_t r, uint8_t g, uint8_t b) {
    std::string line;
    uint8_t lines = 0;
    size_t start = 0;

    while (start < text.size() && lines < maxLines) {
        size_t end = text.find(' ', start);
        const std::string word = text.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        const size_t extra = line.empty() ? word.size() : word.size() + 1;

        if (!line.empty() && line.size() + extra > maxBytesPerLine) {
            UICommon::drawLabel(display, x, y, line, 1, r, g, b);
            y += 14;
            ++lines;
            line.clear();
        }
        if (lines >= maxLines) break;

        if (!line.empty()) line += ' ';
        line += word;
        if (end == std::string::npos) break;
        start = end + 1;
    }

    if (!line.empty() && lines < maxLines) {
        UICommon::drawLabel(display, x, y, line, 1, r, g, b);
        y += 14;
    }
    return y;
}
}  // namespace

void drawSupportScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    const uint8_t total = static_cast<uint8_t>(state.supportActivities.size());

    if (state.supportShowingDetail && state.supportActivityIndex < total) {
        const ActivityCard& activity =
            state.supportActivities[state.supportActivityIndex];
        UICommon::drawScreenFrame(display, "Activity", "Details from server");

        uint16_t y = drawWrappedText(display, 8, 54, activity.title, 31, 2,
                                     240, 240, 240);
        UICommon::drawDivider(display, y + 2);
        drawWrappedText(display, 8, y + 12, activity.description, 34, 10,
                        180, 200, 225);

        UICommon::drawButtonLegend(display,
            /*S1*/ "HOME", /*S2*/ "LIST", /*S3*/ "LIST",
            /*S4*/ "--",   /*S5*/ "LIST");
        return;
    }

    UICommon::drawScreenFrame(display, "Support", "Activities for you");
    char info[32];
    snprintf(info, sizeof(info), "Mood: %s", state.sharedContext.lastEmotion.c_str());
    UICommon::drawLabel(display, 8, 52, info, 1, 100, 200, 120);

    if (total == 0) {
        UICommon::drawLabel(display, 8, 78, "No activities available.", 1,
                            180, 200, 225);
    } else {
        uint8_t windowStart = 0;
        if (state.supportActivityIndex >= kVisibleActivities) {
            windowStart = state.supportActivityIndex - kVisibleActivities + 1;
        }

        for (uint8_t row = 0; row < kVisibleActivities; ++row) {
            const uint8_t index = windowStart + row;
            if (index >= total) break;

            const bool selected = index == state.supportActivityIndex;
            const uint16_t y = 70 + row * 40;
            display.setColor(selected ? 25 : 14, selected ? 55 : 20,
                             selected ? 95 : 30);
            display.drawRectangle(4, y, display.getWidth() - 8, 36, true);
            if (selected) {
                display.setColor(80, 160, 255);
                display.drawRectangle(4, y, 4, 36, true);
            }

            UICommon::drawLabel(display, 14, y + 11,
                                truncateText(state.supportActivities[index].title, 31),
                                1, selected ? 255 : 210, selected ? 255 : 210,
                                selected ? 255 : 210);
        }

        snprintf(info, sizeof(info), "%u / %u",
                 state.supportActivityIndex + 1, total);
        UICommon::drawLabel(display, 196, 52, info, 1, 100, 130, 160);
    }

    UICommon::drawButtonLegend(display,
        /*S1*/ "HOME", /*S2*/ "OPEN", /*S3*/ "OPEN",
        /*S4*/ "NEXT", /*S5*/ "PREV");
}

}  // namespace ScreenHandlers
