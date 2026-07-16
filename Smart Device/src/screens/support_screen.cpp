/**
 * @file support_screen.cpp
 * @brief SUPPORT screen renderer
 *
 * Displays the detected emotion + recommended activity card.
 *
 * Button legend:
 *   MODE(1)   = HOME
 *   ACTION(2) = HOME
 *   START(3)  = --
 *   NEXT(4)   = --
 *   BACK(5)   = BACK
 */

#include "screens/support_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {

void drawSupportScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Support", "Activity for you");

    // Detected emotion summary
    char buf[48];
    snprintf(buf, sizeof(buf), "Mood: %s  (%u%%)",
             state.sharedContext.lastEmotion.c_str(),
             state.sharedContext.confidence);
    UICommon::drawLabel(display, 8, 52, buf, 1, 100, 200, 120);

    // Fetch recommended activity (mock placeholder)
    const ActivityCard card = getRecommendedActivity(state.sharedContext.lastEmotion);

    // Activity card
    UICommon::drawCard(display, 8, 68, 222, 80, card.title, card.description);

    // Additional context
    UICommon::drawLabel(display, 8, 158, "Try this activity to feel better.", 1, 140, 160, 185);

    UICommon::drawButtonLegend(display,
        /*S1*/ "HOME",
        /*S2*/ "HOME",
        /*S3*/ "--",
        /*S4*/ "--",
        /*S5*/ "BACK");
}

}  // namespace ScreenHandlers
