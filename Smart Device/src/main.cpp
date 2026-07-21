/**
 * @file main.cpp
 * @brief ESP32-S Hardware Demo Entry Point
 * 
 * Initializes the hardware demo and runs the main loop.
 * Demonstrates TFT display capability and button input for hardware verification.
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include "pins_config.h"
#include "demo.h"

/**
 * @brief Phát 1 tiếng sine 440Hz kéo dài 5 giây qua loa để test loa nhanh.
 * LƯU Ý: AudioManager::init() (gọi bên trong demo_init()) đã cài driver
 * I2S cho I2S_SPK_PORT từ trước — hàm này KHÔNG được cài driver lần nữa,
 * chỉ việc bật ampli rồi ghi thẳng dữ liệu vào port đã có sẵn.
 */
void playBootChime() {
  Serial.println("[Chime] Playing boot tone (5s)...");

  pinMode(I2S_SPK_SD_PIN, OUTPUT);
  digitalWrite(I2S_SPK_SD_PIN, HIGH);  // bật ampli MAX98357

  // Sinh sóng sine 440Hz (nốt La)
  const int bufLen = 512;
  int16_t buf[bufLen];
  for (int i = 0; i < bufLen; i++) {
    buf[i] = (int16_t)(6000 * sin(2 * PI * 440 * i / AUDIO_SAMPLE_RATE));
  }

  size_t written;
  int loops = (AUDIO_SAMPLE_RATE * 5) / bufLen;  // ~5 giây
  for (int i = 0; i < loops; i++) {
    esp_err_t err = i2s_write(I2S_SPK_PORT, buf, sizeof(buf), &written, portMAX_DELAY);
    if (err != ESP_OK) {
      Serial.printf("[Chime] i2s_write error: %d\n", err);
      break;
    }
  }

  digitalWrite(I2S_SPK_SD_PIN, LOW);  // tắt ampli, tránh rè khi im lặng

  Serial.println("[Chime] Done.");
}

/**
 * @brief Setup function - runs once at startup
 */
void setup() {
  // Initialize serial for debugging
  Serial.begin(DEBUG_BAUD_RATE);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(100);

  Serial.println("\n\n================================");
  Serial.println("AIoT Hardware Demo Starting...");
  Serial.println("================================\n");

  // Initialize the hardware demo
  if (!demo_init()) {
    Serial.println("ERROR: Demo initialization failed!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("Demo initialized successfully!");

  playBootChime();  // <-- test: phát tiếng 5 giây sau khi setup xong
}

/**
 * @brief Main loop function - runs repeatedly
 */
void loop() {
  // Update demo (handles display rendering and button input)
  if (demo_update()) {
    // Demo running normally
    Serial.flush();  // Ensure debug logs are sent
    delay(50);  // 1 second between updates for stability
  } else {
    // Demo error - restart
    Serial.println("ERROR: Demo update failed - restarting!");
    Serial.flush();
    demo_stop();
    delay(50);
    
    if (!demo_init()) {
      Serial.println("ERROR: Demo restart failed!");
      Serial.flush();
      while (1) {
        delay(50);
      }
    }
  }
}