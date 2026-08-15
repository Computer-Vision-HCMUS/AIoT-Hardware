#include "screens/emotion_select_screen.h"

#include "ui_common.h"

#include <cstdio>

namespace ScreenHandlers {
namespace {

constexpr const char* kEmotionLabels[] = {
    "Angry", "Calm", "Disgust", "Fearful", "Happy", "Neutral", "Sad", "Surprised"};

void drawSelectableRow(DisplayController& display, uint16_t y, const char* label,
                       uint8_t probability, bool selected) {
    const uint16_t width = display.getWidth() - 2 * UICommon::kScreenPadding;
    display.setColor(selected ? 28 : 18, selected ? 58 : 26, selected ? 112 : 38);
    display.drawRectangle(UICommon::kScreenPadding, y, width, 38, true);
    if (selected) {
        display.setColor(80, 160, 255);
        display.drawRectangle(UICommon::kScreenPadding, y, 4, 38, true);
    }
    char text[34];
    snprintf(text, sizeof(text), "%s  %u%%", label, probability);
    UICommon::drawLabel(display, UICommon::kScreenPadding + 10, y + 12, text, 1,
                        selected ? 255 : 185, selected ? 255 : 205, selected ? 255 : 225);
}

}  // namespace

void drawEmotionSelectScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Choose emotion", "Select the best match");
    UICommon::drawLabel(display, UICommon::kScreenPadding, 52,
                        "We found these top 3 labels", 1, 160, 200, 240);

    for (uint8_t row = 0; row < 3; ++row) {
        const uint8_t emotionIndex = state.checkInTopEmotionIndices[row];
        drawSelectableRow(display, 76 + row * 43, kEmotionLabels[emotionIndex],
                          state.checkInProbabilities[emotionIndex],
                          row == state.checkInEmotionChoiceIndex);
    }

    UICommon::drawLabel(display, UICommon::kScreenPadding, 210,
                        "Choose what feels most accurate", 1, 120, 165, 210);
    UICommon::drawButtonLegend(display,
        /*S1*/ "--", /*S2*/ "CONFIRM", /*S3*/ "CONFIRM", /*S4*/ "DOWN", /*S5*/ "UP/BACK");
}

void drawPostCheckInMenuScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Check-In saved", "What would you like to do?");
    const char* items[] = {"Discover", "Recommendation", "Companion"};
    const char* descriptions[] = {"Music and podcasts", "Activities for your mood", "Talk with your companion"};
    const uint16_t width = display.getWidth() - 2 * UICommon::kScreenPadding;
    for (uint8_t row = 0; row < 3; ++row) {
        const bool selected = row == state.postCheckInMenuIndex;
        const uint16_t y = 58 + row * 52;
        display.setColor(selected ? 28 : 18, selected ? 58 : 26, selected ? 112 : 38);
        display.drawRectangle(UICommon::kScreenPadding, y, width, 46, true);
        if (selected) {
            display.setColor(80, 160, 255);
            display.drawRectangle(UICommon::kScreenPadding, y, 4, 46, true);
        }
        UICommon::drawLabel(display, UICommon::kScreenPadding + 10, y + 8, items[row], 1,
                            selected ? 255 : 185, selected ? 255 : 205, selected ? 255 : 225);
        UICommon::drawLabel(display, UICommon::kScreenPadding + 10, y + 25, descriptions[row], 1,
                            130, 175, 215);
    }
    UICommon::drawButtonLegend(display,
        /*S1*/ "HOME", /*S2*/ "OPEN", /*S3*/ "OPEN", /*S4*/ "DOWN", /*S5*/ "UP");
}

}  // namespace ScreenHandlers
