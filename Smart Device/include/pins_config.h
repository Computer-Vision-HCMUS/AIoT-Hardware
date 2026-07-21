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
// I2S Audio — Speaker (MAX98357 I2S Class-D Amplifier)
// ============================================================================
// Wire MAX98357: BCLK=25  LRC=32  DIN=33  SD_MODE=5  VIN=3.3V  GND=GND
// Leave GAIN pin floating → 12 dB gain (recommended for 3 W speaker)

#define I2S_SPK_PORT        I2S_NUM_0   // ESP32 I2S peripheral for speaker
#define I2S_SPK_BCLK_PIN    25          // Bit Clock → MAX98357 BCLK
#define I2S_SPK_LRCLK_PIN   32          // Word Select → MAX98357 LRC
#define I2S_SPK_DOUT_PIN    33          // Data Out → MAX98357 DIN
#define I2S_SPK_SD_PIN      5           // Shutdown/Mode control (HIGH = ON)

// ============================================================================
// I2S Audio — Microphone (INMP441 I2S MEMS Microphone)
// ============================================================================
// Wire INMP441: SCK=22  WS=21  SD=35  L/R=GND (left ch)  VDD=3.3V  GND=GND
// GPIO 35 is input-only on ESP32 — ideal for mic data output

#define I2S_MIC_PORT        I2S_NUM_1   // ESP32 I2S peripheral for microphone
#define I2S_MIC_SCK_PIN     22          // Bit Clock → INMP441 SCK
#define I2S_MIC_WS_PIN      21          // Word Select → INMP441 WS
#define I2S_MIC_SD_PIN      35          // Data In ← INMP441 SD (input-only GPIO)

// Audio parameters
#define AUDIO_SAMPLE_RATE   16000       // 16 kHz — sufficient for voice
#define AUDIO_DMA_BUF_LEN   512         // DMA buffer length in samples
#define AUDIO_DMA_BUF_COUNT 4           // Number of DMA buffers

#endif // PINS_CONFIG_H