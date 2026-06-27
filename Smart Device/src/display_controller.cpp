/**
 * @file display_controller.cpp
 * @brief TFT Display implementation for hardware demo
 */

#include "display_controller.h"
#include "pins_config.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <cstdio>

// Global TFT instance for TFT_eSPI library
static TFT_eSPI tft = TFT_eSPI();

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
        tft.writecommand(0x36);   // MADCTL
        tft.writedata(0x00);
        tft.setAddrWindow(0, 80, 240, 320);
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

    // Set text size and position
    tft.setTextSize(fontSize);
    tft.setCursor(x, y);

    // Draw the text using print (more stable than println)
    tft.print(text.c_str());  // Use print instead of println
    delay(2);
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
