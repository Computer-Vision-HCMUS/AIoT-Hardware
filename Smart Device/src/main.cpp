/**
 * @file main.cpp
 * @brief ESP32-S Hardware Demo Entry Point
 * 
 * Initializes the hardware demo and runs the main loop.
 * Demonstrates TFT display capability and button input for hardware verification.
 */

#include <Arduino.h>
#include "demo.h"

/**
 * @brief Setup function - runs once at startup
 */
void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
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