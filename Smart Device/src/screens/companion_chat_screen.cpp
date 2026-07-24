/**
 * @file companion_chat_screen.cpp
 * @brief COMPANION_CHAT screen renderer
 *
 * Shows chat bubbles. Recording state shows "REC" indicator and timer.
 * NOTE: Recording is UI simulation only — no real microphone or audio.
 *
 * Button legend:
 *   MODE(1)   = RECORD
 *   ACTION(2) = STOP
 *   START(3)  = --
 *   NEXT(4)   = --
 *   BACK(5)   = BACK
 */

#include "screens/companion_chat_screen.h"
#include "ui_common.h"
#include <Arduino.h>
#include <cstdio>

namespace ScreenHandlers {

// Draw a single chat bubble
// sender == "user" → right-aligned blue bubble
// sender == "ai"   → left-aligned teal bubble
static void drawChatBubble(DisplayController& display,
                            const std::string& sender,
                            const std::string& text,
                            uint16_t y) {
    const uint16_t W    = display.getWidth();
    const uint16_t bubW = 170;
    const uint16_t bubH = 28;

    if (sender == "user") {
        // Right-aligned, blue
        uint16_t x = W - bubW - 4;
        display.setColor(30, 80, 160);
        display.drawRectangle(x, y, bubW, bubH, true);
        display.setColor(200, 225, 255);
        display.drawText(x + 5, y + 8, text, 1);
    } else {
        // Left-aligned, teal
        display.setColor(20, 90, 80);
        display.drawRectangle(4, y, bubW, bubH, true);
        display.setColor(160, 230, 220);
        display.drawText(9, y + 8, text, 1);
    }
}

void drawCompanionChatScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Companion", "AI Chat");

    const auto& ctx    = state.sharedContext;
    const uint16_t W   = display.getWidth();
    uint16_t chatAreaY = 50;

    if (ctx.chatHistory.empty()) {
        // Welcome message from AI
        UICommon::drawLabel(display, 8, 55, "Welcome!", 1, 100, 200, 180);
        drawChatBubble(display, "ai", "Press RECORD to speak.", chatAreaY + 20);
    } else {
        // Render last 3 messages to fit display
        const uint8_t kMaxVisible = 3;
        uint8_t total  = (uint8_t)ctx.chatHistory.size();
        uint8_t startI = (total > kMaxVisible) ? total - kMaxVisible : 0;

        for (uint8_t i = startI; i < total; ++i) {
            const ChatMessage& msg = ctx.chatHistory[i];
            uint16_t y = chatAreaY + (i - startI) * 36;
            drawChatBubble(display, msg.sender, msg.text, y);
        }
    }

    // ---- Recording state indicator ----
    if (ctx.companionSending) {
        UICommon::drawLabel(display, 8, 175, "Thinking... please wait", 1, 255, 190, 40);
        UICommon::drawButtonLegend(display,
            /*S1*/ "--", /*S2*/ "--", /*S3*/ "WAIT", /*S4*/ "--", /*S5*/ "WAIT");
    } else if (ctx.isRecording) {
        uint32_t elapsed = (millis() - ctx.recordingStartMs) / 1000;
        char buf[32];
        snprintf(buf, sizeof(buf), "REC  %02u/20s", elapsed > 20 ? 20 : elapsed);

        // Red pulsing REC indicator bar
        display.setColor(180, 0, 0);
        display.drawRectangle(0, 175, W, 20, true);
        display.setColor(255, 80, 80);
        display.drawText(6, 179, buf, 1);

        // Animated wave dots
        static uint8_t wavePhase = 0;
        wavePhase = (wavePhase + 1) % 4;
        for (uint8_t d = 0; d < wavePhase + 1; ++d) {
            display.setColor(255, 120, 120);
            display.drawRectangle(110 + d * 12, 180, 8, 8, true);
        }

        UICommon::drawButtonLegend(display,
            /*S1*/ "--",
            /*S2*/ "SEND",
            /*S3*/ "--",
            /*S4*/ "--",
            /*S5*/ "CANCEL");
    } else {
        if (!ctx.companionStatus.empty())
            UICommon::drawLabel(display, 8, 175, ctx.companionStatus, 1, 100, 200, 180);
        UICommon::drawButtonLegend(display,
            /*S1*/ "REC",
            /*S2*/ "--",
            /*S3*/ "--",
            /*S4*/ "--",
            /*S5*/ "BACK");
    }
}

}  // namespace ScreenHandlers
