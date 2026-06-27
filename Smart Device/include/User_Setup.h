/**
 * @file User_Setup.h
 * @brief TFT_eSPI library configuration for mhetesp32devkit
 * 
 * Configures pins and settings for TFT_eSPI to work with our hardware setup.
 */

#ifndef USER_SETUP_H
#define USER_SETUP_H

// ============================================================================
// Driver selection
// ============================================================================
// Uncomment one driver
//#define ILI9341_DRIVER       // 240x320 SPI display
#define ST7789_DRIVER
// ============================================================================
// SPI Port Selection (for ESP32)
// ============================================================================
// Use HSPI port (standard for ESP32)
#define USE_HSPI_PORT

// ============================================================================
// Pin Configuration
// ============================================================================
// Define pins based on our pins_config.h

#define TFT_CS    15   // Chip select control pin (GPIO15)
#define TFT_RST   16   // Reset pin (GPIO16)
#define TFT_DC    17   // Data Command control pin (GPIO17)

#define TFT_MOSI  23   // SPI MOSI (GPIO23)
#define TFT_SCLK  18   // SPI Clock (GPIO18)
#define TFT_MISO  19   // SPI MISO (GPIO19) - optional for display

// ============================================================================
// Display Settings
// ============================================================================
#define TFT_INIT_DELAY 0
#define CGRAM_OFFSET 
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Rotation: 0 = no rotation, 1 = 90° CW, 2 = 180°, 3 = 270° CW
#define TFT_ROTATION 0

// ============================================================================
// Font Selection
// ============================================================================
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5743 bytes in FLASH, 96 characters

// ============================================================================
// Color Settings
// ============================================================================
#define TFT_BL   -1  // No backlight pin configured
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
