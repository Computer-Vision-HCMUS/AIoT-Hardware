/**
 * @file checkin_screen.cpp
 * @brief CHECK_IN screen renderer
 *
 * Phase 1 (checkInAnalyzing == true):  "Listening & analyzing..." state.
 * Phase 2 (checkInAnalyzing == false): shows mock emotion result and confidence.
 *
 * Button legend:
 *   MODE(1)   = --
 *   ACTION(2) = CONFIRM / NEXT
 *   START(3)  = CONFIRM
 *   NEXT(4)   = --
 *   BACK(5)   = CANCEL
 */

#include "screens/checkin_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {

void drawCheckInScreen(DisplayController& display, AppState& state) {
    if (!display.isReady()) return;

    if (state.checkInAnalyzing) {
        // ---- Phase 1: Analyzing ----
        UICommon::drawScreenFrame(display, "Check-In", "Emotion Detection");

        UICommon::drawLabel(display,  8, 55, "Listening & analyzing...", 1, 180, 220, 255);
        UICommon::drawLabel(display,  8, 72, "Please remain calm.", 1, 120, 150, 180);

        // Animated dots drawn via a static counter
        static uint8_t dotCount = 0;
        dotCount = (dotCount + 1) % 4;
        std::string dots(dotCount, '.');
        UICommon::drawLabel(display, 8, 90, "Processing" + dots, 1, 100, 200, 120);

        // Pulsing block visual indicator
        display.setColor(0, 120, 60);
        uint16_t bw = (dotCount + 1) * 30;
        display.drawRectangle(8, 110, bw, 14, true);

        UICommon::drawCard(display, 8, 132, 220, 50,
                           "What to expect",
                           "Press ACTION when ready");

        UICommon::drawButtonLegend(display,
            /*S1*/ "--",
            /*S2*/ "SCAN",
            /*S3*/ "SCAN",
            /*S4*/ "--",
            /*S5*/ "BACK");

    } else {
        // ---- Phase 2: Show result ----
        // Fetch mock emotion result (calls placeholder function)
        const EmotionResult result = runEmotionDetection();
        state.sharedContext.lastEmotion = result.label;
        state.sharedContext.confidence  = result.confidence;

        UICommon::drawScreenFrame(display, "Check-In", "Result");

        UICommon::drawLabel(display, 8, 55, "Detection complete!", 1, 80, 220, 120);

        char buf[48];
        snprintf(buf, sizeof(buf), "Emotion: %s", result.label.c_str());
        UICommon::drawLabel(display, 8, 75, buf, 2, 255, 220, 60);

        snprintf(buf, sizeof(buf), "Confidence: %u%%", result.confidence);
        UICommon::drawLabel(display, 8, 102, buf, 1, 180, 220, 255);

        UICommon::drawCard(display, 8, 122, 220, 50,
                           "Next",
                           "View support recommendation");

        UICommon::drawButtonLegend(display,
            /*S1*/ "--",
            /*S2*/ "NEXT",
            /*S3*/ "NEXT",
            /*S4*/ "--",
            /*S5*/ "BACK");
    }
}

}  // namespace ScreenHandlers
