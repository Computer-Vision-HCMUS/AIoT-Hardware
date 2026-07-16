/**
 * @file User_Setup.h
 * @brief TFT_eSPI library configuration for mhetesp32devkit
 * 
 * Configures pins and settings for TFT_eSPI to work with our hardware setup.
 */

#ifndef USER_SETUP_H
#define USER_SETUP_H

#include "pins_config.h"

// ============================================================================
// Driver selection
// =============================================================================
#define ST7789_2_DRIVER
// ============================================================================
// SPI Port Selection (for ESP32)
// ============================================================================
// Use HSPI port (standard for ESP32)
#define USE_HSPI_PORT

// ============================================================================
// Pin Configuration
// ============================================================================
// Define pins based on our pins_config.h

#define TFT_CS    DISPLAY_CS_PIN
#define TFT_RST   DISPLAY_RST_PIN
#define TFT_DC    DISPLAY_DC_PIN

#define TFT_MOSI  DISPLAY_MOSI_PIN
#define TFT_SCLK  DISPLAY_CLK_PIN
#define TFT_MISO  DISPLAY_MISO_PIN

// ============================================================================
// Display Settings
// ============================================================================
#define TFT_INIT_DELAY 0
#define CGRAM_OFFSET 
#define TFT_WIDTH  DISPLAY_WIDTH
#define TFT_HEIGHT DISPLAY_HEIGHT

// Rotation: 0 = no rotation, 1 = 90° CW, 2 = 180°, 3 = 270° CW
#define TFT_ROTATION DISPLAY_ROTATION

// ============================================================================
// Font Selection
// ============================================================================
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5743 bytes in FLASH, 96 characters

// ============================================================================
// Color Settings
// ============================================================================
#define TFT_BL   DISPLAY_BLK_PIN
#define TFT_BACKLIGHT_ON HIGH

// ============================================================================
// SPI Speed
// ============================================================================
// SPI clock speed in MHz - higher is faster but may cause issues
#define SPI_FREQUENCY   40000000  // 40 MHz - stable for display

// ============================================================================
// Prevent TFT_eSPI from trying to detect display
// ============================================================================
// We've manually defined the driver above

#endif // USER_SETUP_H
