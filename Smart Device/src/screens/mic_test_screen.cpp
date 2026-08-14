/**
 * @file mic_test_screen.cpp
 * @brief MIC_TEST screen renderer
 *
 * Draws a live VU meter driven by AudioManager::getPeakLevel() stored in
 * AppState.sharedContext.micPeakLevel (updated by demo_app each frame).
 *
 * Passthrough is managed externally by demo_app.cpp:
 *   - Enter MIC_TEST → AudioManager::startPassthrough()
 *   - Leave MIC_TEST → AudioManager::stopPassthrough()
 *
 * Button legend:
 *   S1–S4 = --  (no action while in test)
 *   S5(BACK) = BACK
 */

#include "screens/mic_test_screen.h"
#include "ui_common.h"
#include <cstdio>

namespace ScreenHandlers {

void drawMicTestScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    const uint16_t W = display.getWidth();   // 240

    // ── Background & Title ──
    display.setBackgroundColor(8, 12, 20);
    display.clear();

    display.setColor(255, 255, 255);
    display.drawText(UICommon::kScreenPadding, UICommon::kScreenTitleTop, "Test Mic", 2);

    UICommon::drawDivider(display, 28);

    display.setColor(130, 150, 180);
    display.drawText(UICommon::kScreenPadding, 33, "Speak into mic — hear it live", 1);

    UICommon::drawDivider(display, 46);

    // ── VU Meter ──
    const uint16_t meterX = UICommon::kScreenPadding;
    const uint16_t meterY = 62;
    const uint16_t meterW = W - 2 * UICommon::kScreenPadding;
    const uint16_t meterH = 32;

    const uint16_t peak = state.sharedContext.micPeakLevel;  // 0–100

    // Label + percentage
    display.setColor(120, 145, 175);
    display.drawText(meterX, 52, "Volume", 1);

    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%3u%%", peak);
    display.setColor(200, 215, 240);
    display.drawText(W - UICommon::kScreenPadding - 26, 52, pctBuf, 1);

    // Meter background (dark rail)
    display.setColor(22, 32, 48);
    display.drawRectangle(meterX, meterY, meterW, meterH, true);

    // Filled bar — color shifts green→yellow→red with level
    if (peak > 0) {
        const uint16_t fillW = static_cast<uint16_t>(
            static_cast<uint32_t>(meterW) * peak / 100u);

        uint8_t r, g, b;
        if      (peak < 60) { r =  40; g = 210; b =  80; }  // Green
        else if (peak < 80) { r = 230; g = 180; b =   0; }  // Yellow
        else                { r = 255; g =  50; b =  30; }  // Red (clipping)

        display.setColor(r, g, b);
        display.drawRectangle(meterX, meterY, fillW, meterH, true);
    }

    // Tick marks at 25%, 50%, 75%
    for (uint8_t t = 25; t <= 75; t += 25) {
        const uint16_t tx = meterX + static_cast<uint16_t>(
            static_cast<uint32_t>(meterW) * t / 100u);
        display.setColor(10, 18, 28);
        display.drawRectangle(tx, meterY, 2, meterH, true);
    }

    // Meter border (subtle)
    display.setColor(40, 60, 88);
    display.drawRectangle(meterX, meterY, meterW, meterH, false);

    // ── Status info ──
    const bool active = state.sharedContext.audioActive;

    display.setColor(active ? 80 : 140,
                     active ? 220 : 150,
                     active ? 110 : 160);
    display.drawText(UICommon::kScreenPadding, 104, active ? "Status : Listening..." : "Status : Starting...", 1);

    display.setColor(70, 90, 115);
    display.drawText(UICommon::kScreenPadding, 120, "Mic    : INMP441  (I2S1 / GPIO 22,21,35)", 1);
    display.drawText(UICommon::kScreenPadding, 136, "Amp    : MAX98357 (I2S0 / GPIO 25,32,33)", 1);
    display.drawText(UICommon::kScreenPadding, 152, "Rate   : 16 kHz  |  Depth : 16-bit", 1);
    display.drawText(UICommon::kScreenPadding, 168, "Latency: ~32 ms per DMA buffer", 1);

    // ── Button legend ──
    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ "--",
        /*S3*/ "--",
        /*S4*/ "--",
        /*S5*/ "BACK");
}

}  // namespace ScreenHandlers
