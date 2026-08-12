/**
 * @file ui_common.cpp
 * @brief Shared UI rendering primitives for EmotiCare UI Shell (spec 005)
 */

#include "ui_common.h"
#include "display_controller.h"
#include <Arduino.h>
#include <algorithm>
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

std::string compactLegendLabel(const std::string& label, size_t maxChars) {
    if (label.size() <= maxChars) return label;
    if (maxChars <= 3) return label.substr(0, maxChars);
    return label.substr(0, maxChars - 3) + "...";
}

void drawLegendItem(DisplayController& display,
                    uint16_t x, uint16_t y, uint16_t cellWidth,
                    const char* button, const std::string& label,
                    uint8_t labelR, uint8_t labelG, uint8_t labelB) {
    static constexpr uint16_t kLabelOffset = 20;
    static constexpr uint16_t kGlyphWidth = 6;

    display.setColor(80, 170, 220);
    display.drawText(x, y, std::string(button) + ":", 1);

    const size_t maxChars = cellWidth > kLabelOffset
        ? (cellWidth - kLabelOffset) / kGlyphWidth
        : 0;
    display.setColor(labelR, labelG, labelB);
    display.drawText(x + kLabelOffset, y,
                     compactLegendLabel(label, maxChars), 1);
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
    display.drawText(std::max<uint16_t>(x, kScreenPadding), y, text, fontSize);
}

// ---------------------------------------------------------------------------
// drawDivider — thin horizontal line
// ---------------------------------------------------------------------------
void drawDivider(DisplayController& display, uint16_t y) {
    display.setColor(60, 70, 90);
    fillRect(display, kScreenPadding, y,
             display.getWidth() - 2 * kScreenPadding, 1);
}

// ---------------------------------------------------------------------------
// drawCard — dark-background two-line info card
// ---------------------------------------------------------------------------
void drawCard(DisplayController& display,
              uint16_t x, uint16_t y, uint16_t width, uint16_t height,
              const std::string& title, const std::string& detail) {
    // Card background
    display.setColor(30, 38, 52);
    display.drawRoundedRectangle(x, y, width, height, 6, true);

    // Left accent bar
    display.setColor(80, 160, 255);
    fillRect(display, x + 2, y + 4, 3, height > 8 ? height - 8 : height);

    // Title
    display.setColor(240, 240, 240);
    display.drawText(x + 9, y + 5, title, 1);

    // Detail
    display.setColor(160, 170, 185);
    display.drawText(x + 9, y + 19, detail, 1);
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
    const uint16_t footerH = 34;
    const uint16_t footerY = H - footerH;
    const uint16_t usableW = W - 2 * kScreenPadding;

    // Footer background
    display.setColor(12, 18, 28);
    fillRect(display, kScreenPadding, footerY,
             usableW, footerH - kFrameInset);

    // Top separator line
    display.setColor(45, 70, 110);
    fillRect(display, kScreenPadding, footerY, usableW, 1);

    // --- Row 1: S1 | S2 | S3 ---
    const uint16_t row1Y = footerY + 4;
    const uint16_t row1CellW = usableW / 3;
    drawLegendItem(display, kScreenPadding, row1Y, row1CellW,
                   "S1", modeLabel, 200, 220, 255);
    drawLegendItem(display, kScreenPadding + row1CellW, row1Y, row1CellW,
                   "S2", actionLabel, 200, 220, 255);
    drawLegendItem(display, kScreenPadding + row1CellW * 2, row1Y,
                   usableW - row1CellW * 2,
                   "S3", startLabel, 200, 220, 255);

    // --- Row 2: S4 | S5 ---
    const uint16_t row2Y = footerY + 15;
    const uint16_t row2CellW = usableW / 2;
    drawLegendItem(display, kCornerTextPadding, row2Y,
                   kScreenPadding + row2CellW - kCornerTextPadding,
                   "S4", nextLabel, 180, 240, 180);
    drawLegendItem(display, kScreenPadding + row2CellW, row2Y,
                   usableW - row2CellW,
                   "S5", backLabel, 180, 240, 180);

    // Draw last so full-width row backgrounds cannot cover the safe-area edge.
    drawScreenBorder(display);
}

// ---------------------------------------------------------------------------
// drawHintBar — legacy single-string hint (kept for compatibility)
// ---------------------------------------------------------------------------
void drawHintBar(DisplayController& display, const std::string& hintText) {
    const uint16_t H      = display.getHeight();
    const uint16_t barY   = H - 28;
    const uint16_t barH   = 28;

    display.setColor(15, 20, 30);
    fillRect(display, kScreenPadding, barY,
             display.getWidth() - 2 * kScreenPadding, barH - kFrameInset);

    display.setColor(50, 80, 120);
    fillRect(display, kScreenPadding, barY,
             display.getWidth() - 2 * kScreenPadding, 1);

    display.setColor(140, 210, 255);
    display.drawText(kCornerTextPadding, barY + 7, hintText, 1);

    drawScreenBorder(display);
}

void drawScreenBorder(DisplayController& display) {
    const uint16_t width = display.getWidth();
    const uint16_t height = display.getHeight();
    if (width <= 2 * kFrameInset || height <= 2 * kFrameInset) return;

    display.setColor(55, 95, 145);
    display.drawRoundedRectangle(kFrameInset, kFrameInset,
                                 width - 2 * kFrameInset,
                                 height - 2 * kFrameInset,
                                 kFrameRadius, false);
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

    drawScreenBorder(display);

    // Title (large)
    display.setColor(255, 255, 255);
    display.drawText(kCornerTextPadding, 10, title, 2);

    // Subtitle (small, muted)
    display.setColor(140, 170, 210);
    display.drawText(kScreenPadding, 32, subtitle, 1);

    // Divider below header
    drawDivider(display, 47);
}

}  // namespace UICommon
