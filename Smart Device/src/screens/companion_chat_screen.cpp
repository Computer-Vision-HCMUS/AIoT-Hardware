/**
 * @file companion_chat_screen.cpp
 * @brief Compact, bounded chat UI for the 240x280 TFT.
 */

#include "screens/companion_chat_screen.h"
#include "ui_common.h"

#include <Arduino.h>
#include <cstdio>
#include <string>
#include <vector>

namespace ScreenHandlers {
namespace {

constexpr uint16_t kChatTop = 53;
constexpr uint16_t kChatBottom = 214;
constexpr uint8_t kCharsPerLine = 28;
constexpr uint8_t kMaxLinesPerBubble = 3;

size_t utf8Length(const std::string& text) {
    size_t count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0) != 0x80) ++count;
    }
    return count;
}

std::string truncateUtf8(const std::string& text, size_t maxChars) {
    size_t chars = 0;
    size_t end = 0;
    while (end < text.size() && chars < maxChars) {
        const unsigned char ch = static_cast<unsigned char>(text[end]);
        if ((ch & 0xC0) != 0x80) ++chars;
        ++end;
    }
    return text.substr(0, end);
}

std::vector<std::string> wrapMessage(const std::string& message) {
    std::vector<std::string> lines;
    std::string current;
    std::string word;
    bool clipped = false;

    auto appendWord = [&]() {
        if (word.empty() || clipped) return;
        if (utf8Length(word) > kCharsPerLine) word = truncateUtf8(word, kCharsPerLine - 3) + "...";
        const size_t nextLength = utf8Length(current) + (current.empty() ? 0 : 1) + utf8Length(word);
        if (nextLength <= kCharsPerLine) {
            if (!current.empty()) current += ' ';
            current += word;
        } else {
            if (!current.empty()) lines.push_back(current);
            current = word;
        }
        word.clear();
        if (lines.size() >= kMaxLinesPerBubble) clipped = true;
    };

    for (char ch : message) {
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            appendWord();
        } else {
            word += ch;
        }
    }
    appendWord();
    if (!current.empty() && lines.size() < kMaxLinesPerBubble) lines.push_back(current);
    if (lines.empty()) lines.push_back("...");
    if (clipped || lines.size() > kMaxLinesPerBubble) {
        lines.resize(kMaxLinesPerBubble);
        lines.back() = truncateUtf8(lines.back(), kCharsPerLine - 3) + "...";
    }
    return lines;
}

uint16_t bubbleHeight(const std::vector<std::string>& lines) {
    return static_cast<uint16_t>(18 + lines.size() * 14);
}

void drawChatBubble(DisplayController& display, const ChatMessage& message,
                    const std::vector<std::string>& lines, uint16_t y) {
    const bool isUser = message.sender == "user";
    const uint16_t W = display.getWidth();
    const uint16_t bubbleW = 208;
    const uint16_t bubbleH = bubbleHeight(lines);
    const uint16_t x = isUser ? W - bubbleW - 8 : 8;

    if (isUser) {
        display.setColor(35, 75, 145);
    } else {
        display.setColor(22, 85, 76);
    }
    display.drawRoundedRectangle(x, y, bubbleW, bubbleH, 7, true);

    display.setColor(isUser ? 135 : 100, isUser ? 195 : 210, isUser ? 255 : 185);
    display.drawText(x + 8, y + 4, isUser ? "YOU" : "COMPANION", 1);

    display.setColor(isUser ? 235 : 220, isUser ? 242 : 245, isUser ? 255 : 235);
    for (size_t i = 0; i < lines.size(); ++i) {
        display.drawText(x + 8, y + 18 + i * 14, lines[i], 1);
    }
}

void drawChatHistory(DisplayController& display, const std::vector<ChatMessage>& history) {
    struct PreparedBubble {
        const ChatMessage* message;
        std::vector<std::string> lines;
        uint16_t height;
    };
    std::vector<PreparedBubble> visible;
    uint16_t usedHeight = 0;

    for (auto it = history.rbegin(); it != history.rend() && visible.size() < 3; ++it) {
        PreparedBubble bubble{&*it, wrapMessage(it->text), 0};
        bubble.height = bubbleHeight(bubble.lines);
        const uint16_t gap = visible.empty() ? 0 : 6;
        if (usedHeight + gap + bubble.height > kChatBottom - kChatTop) break;
        usedHeight += gap + bubble.height;
        visible.push_back(std::move(bubble));
    }

    uint16_t y = kChatBottom - usedHeight;
    for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
        drawChatBubble(display, *it->message, it->lines, y);
        y += it->height + 6;
    }
}

void drawStatus(DisplayController& display, const SharedContext& ctx) {
    const uint16_t W = display.getWidth();
    constexpr uint16_t statusY = 220;

    if (ctx.companionSending) {
        display.setColor(80, 65, 20);
        display.drawRoundedRectangle(8, statusY, W - 16, 22, 6, true);
        UICommon::drawLabel(display, 14, statusY + 6, "Companion is thinking...", 1, 255, 205, 70);
        UICommon::drawButtonLegend(display,
            /*S1*/ "--", /*S2*/ "--", /*S3*/ "WAIT", /*S4*/ "--", /*S5*/ "WAIT");
    } else if (ctx.isRecording) {
        const uint32_t elapsed = (millis() - ctx.recordingStartMs) / 1000;
        char label[24];
        snprintf(label, sizeof(label), "REC  %02u/10s", elapsed > 10 ? 10 : elapsed);
        display.setColor(135, 25, 35);
        display.drawRoundedRectangle(8, statusY, W - 16, 22, 6, true);
        display.setColor(255, 220, 225);
        display.drawText(14, statusY + 6, label, 1);
        display.drawText(110, statusY + 6, "Listening...", 1);
        UICommon::drawButtonLegend(display,
            /*S1*/ "--", /*S2*/ "--", /*S3*/ "--", /*S4*/ "--", /*S5*/ "CANCEL");
    } else {
        const std::string status = ctx.companionStatus.empty() ? "Press REC, then speak." : ctx.companionStatus;
        UICommon::drawLabel(display, 14, statusY + 6, truncateUtf8(status, 34), 1, 105, 190, 175);
        UICommon::drawButtonLegend(display,
            /*S1*/ ctx.companionRecordingReady ? "--" : "REC",
            /*S2*/ ctx.companionRecordingReady ? "SEND" : "--",
            /*S3*/ ctx.companionRecordingReady ? "SEND" : "--",
            /*S4*/ "--", /*S5*/ "BACK");
    }
}

}  // namespace

void drawCompanionChatScreen(DisplayController& display, const AppState& state) {
    if (!display.isReady()) return;

    UICommon::drawScreenFrame(display, "Companion", "Voice chat");
    const auto& ctx = state.sharedContext;

    if (ctx.chatHistory.empty()) {
        const ChatMessage welcome{"ai", "Hello. I am here to listen. Press REC to speak.", 0};
        const auto lines = wrapMessage(welcome.text);
        drawChatBubble(display, welcome, lines, kChatTop + 10);
    } else {
        drawChatHistory(display, ctx.chatHistory);
    }
    drawStatus(display, ctx);
}

}  // namespace ScreenHandlers
