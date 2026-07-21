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

// ============================================================================
// I2S Microphone Pin Configuration (INMP441 or compatible MEMS mic)
// ============================================================================
// TODO(hardware): confirm actual wiring with schematic before flashing
// These are placeholder assignments — adjust to match physical board.

#define I2S_MIC_SCK_PIN     32   // Serial Clock (BCLK)
#define I2S_MIC_WS_PIN      33   // Word Select  (LRCLK)
#define I2S_MIC_SD_PIN      34   // Serial Data  (DOUT from mic → ESP32 input)

// I2S driver settings
#define I2S_PORT_NUM        I2S_NUM_0
#define I2S_SAMPLE_RATE     16000   // 16 kHz — optimal for Whisper STT
#define I2S_SAMPLE_BITS     16      // 16-bit PCM
#define I2S_CHANNEL_FMT     I2S_CHANNEL_FMT_ONLY_LEFT  // mono mic
#define I2S_DMA_BUF_COUNT   4
#define I2S_DMA_BUF_LEN     256

// Audio capture settings
#define AUDIO_MAX_RECORD_MS 8000    // max 8 seconds per recording
#define AUDIO_BUFFER_BYTES  (I2S_SAMPLE_RATE * (I2S_SAMPLE_BITS / 8) * (AUDIO_MAX_RECORD_MS / 1000))
//                          = 16000 * 2 * 8 = 256 000 bytes (~256 KB)

#endif // PINS_CONFIG_H
