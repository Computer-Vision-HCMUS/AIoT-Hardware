/**
 * @file ui_common.cpp
 * @brief Shared UI rendering primitives for EmotiCare UI Shell (spec 005)
 */

#include "ui_common.h"
#include "display_controller.h"
#include <Arduino.h>
#include <cstdio>

namespace UICommon {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {
void fillRect(DisplayController& display,
              uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    display.drawRectangle(x, y, w, h, true);
}
}  // namespace

// ---------------------------------------------------------------------------
// drawLabel — coloured text at (x, y)
// ---------------------------------------------------------------------------
void drawLabel(DisplayController& display,
               uint16_t x, uint16_t y,
               const std::string& text,
               uint8_t fontSize,
               uint8_t r, uint8_t g, uint8_t b) {
    display.setColor(r, g, b);
    display.drawText(x, y, text, fontSize);
}

// ---------------------------------------------------------------------------
// drawDivider — thin horizontal line
// ---------------------------------------------------------------------------
void drawDivider(DisplayController& display, uint16_t y) {
    display.setColor(60, 70, 90);
    fillRect(display, 0, y, display.getWidth(), 1);
}

// ---------------------------------------------------------------------------
// drawCard — dark-background two-line info card
// ---------------------------------------------------------------------------
void drawCard(DisplayController& display,
              uint16_t x, uint16_t y, uint16_t width, uint16_t height,
              const std::string& title, const std::string& detail) {
    // Card background
    display.setColor(30, 38, 52);
    fillRect(display, x, y, width, height);

    // Left accent bar
    display.setColor(80, 160, 255);
    fillRect(display, x, y, 3, height);

    // Title
    display.setColor(240, 240, 240);
    display.drawText(x + 7, y + 5, title, 1);

    // Detail
    display.setColor(160, 170, 185);
    display.drawText(x + 7, y + 19, detail, 1);
}

// ---------------------------------------------------------------------------
// drawButtonLegend — always-visible footer showing S1..S5 button functions
//
// Physical buttons:  S1=MODE  S2=ACTION  S3=START  S4=NEXT  S5=BACK
//
// Layout on 240×280 display (30px footer at bottom):
//   Row 1 (top):    S1:<func>  S2:<func>  S3:<func>
//   Row 2 (bottom): S4:<func>       S5:<func>
// ---------------------------------------------------------------------------
void drawButtonLegend(DisplayController& display,
                      const std::string& modeLabel,
                      const std::string& actionLabel,
                      const std::string& startLabel,
                      const std::string& nextLabel,
                      const std::string& backLabel) {
    const uint16_t W       = display.getWidth();   // 240
    const uint16_t H       = display.getHeight();  // 280
    const uint16_t footerH = 30;
    const uint16_t footerY = H - footerH;

    // Footer background
    display.setColor(12, 18, 28);
    fillRect(display, 0, footerY, W, footerH);

    // Top separator line
    display.setColor(45, 70, 110);
    fillRect(display, 0, footerY, W, 1);

    // --- Row 1: S1 | S2 | S3 ---
    const uint16_t row1Y = footerY + 3;
    // S1 (MODE) — left
    display.setColor(80, 150, 220);
    display.drawText(2, row1Y, "S1", 1);
    display.setColor(60, 80, 100);
    display.drawText(14, row1Y, ":", 1);
    display.setColor(200, 220, 255);
    display.drawText(19, row1Y, modeLabel, 1);

    // S2 (ACTION) — center-left
    display.setColor(80, 150, 220);
    display.drawText(82, row1Y, "S2", 1);
    display.setColor(60, 80, 100);
    display.drawText(94, row1Y, ":", 1);
    display.setColor(200, 220, 255);
    display.drawText(99, row1Y, actionLabel, 1);

    // S3 (START) — right
    display.setColor(80, 150, 220);
    display.drawText(162, row1Y, "S3", 1);
    display.setColor(60, 80, 100);
    display.drawText(174, row1Y, ":", 1);
    display.setColor(200, 220, 255);
    display.drawText(179, row1Y, startLabel, 1);

    // --- Row 2: S4 | S5 ---
    const uint16_t row2Y = footerY + 16;
    // S4 (NEXT) — left-center
    display.setColor(80, 200, 120);
    display.drawText(12, row2Y, "S4", 1);
    display.setColor(60, 80, 100);
    display.drawText(24, row2Y, ":", 1);
    display.setColor(180, 240, 180);
    display.drawText(29, row2Y, nextLabel, 1);

    // S5 (BACK) — right-center
    display.setColor(80, 200, 120);
    display.drawText(132, row2Y, "S5", 1);
    display.setColor(60, 80, 100);
    display.drawText(144, row2Y, ":", 1);
    display.setColor(180, 240, 180);
    display.drawText(149, row2Y, backLabel, 1);
}

// ---------------------------------------------------------------------------
// drawHintBar — legacy single-string hint (kept for compatibility)
// ---------------------------------------------------------------------------
void drawHintBar(DisplayController& display, const std::string& hintText) {
    const uint16_t H      = display.getHeight();
    const uint16_t barY   = H - 28;
    const uint16_t barH   = 28;

    display.setColor(15, 20, 30);
    fillRect(display, 0, barY, display.getWidth(), barH);

    display.setColor(50, 80, 120);
    fillRect(display, 0, barY, display.getWidth(), 1);

    display.setColor(140, 210, 255);
    display.drawText(4, barY + 7, hintText, 1);
}

// ---------------------------------------------------------------------------
// drawScreenFrame — clear screen, draw title + subtitle + divider
// Caller must then call drawButtonLegend() with screen-specific labels.
// ---------------------------------------------------------------------------
void drawScreenFrame(DisplayController& display,
                     const std::string& title,
                     const std::string& subtitle) {
    // Background
    display.setBackgroundColor(10, 15, 25);
    display.clear();

    // Title (large)
    display.setColor(255, 255, 255);
    display.drawText(6, 6, title, 2);

    // Subtitle (small, muted)
    display.setColor(140, 170, 210);
    display.drawText(6, 30, subtitle, 1);

    // Divider below header
    drawDivider(display, 45);
}

}  // namespace UICommon
