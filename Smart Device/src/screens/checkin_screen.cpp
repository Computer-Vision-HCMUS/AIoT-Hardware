/** @file checkin_screen.cpp @brief Local voice emotion Check-In UI. */

#include "screens/checkin_screen.h"
#include "ui_common.h"
#include <Arduino.h>
#include <cstdio>

namespace ScreenHandlers {
namespace {

void drawProbability(DisplayController& display, uint16_t x, uint16_t y,
                     const char* label, uint8_t probability, uint8_t red,
                     uint8_t green, uint8_t blue) {
    char text[22];
    snprintf(text, sizeof(text), "%s %u%%", label, probability);
    UICommon::drawLabel(display, x, y, text, 1, red, green, blue);
}

}  // namespace

void drawCheckInScreen(DisplayController& display, AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Check-In", "Emotion Detection");

    if (state.checkInAnalyzing) {
        if (state.checkInProcessing) {
            UICommon::drawLabel(display, UICommon::kScreenPadding, 55, "Processing voice emotion...", 1, 255, 190, 40);
            UICommon::drawLabel(display, UICommon::kScreenPadding, 75, "Please wait", 2, 180, 220, 255);
            static uint8_t dots = 0;
            dots = (dots + 1) % 4;
            char waitText[20];
            snprintf(waitText, sizeof(waitText), "SER%s", dots == 0 ? "" : dots == 1 ? "." : dots == 2 ? ".." : "...");
            UICommon::drawLabel(display, UICommon::kScreenPadding, 108, waitText, 1, 100, 200, 120);
            UICommon::drawButtonLegend(display,
                /*S1*/ "WAIT", /*S2*/ "WAIT", /*S3*/ "WAIT", /*S4*/ "WAIT", /*S5*/ "WAIT");
            return;
        } else if (state.checkInRecording) {
            const uint32_t elapsed = (millis() - state.checkInRecordingStartMs) / 1000;
            char timer[24];
            snprintf(timer, sizeof(timer), "REC  %02u/10s", elapsed > 10 ? 10 : elapsed);
            UICommon::drawLabel(display, UICommon::kScreenPadding, 55, timer, 2, 255, 90, 90);
            UICommon::drawLabel(display, UICommon::kScreenPadding, 83, "Say how you feel today.", 1, 180, 220, 255);
        } else {
            UICommon::drawLabel(display, UICommon::kScreenPadding, 55, "Voice emotion check-in", 1, 180, 220, 255);
            UICommon::drawLabel(display, UICommon::kScreenPadding, 73, state.checkInStatus, 1, 120, 150, 180);
        }

        static uint8_t pulse = 0;
        pulse = (pulse + 1) % 4;
        display.setColor(0, 120, 60);
        display.drawRectangle(UICommon::kScreenPadding, 102, (pulse + 1) * 45, 14, true);
        UICommon::drawCard(display, UICommon::kScreenPadding, 132,
                           display.getWidth() - 2 * UICommon::kScreenPadding, 43,
                           "On-device SER",
                           "PCM -> MFCC -> emotion session");

        UICommon::drawButtonLegend(display,
            /*S1*/ state.checkInRecording ? "--" : "REC",
            /*S2*/ "EXEC",
            /*S3*/ "EXEC",
            /*S4*/ "--",
            /*S5*/ "BACK");
        return;
    }

    UICommon::drawScreenFrame(display, "Check-In", state.checkInConfirmed ? "Confirmed" : "RF probabilities");

    char buf[48];
    const std::string& displayedEmotion = state.checkInConfirmed
        ? state.sharedContext.lastEmotion : state.checkInDetectedEmotion;
    const uint8_t displayedConfidence = state.checkInConfirmed
        ? state.sharedContext.confidence : state.checkInDetectedConfidence;
    snprintf(buf, sizeof(buf), "TOP: %s", displayedEmotion.c_str());
    UICommon::drawLabel(display, UICommon::kScreenPadding, 55, buf, 2, 255, 220, 60);
    snprintf(buf, sizeof(buf), "Confidence: %u%%", displayedConfidence);
    UICommon::drawLabel(display, UICommon::kScreenPadding, 80, buf, 1, 180, 220, 255);
    UICommon::drawDivider(display, 96);

    const auto& probability = state.checkInProbabilities;
    drawProbability(display, 12, 106, "ANGRY",   probability[0], 255, 110, 90);
    drawProbability(display, 126, 106, "HAPPY",  probability[4], 255, 210, 70);
    drawProbability(display, 12, 126, "CALM",    probability[1], 100, 210, 160);
    drawProbability(display, 126, 126, "NEUTRAL", probability[5], 150, 190, 255);
    drawProbability(display, 12, 146, "DISGUST", probability[2], 210, 135, 235);
    drawProbability(display, 126, 146, "SAD",    probability[6], 120, 165, 255);
    drawProbability(display, 12, 166, "FEAR",    probability[3], 255, 150, 100);
    drawProbability(display, 126, 166, "SURPRISE", probability[7], 255, 230, 120);
    if (state.checkInUncertain && !state.checkInConfirmed)
        UICommon::drawLabel(display, UICommon::kScreenPadding, 190, "Uncertain - confirm if correct", 1, 255, 180, 50);
    else
        UICommon::drawLabel(display, UICommon::kScreenPadding, 190, state.checkInStatus, 1, 120, 180, 255);

    UICommon::drawCard(display, UICommon::kScreenPadding, 210,
                       display.getWidth() - 2 * UICommon::kScreenPadding, 35,
                       state.checkInConfirmed ? "Support" : "Confirm",
                       state.checkInConfirmed ? "Press S2/S3 for support" : "Press S2/S3 to save");
    UICommon::drawButtonLegend(display,
        /*S1*/ "--",
        /*S2*/ state.checkInConfirmed ? "SUPPORT" : "CONFIRM",
        /*S3*/ state.checkInConfirmed ? "SUPPORT" : "CONFIRM",
        /*S4*/ "--", /*S5*/ "BACK");
}

}  // namespace ScreenHandlers
