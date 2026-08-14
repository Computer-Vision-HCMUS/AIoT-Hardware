#include "screens/button_test_screen.h"
#include "ui_common.h"
#include <cstdio>

namespace ScreenHandlers {
void drawButtonTestScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;
    UICommon::drawScreenFrame(display, "Test Buttons", "Press each physical button");
    static constexpr const char* kNames[] = {"S1 MODE", "S2 ACTION", "S3 START", "S4 NEXT", "S5 BACK"};
    for (uint8_t i = 0; i < 5; ++i) {
        const uint16_t y = 55 + i * 31;
        const bool active = state.sharedContext.buttonPressed[i];
        display.setColor(active ? 30 : 20, active ? 120 : 35, active ? 70 : 55);
        display.drawRectangle(UICommon::kScreenPadding, y, display.getWidth() - 2 * UICommon::kScreenPadding, 25, true);
        display.setColor(active ? 120 : 170, active ? 255 : 190, active ? 160 : 220);
        display.drawText(UICommon::kScreenPadding + 6, y + 8, kNames[i], 1);
        char count[20];
        snprintf(count, sizeof(count), "#%lu", static_cast<unsigned long>(state.sharedContext.buttonPressCounts[i]));
        display.drawText(183, y + 8, count, 1);
    }
    char last[24];
    snprintf(last, sizeof(last), "Last input: S%u", state.sharedContext.lastButtonId + 1);
    UICommon::drawLabel(display, UICommon::kScreenPadding, 220, last, 1, 120, 180, 255);
    UICommon::drawButtonLegend(display, "EXIT", "TEST", "TEST", "TEST", "TEST");
}
}  // namespace ScreenHandlers
