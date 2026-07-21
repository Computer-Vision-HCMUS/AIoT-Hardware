/**
 * @file home_screen.cpp
 * @brief HOME screen renderer
 *
 * Layout:
 *   - Header: "EmotiCare"
 *   - Status strip: WiFi status + current emotion (None if not yet checked-in)
 *   - Menu: 4 items with UP/DOWN highlight cursor
 *   - Footer: button legend
 *
 * Button legend:
 *   S1(MODE)   = --
 *   S2(ACTION) = OK
 *   S3(START)  = OK
 *   S4(NEXT)   = DOWN
 *   S5(BACK)   = UP
 */

#include "screens/home_screen.h"
#include "ui_common.h"
#include <cstdio>

namespace ScreenHandlers {

void drawHomeScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    // ---- Background & title ----
    display.setBackgroundColor(10, 15, 25);
    display.clear();

    display.setColor(255, 255, 255);
    display.drawText(6, 6, "EmotiCare", 2);

    UICommon::drawDivider(display, 28);

    // ---- Status strip ----
    // Network status is supplied by NetworkManager (Online / Setup AP / Offline).
    const bool online = state.sharedContext.deviceStatus == "Online";
    display.setColor(online ? 80 : 210, online ? 200 : 150, online ? 100 : 70);
    display.drawText(6, 33, "WiFi", 1);
    display.setColor(50, 70, 90);
    display.drawText(30, 33, "|", 1);
    display.setColor(online ? 100 : 240, online ? 220 : 180, online ? 120 : 90);
    display.drawText(38, 33, state.sharedContext.deviceStatus, 1);

    // Separator between WiFi and emotion
    display.setColor(50, 70, 90);
    display.drawText(88, 33, "|", 1);

    // Current emotion
    display.setColor(160, 170, 190);
    display.drawText(96, 33, "Mood:", 1);

    if (state.sharedContext.lastEmotion.empty() ||
        state.sharedContext.lastEmotion == "Neutral") {
        // Not yet checked in
        display.setColor(100, 110, 130);
        display.drawText(134, 33, "None", 1);
    } else {
        // Emotion detected after check-in
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %u%%",
                 state.sharedContext.lastEmotion.c_str(),
                 state.sharedContext.confidence);
        display.setColor(255, 210, 70);
        display.drawText(134, 33, buf, 1);
    }

    UICommon::drawDivider(display, 47);

    // ---- Menu items ----
    static const char* kMenuItems[] = {
        "1. Check-In",
        "2. Discover",
        "3. Companion Chat",
        "4. Insights"
    };
    constexpr uint8_t kCount = 4;

    const uint16_t startY = 52;
    const uint16_t itemH  = 32;
    const uint16_t W      = display.getWidth();

    for (uint8_t i = 0; i < kCount; ++i) {
        uint16_t y        = startY + i * itemH;
        bool     selected = (i == state.homeMenuIndex);

        if (selected) {
            display.setColor(28, 58, 112);
            display.drawRectangle(0, y, W, itemH, true);
            display.setColor(80, 160, 255);
            display.drawRectangle(0, y, 4, itemH, true);
            display.setColor(255, 255, 255);
        } else {
            display.setColor(85, 105, 135);
        }

        display.drawText(10, y + 10, kMenuItems[i], 1);
    }

    // ---- Button legend ----
    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ "OK",
        /*S3*/ "OK",
        /*S4*/ "DOWN",
        /*S5*/ "UP");
}

}  // namespace ScreenHandlers
