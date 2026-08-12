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
#include <string>

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
    display.drawText(UICommon::kScreenPadding, y, label, 1);

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

static void drawAiAssessment(DisplayController& display,
                             const std::string& assessment,
                             const char* period) {
    constexpr uint8_t kGlyphWidth = 6;  // built-in font at size 1
    const size_t charsPerLine =
        (display.getWidth() - 2 * UICommon::kScreenPadding) / kGlyphWidth;
    constexpr uint8_t kMaxLines = 10;
    std::string line;
    uint16_t y = 92;
    uint8_t lines = 0;
    size_t start = 0;

    char title[36];
    snprintf(title, sizeof(title), "AI assessment - %s", period);
    UICommon::drawLabel(display, kTextLeft, 74,
                        title, 1, 110, 190, 255);

    while (start < assessment.size() && lines < kMaxLines) {
        while (start < assessment.size() && assessment[start] == ' ') ++start;
        if (start >= assessment.size()) break;
        size_t end = assessment.find(' ', start);
        if (end == std::string::npos) end = assessment.size();
        std::string word = assessment.substr(start, end - start);

        while (word.size() > charsPerLine && lines < kMaxLines) {
            if (!line.empty()) {
                display.setColor(224, 230, 242);
                display.drawText(UICommon::kScreenPadding, y, line.c_str(), 1);
                y += 14;
                ++lines;
                line.clear();
            }
            if (lines >= kMaxLines) break;
            display.setColor(224, 230, 242);
            display.drawText(UICommon::kScreenPadding, y,
                             word.substr(0, charsPerLine).c_str(), 1);
            y += 14;
            ++lines;
            word.erase(0, charsPerLine);
        }
        if (lines >= kMaxLines) break;
        if (!word.empty() && !line.empty() &&
            line.size() + 1 + word.size() > charsPerLine) {
            display.setColor(224, 230, 242);
            display.drawText(UICommon::kScreenPadding, y, line.c_str(), 1);
            y += 14;
            ++lines;
            line.clear();
        }
        if (lines >= kMaxLines) break;
        if (!word.empty()) {
            if (!line.empty()) line += ' ';
            line += word;
        }
        start = end + 1;
    }

    if (!line.empty() && lines < kMaxLines) {
        display.setColor(224, 230, 242);
        display.drawText(UICommon::kScreenPadding, y, line.c_str(), 1);
    }
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

    if (state.sharedContext.insightsShowingAiAssessment) {
        drawAiAssessment(display, state.sharedContext.insightsAiAssessment, period);
        UICommon::drawButtonLegend(display,
            /*S1*/ "CHART",
            /*S2*/ "NEXT",
            /*S3*/ "PREV",
            /*S4*/ "NEXT",
            /*S5*/ "BACK");
        return;
    }

    // Fetch stats for the selected period
    const EmotionDistribution dist = getStatisticsByPeriod(period);

    // Emotion stat bars
    uint16_t baseY = 74;
    uint16_t rowH  = 16;
    drawStatBar(display, "Angry    ", dist.angryPct,     baseY,             230, 95, 85);
    drawStatBar(display, "Calm     ", dist.calmPct,      baseY + rowH,      80, 195, 165);
    drawStatBar(display, "Disgust  ", dist.disgustPct,   baseY + rowH * 2,  145, 115, 185);
    drawStatBar(display, "Fearful  ", dist.fearfulPct,   baseY + rowH * 3,  215, 135, 95);
    drawStatBar(display, "Happy    ", dist.happyPct,     baseY + rowH * 4,  255, 200, 60);
    drawStatBar(display, "Neutral  ", dist.neutralPct,   baseY + rowH * 5,  100, 190, 220);
    drawStatBar(display, "Sad      ", dist.sadPct,       baseY + rowH * 6,  100, 130, 200);
    drawStatBar(display, "Surprise ", dist.surprisedPct, baseY + rowH * 7,  255, 160, 220);

    // Summary note
    char summary[48];
    snprintf(summary, sizeof(summary), "Period: %s  |  AI-analyzed", dist.period.c_str());
    UICommon::drawLabel(display, UICommon::kScreenPadding, 205,
                        summary, 1, 90, 110, 140);

    UICommon::drawButtonLegend(display,
        /*S1*/ "AI VIEW",
        /*S2*/ "NEXT",
        /*S3*/ "PREV",
        /*S4*/ "NEXT",
        /*S5*/ "BACK");
}

}  // namespace ScreenHandlers
