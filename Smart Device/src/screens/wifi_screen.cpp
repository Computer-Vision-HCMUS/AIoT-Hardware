/**
 * @file wifi_screen.cpp
 * @brief WiFi setup guide and connection action.
 */

#include "screens/wifi_screen.h"
#include "ui_common.h"

namespace ScreenHandlers {

void drawWifiScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    display.setBackgroundColor(10, 15, 25);
    display.clear();
    display.setColor(255, 255, 255);
    display.drawText(6, 6, "WiFi Setup", 2);
    UICommon::drawDivider(display, 28);

    const std::string& status = state.sharedContext.deviceStatus;
    const bool wifiOn = (status == "Online" || status == "Unpaired");

    display.setColor(130, 145, 170);
    display.drawText(6, 36, "Status:", 1);
    display.setColor(wifiOn ? 80 : 255, wifiOn ? 200 : 160, wifiOn ? 100 : 0);
    display.drawText(54, 36, status.c_str(), 1);
    UICommon::drawDivider(display, 52);

    const uint16_t y = 68;
    display.setColor(100, 110, 130);
    if (wifiOn) {
        display.drawText(6, y, "Wi-Fi is connected.", 1);
        display.drawText(6, y + 18, "Press S2 to change Wi-Fi.", 1);
        display.drawText(6, y + 32, "A setup hotspot will open.", 1);
    } else if (status == "Setup AP") {
        display.drawText(6, y, "1. Connect phone to:", 1);
        display.setColor(255, 200, 60);
        display.drawText(6, y + 16, "EmotiCare-Setup", 1);
        display.setColor(100, 110, 130);
        display.drawText(6, y + 32, "2. Password: 12345678", 1);
        display.drawText(6, y + 46, "3. Open: 192.168.4.1", 1);
        display.drawText(6, y + 64, "S2: retry saved Wi-Fi", 1);
    } else {
        display.drawText(6, y, "No Wi-Fi connection.", 1);
        display.drawText(6, y + 18, "Press S2 to open setup.", 1);
    }

    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ wifiOn ? "Change Wi-Fi" : "Connect",
        /*S3*/ "--",
        /*S4*/ "--",
        /*S5*/ "Back");
}

}  // namespace ScreenHandlers
