/**
 * @file pins_config.h
 * @brief Hardware pin configuration for ESP32-S demo
 * 
 * Centralizes all GPIO and hardware pin assignments for the hardware demo.
 * Application code must use these symbolic names rather than numeric pin values.
 * This preserves board portability and eases review per AIoT-Hardware Constitution Principle I.
 */

#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <cstdint>

// ============================================================================
// Display (TFT_eSPI) Pin Configuration
// ============================================================================
// These pins are wired to the TFT display controller via SPI

#define DISPLAY_CS_PIN      15   // Chip Select
#define DISPLAY_RST_PIN     16   // Reset
#define DISPLAY_DC_PIN      17   // Data/Command
#define DISPLAY_MOSI_PIN    23   // SPI MOSI (Master Out Slave In)
#define DISPLAY_MISO_PIN    19   // SPI MISO (Master In Slave Out)
#define DISPLAY_CLK_PIN     18   // SPI Clock
#define DISPLAY_BLK_PIN     4    // Backlight (GPIO4) - thử pin này, điều chỉnh nếu cần

// Display resolution and configuration (hardcoded for mhetesp32devkit)
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      280
#define DISPLAY_ROTATION    0    // 0: normal, 1: 90°, 2: 180°, 3: 270°

// ============================================================================
// Button Pin Configuration
// ============================================================================
// Physical buttons for the EmotiCare navigation contract.
// Adjust these values to match the actual board wiring if your hardware uses different pins.

#define MODE_PIN            12   // MODE button
#define ACTION_PIN          13   // ACTION button
#define START_PIN           14   // START button
#define NEXT_PIN            27   // NEXT button
#define BACK_PIN            26   // BACK button

// Backward-compatible aliases for the earlier two-button demo
#define BUTTON_A_PIN        MODE_PIN
#define BUTTON_B_PIN        ACTION_PIN

// Button debounce timing
#define DEBOUNCE_TIME_MS    100   // 100ms debounce delay (aggressive)
#define DEBOUNCE_SAMPLES    15    // Require 15 stable samples (very aggressive)

// ============================================================================
// Interrupt Configuration
// ============================================================================
// GPIO interrupt settings for button detection

#define BUTTON_INTR_TYPE    GPIO_INTR_NEGEDGE  // Trigger on falling edge (active low)

// ============================================================================
// Status Indicators (optional LED pins if available)
// ============================================================================
// Future expansion for status LEDs

#define STATUS_LED_PIN      2    // GPIO2 (if used for status indication)

// ============================================================================
// UART/Serial Configuration
// ============================================================================
// For debug logging

#define DEBUG_BAUD_RATE     115200

#endif // PINS_CONFIG_H
