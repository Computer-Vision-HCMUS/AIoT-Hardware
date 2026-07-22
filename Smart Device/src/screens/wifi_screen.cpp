/**
 * @file wifi_screen.cpp
 * @brief WiFi Setup screen — single ON/OFF toggle.
 *
 * Toggle ON  → connect to saved WiFi credentials
 * Toggle OFF → disconnect, open AP provisioning portal (captive portal)
 *              Phone/PC connecting to EmotiCare-Setup will be auto-redirected
 *              to the setup form via captive portal detection.
 *
 * Button map:
 *   ACTION(1) = flip the toggle
 *   BACK(4)   = return to previous screen
 */

#include "screens/wifi_screen.h"
#include "ui_common.h"
#include <cstdio>

namespace ScreenHandlers {

void drawWifiScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    // ── Background & title ────────────────────────────────────────────────
    display.setBackgroundColor(10, 15, 25);
    display.clear();

    display.setColor(255, 255, 255);
    display.drawText(6, 6, "WiFi Setup", 2);
    UICommon::drawDivider(display, 28);

    // ── Determine current WiFi state ──────────────────────────────────────
    const std::string& status = state.sharedContext.deviceStatus;
    // Toggle is ON when connected to external WiFi (Online / Unpaired)
    // Toggle is OFF when in AP provisioning mode (Setup AP) or Offline
    const bool wifiOn = (status == "Online" || status == "Unpaired");

    // ── Status label ──────────────────────────────────────────────────────
    display.setColor(130, 145, 170);
    display.drawText(6, 36, "Current:", 1);
    if (wifiOn) {
        display.setColor(80, 200, 100);
        display.drawText(60, 36, status.c_str(), 1);
    } else {
        display.setColor(255, 160, 0);
        display.drawText(60, 36, status.c_str(), 1);
    }

    UICommon::drawDivider(display, 52);

    // ── Toggle widget ─────────────────────────────────────────────────────
    // Draw a big pill-shaped toggle in the center of the screen
    const uint16_t W       = display.getWidth();   // 240
    const uint16_t pillW   = 120;
    const uint16_t pillH   = 44;
    const uint16_t pillX   = (W - pillW) / 2;
    const uint16_t pillY   = 72;
    const uint16_t knobSize = 36;

    // Pill background
    if (wifiOn) {
        display.setColor(30, 140, 70);   // green when ON
    } else {
        display.setColor(60, 65, 80);    // dark grey when OFF
    }
    display.drawRectangle(pillX, pillY, pillW, pillH, true);

    // Knob — right side when ON, left side when OFF
    const uint16_t knobX = wifiOn
        ? (pillX + pillW - knobSize - 4)
        : (pillX + 4);
    const uint16_t knobY = pillY + (pillH - knobSize) / 2;

    display.setColor(230, 235, 245);
    display.drawRectangle(knobX, knobY, knobSize, knobSize, true);

    // ON / OFF label inside pill
    if (wifiOn) {
        display.setColor(200, 240, 210);
        display.drawText(pillX + 10, pillY + 15, "ON", 1);
    } else {
        display.setColor(140, 145, 160);
        display.drawText(pillX + pillW - 28, pillY + 15, "OFF", 1);
    }

    // ── Description text ──────────────────────────────────────────────────
    display.setColor(100, 110, 130);
    const uint16_t descY = pillY + pillH + 14;
    if (wifiOn) {
        display.drawText(6, descY,      "Toggle OFF to switch WiFi", 1);
        display.drawText(6, descY + 14, "or configure a new network.", 1);
    } else {
        display.drawText(6, descY,      "Connect phone to:", 1);
        display.setColor(255, 200, 60);
        display.drawText(6, descY + 14, "EmotiCare-Setup", 1);
        display.setColor(100, 110, 130);
        display.drawText(6, descY + 28, "Setup page opens automatically.", 1);
    }

    // ── Button legend ─────────────────────────────────────────────────────
    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ "Toggle",
        /*S3*/ "--",
        /*S4*/ "--",
        /*S5*/ "Back");
}

}  // namespace ScreenHandlers
