/**
 * @file insights_screen.cpp
 * @brief INSIGHTS screen renderer — emotion statistics by time period
 *
 * Shows period selector (Day / Week / Month) and emotion distribution.
 *
 * Button legend:
 *   MODE(1)   = --
 *   ACTION(2) = NEXT PERIOD
 *   START(3)  = PREV PERIOD
 *   NEXT(4)   = NEXT PERIOD
 *   BACK(5)   = BACK
 */

#include "screens/insights_screen.h"
#include "ui_common.h"
#include "service.h"
#include <cstdio>

namespace ScreenHandlers {

static const char* kPeriods[] = { "Day", "Week", "Month" };
constexpr uint8_t  kPeriodCount = 3;

// Draw a text-based percentage bar
// e.g. "Happy   [========  ] 60%"
static void drawStatBar(DisplayController& display,
                         const char* label,
                         uint8_t pct,
                         uint16_t y,
                         uint8_t r, uint8_t g, uint8_t b) {
    display.setColor(180, 180, 200);
    display.drawText(6, y, label, 1);

    // Bar background
    const uint16_t barX = 68;
    const uint16_t barW = 120;
    const uint16_t barH = 9;
    display.setColor(30, 38, 52);
    display.drawRectangle(barX, y + 1, barW, barH, true);

    // Filled portion
    uint16_t fillW = (uint16_t)((uint32_t)barW * pct / 100);
    display.setColor(r, g, b);
    display.drawRectangle(barX, y + 1, fillW, barH, true);

    // Percentage text
    char buf[8];
    snprintf(buf, sizeof(buf), "%3u%%", pct);
    display.setColor(200, 210, 230);
    display.drawText(barX + barW + 4, y, buf, 1);
}

void drawInsightsScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    uint8_t pidx = state.sharedContext.insightsPeriodIndex % kPeriodCount;
    const char* period = kPeriods[pidx];

    UICommon::drawScreenFrame(display, "Insights", "Emotion Breakdown");

    // Period selector strip
    const uint16_t W    = display.getWidth();
    const uint16_t psY  = 48;
    const uint16_t psH  = 18;
    uint16_t colW = W / kPeriodCount;
    for (uint8_t i = 0; i < kPeriodCount; ++i) {
        bool sel = (i == pidx);
        display.setColor(sel ? 40 : 18, sel ? 80 : 25, sel ? 140 : 40);
        display.drawRectangle(i * colW, psY, colW, psH, true);
        if (sel) {
            display.setColor(80, 160, 255);
            display.drawRectangle(i * colW, psY, colW, 2, true);
        }
        display.setColor(sel ? 255 : 130, sel ? 255 : 140, sel ? 255 : 160);
        display.drawText(i * colW + 6, psY + 4, kPeriods[i], 1);
    }

    // Fetch stats for the selected period
    const EmotionDistribution dist = getStatisticsByPeriod(period);

    // Emotion stat bars
    uint16_t baseY = 74;
    uint16_t rowH  = 18;
    drawStatBar(display, "Happy  ", dist.happyPct,   baseY,             255, 200,  60);
    drawStatBar(display, "Calm   ", dist.calmPct,    baseY + rowH,      60,  200, 120);
    drawStatBar(display, "Focused", dist.focusedPct, baseY + rowH * 2,  80,  160, 255);
    drawStatBar(display, "Sad    ", dist.sadPct,     baseY + rowH * 3,  100, 130, 200);
    drawStatBar(display, "Anxious", dist.anxiousPct, baseY + rowH * 4,  200, 100, 100);

    // Summary note
    char summary[48];
    snprintf(summary, sizeof(summary), "Period: %s  |  AI-analyzed", dist.period.c_str());
    UICommon::drawLabel(display, 6, 167, summary, 1, 90, 110, 140);

    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ "NEXT",
        /*S3*/ "PREV",
        /*S4*/ "NEXT",
        /*S5*/ "BACK");
}

}  // namespace ScreenHandlers
