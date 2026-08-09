/**
 * @file display_controller.cpp
 * @brief TFT Display implementation for hardware demo
 */

#include "display_controller.h"
#include "pins_config.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <algorithm>
#include <cstdio>

// Global TFT instance for TFT_eSPI library
static TFT_eSPI tft = TFT_eSPI();

namespace {
constexpr uint16_t kTextLeftInset = 14;
constexpr uint16_t kTextRightInset = 14;
constexpr uint16_t kTextTopInset = 10;
constexpr uint16_t kTextBottomInset = 10;

uint32_t readUtf8Codepoint(const std::string& text, size_t& index) {
    const uint8_t first = static_cast<uint8_t>(text[index++]);
    if (first < 0x80) return first;

    uint8_t remaining = first < 0xE0 ? 1 : (first < 0xF0 ? 2 : 3);
    uint32_t codepoint = first & (0x7F >> remaining);
    while (remaining-- && index < text.size()) {
        const uint8_t next = static_cast<uint8_t>(text[index++]);
        if ((next & 0xC0) != 0x80) return '?';
        codepoint = (codepoint << 6) | (next & 0x3F);
    }
    return codepoint;
}

char latinFallback(uint32_t codepoint) {
    switch (codepoint) {
        case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: case 0x00C4:
        case 0x00C5: case 0x0102: case 0x1EA0: case 0x1EA2: case 0x1EA4:
        case 0x1EA6: case 0x1EA8: case 0x1EAA: case 0x1EAC: case 0x1EAE:
        case 0x1EB0: case 0x1EB2: case 0x1EB4: case 0x1EB6: return 'A';
        case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4:
        case 0x00E5: case 0x0103: case 0x1EA1: case 0x1EA3: case 0x1EA5:
        case 0x1EA7: case 0x1EA9: case 0x1EAB: case 0x1EAD: case 0x1EAF:
        case 0x1EB1: case 0x1EB3: case 0x1EB5: case 0x1EB7: return 'a';
        case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB: case 0x1EBA:
        case 0x1EBC: case 0x1EB8: case 0x1EBE:
        case 0x1EC0: case 0x1EC2: case 0x1EC4: case 0x1EC6: return 'E';
        case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: case 0x1EB9:
        case 0x1EBB: case 0x1EBD: case 0x1EBF: case 0x1EC1: case 0x1EC3:
        case 0x1EC5: case 0x1EC7: return 'e';
        case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF: case 0x0128:
        case 0x1ECA: case 0x1EC8: return 'I';
        case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: case 0x0129:
        case 0x1ECB: case 0x1EC9: return 'i';
        case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: case 0x00D6:
        case 0x00D8: case 0x01A0: case 0x1ECC: case 0x1ECE: case 0x1ED0:
        case 0x1ED2: case 0x1ED4: case 0x1ED6: case 0x1ED8: case 0x1EDA:
        case 0x1EDC: case 0x1EDE: case 0x1EE0: case 0x1EE2: return 'O';
        case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6:
        case 0x00F8: case 0x01A1: case 0x1ECD: case 0x1ECF: case 0x1ED1:
        case 0x1ED3: case 0x1ED5: case 0x1ED7: case 0x1ED9: case 0x1EDB:
        case 0x1EDD: case 0x1EDF: case 0x1EE1: case 0x1EE3: return 'o';
        case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC: case 0x0168:
        case 0x01AF: case 0x1EE4: case 0x1EE6: case 0x1EE8: case 0x1EEA:
        case 0x1EEC: case 0x1EEE: case 0x1EF0: return 'U';
        case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: case 0x0169:
        case 0x01B0: case 0x1EE5: case 0x1EE7: case 0x1EE9: case 0x1EEB:
        case 0x1EED: case 0x1EEF: case 0x1EF1: return 'u';
        case 0x00DD: case 0x0178: case 0x1EF2: case 0x1EF6: case 0x1EF8:
        case 0x1EF4: return 'Y';
        case 0x00FD: case 0x00FF: case 0x1EF3: case 0x1EF7: case 0x1EF9:
        case 0x1EF5: return 'y';
        case 0x0110: return 'D';
        case 0x0111: return 'd';
        case 0x00B7: return ' ';
        default: return '?';
    }
}

std::string toTftText(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (size_t index = 0; index < text.size();) {
        const uint32_t codepoint = readUtf8Codepoint(text, index);
        result += codepoint < 0x80 ? static_cast<char>(codepoint)
                                   : latinFallback(codepoint);
    }
    return result;
}

std::string fitTextToWidth(std::string text, int16_t maxWidth) {
    if (maxWidth <= 0 || text.empty()) return {};
    if (tft.textWidth(text.c_str()) <= maxWidth) return text;

    static constexpr const char* kEllipsis = "...";
    const int16_t ellipsisWidth = tft.textWidth(kEllipsis);
    if (ellipsisWidth > maxWidth) {
        while (!text.empty() && tft.textWidth(text.c_str()) > maxWidth) {
            text.pop_back();
        }
        return text;
    }

    while (!text.empty()) {
        text.pop_back();
        if (tft.textWidth(text.c_str()) + ellipsisWidth <= maxWidth) {
            text += kEllipsis;
            return text;
        }
    }
    return kEllipsis;
}
}  // namespace

// ============================================================================
// DisplayController Implementation
// ============================================================================

DisplayController::DisplayController()
    : initialized_(false),
      width_(DISPLAY_WIDTH),
      height_(DISPLAY_HEIGHT),
      text_color_r_(255), text_color_g_(255), text_color_b_(255),
      bg_color_r_(0), bg_color_g_(0), bg_color_b_(0) {}

bool DisplayController::init() {
    if (initialized_) {
        return true;
    }

    try {
        Serial.println("[Display] Initializing TFT display...");
        
        // Enable backlight first
        Serial.println("[Display] Enabling backlight...");
        pinMode(DISPLAY_BLK_PIN, OUTPUT);
        digitalWrite(DISPLAY_BLK_PIN, HIGH);  // BLK active HIGH
        delay(200);  // Increased delay for backlight to stabilize
        Serial.println("[Display] Backlight enabled");
        
        // Initialize TFT display
        tft.init();
        Serial.println("[Display] TFT init() completed");
        delay(200);  // Extra delay after init
        
        // Set rotation (0-3 for different orientations)
        tft.setRotation(DISPLAY_ROTATION);
        tft.fillScreen(TFT_BLACK);   //clear
        delay(100);
        tft.fillScreen(TFT_BLACK);  
        delay(100);
        Serial.printf("[Display] Rotation set to %d\n", DISPLAY_ROTATION);
        delay(100);
        
        // Set white color and draw a test rectangle to verify display works
        tft.fillRect(10, 10, 50, 50, TFT_WHITE);
        Serial.println("[Display] Test white rectangle drawn at (10,10)");
        delay(500);  // Show test pattern briefly
        
        // Clear again for actual rendering
        tft.fillScreen(TFT_BLACK);
        delay(150);
        tft.fillScreen(TFT_BLACK);
        Serial.println("[Display] Screen cleared again");
        delay(100);
        
        // Initialize default colors
        text_color_r_ = 255;
        text_color_g_ = 255;
        text_color_b_ = 255;
        bg_color_r_ = 0;
        bg_color_g_ = 0;
        bg_color_b_ = 0;

        initialized_ = true;
        Serial.println("[Display] Display initialized successfully!");
        return true;
    } catch (...) {
        Serial.println("[Display] ERROR: Display initialization failed!");
        initialized_ = false;
        return false;
    }
}

void DisplayController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    text_color_r_ = r;
    text_color_g_ = g;
    text_color_b_ = b;
    uint16_t color = tft.color565(r, g, b);
    tft.setTextColor(color);
}

void DisplayController::setBackgroundColor(uint8_t r, uint8_t g, uint8_t b) {
    bg_color_r_ = r;
    bg_color_g_ = g;
    bg_color_b_ = b;
}

void DisplayController::drawText(uint16_t x, uint16_t y, const std::string& text, uint8_t fontSize) {
    if (!initialized_) {
        return;
    }

    // Clamp font size to valid range (1-3 for stability)
    if (fontSize < 1) fontSize = 1;
    if (fontSize > 3) fontSize = 3;  // Limit to 3

    // Keep text on one line and inside the rounded screen frame. Long dynamic
    // values are shortened instead of wrapping into the footer or border.
    tft.setTextSize(fontSize);
    tft.setTextWrap(false, false);

    const uint16_t safeX = std::max<uint16_t>(x, kTextLeftInset);
    const uint16_t textHeight = static_cast<uint16_t>(tft.fontHeight());
    const uint16_t maxY = height_ > kTextBottomInset + textHeight
        ? height_ - kTextBottomInset - textHeight
        : kTextTopInset;
    const uint16_t safeY = std::max<uint16_t>(y, kTextTopInset);
    if (safeY > maxY) return;
    if (safeX >= width_ - kTextRightInset) return;

    // TFT_eSPI's built-in GLCD font is ASCII-only. Convert Vietnamese UTF-8
    // to readable Latin characters instead of rendering each UTF-8 byte as a
    // corrupt glyph.
    const int16_t maxWidth = static_cast<int16_t>(width_ - kTextRightInset - safeX);
    const std::string displayText = fitTextToWidth(toTftText(text), maxWidth);
    if (displayText.empty()) return;

    tft.setCursor(safeX, safeY);
    tft.print(displayText.c_str());
    delay(2);
}

void DisplayController::drawTextRightAligned(uint16_t rightX, uint16_t y,
                                             const std::string& text,
                                             uint8_t fontSize) {
    if (!initialized_) return;
    if (fontSize < 1) fontSize = 1;
    if (fontSize > 3) fontSize = 3;

    tft.setTextSize(fontSize);
    const uint16_t safeRight = std::min<uint16_t>(
        rightX, width_ - kTextRightInset);
    if (safeRight <= kTextLeftInset) return;

    const int16_t availableWidth = static_cast<int16_t>(safeRight - kTextLeftInset);
    const std::string displayText = fitTextToWidth(toTftText(text), availableWidth);
    if (displayText.empty()) return;

    const int16_t textWidth = tft.textWidth(displayText.c_str());
    const uint16_t x = safeRight - static_cast<uint16_t>(textWidth);
    drawText(x, y, displayText, fontSize);
}

void DisplayController::drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool filled) {
    if (!initialized_) return;

    uint16_t color = tft.color565(text_color_r_, text_color_g_, text_color_b_);

    if (filled) {
        tft.fillRect(x, y, width, height, color);
    } else {
        tft.drawRect(x, y, width, height, color);
    }
}

void DisplayController::drawRoundedRectangle(uint16_t x, uint16_t y,
                                             uint16_t width, uint16_t height,
                                             uint16_t radius, bool filled) {
    if (!initialized_ || width == 0 || height == 0) return;

    const uint16_t maxRadius = static_cast<uint16_t>(
        std::min<uint16_t>(width, height) / 2U);
    radius = std::min(radius, maxRadius);
    const uint16_t color = tft.color565(text_color_r_, text_color_g_, text_color_b_);

    if (filled) {
        tft.fillRoundRect(x, y, width, height, radius, color);
    } else {
        tft.drawRoundRect(x, y, width, height, radius, color);
    }
}

void DisplayController::clear() {
    if (!initialized_) return;

    uint16_t bg_color = tft.color565(bg_color_r_, bg_color_g_, bg_color_b_);
    tft.fillScreen(bg_color);
    delay(30);  // Ensure clear completes before next operation
}

void DisplayController::update() {
    // TFT_eSPI typically updates immediately
    // This method is here for compatibility if buffering is added later
}

uint16_t DisplayController::getWidth() const {
    return width_;
}

uint16_t DisplayController::getHeight() const {
    return height_;
}

bool DisplayController::isReady() const {
    return initialized_;
}

void DisplayController::initSPI() {
    // SPI initialization handled by TFT_eSPI library
}

void DisplayController::sendCommand(uint8_t cmd) {
    // Low-level command sending handled by TFT_eSPI
}

void DisplayController::sendData(uint8_t data) {
    // Low-level data sending handled by TFT_eSPI
}
