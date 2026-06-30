#include "ui/demo_rendering.h"

#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_timer.h>
#include <cstdio>

namespace DemoRendering {

void renderWelcomeScreen(DisplayController& display, uint64_t boot_time_ms) {
    if (!display.isReady()) {
        return;
    }

    display.setBackgroundColor(0, 0, 0);
    display.clear();
    delay(20);

    display.setColor(255, 255, 255);
    delay(2);
    display.drawText(30, 20, "AIoT Demo", 2);
    delay(20);

    display.setColor(0, 255, 0);
    delay(2);
    display.drawText(20, 60, "Device: ESP32", 1);
    delay(20);

    display.setColor(0, 150, 255);
    delay(2);
    display.drawText(20, 80, "Status: OK", 1);
    delay(20);

    display.setColor(200, 200, 200);
    delay(2);
    uint64_t uptime_s = (esp_timer_get_time() - boot_time_ms) / 1000000;
    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "Up: %llu s", uptime_s);
    display.drawText(20, 100, uptime_str, 1);
    delay(20);

    display.setColor(100, 100, 100);
    delay(2);
    display.drawText(10, 180, "B: Info", 1);
    delay(20);
}

void renderDeviceInfoScreen(DisplayController& display) {
    if (!display.isReady()) {
        return;
    }

    display.setBackgroundColor(0, 0, 0);
    display.clear();
    delay(20);

    display.setColor(255, 255, 255);
    delay(2);
    display.drawText(30, 20, "Info", 2);
    delay(20);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    display.setColor(0, 255, 0);
    delay(2);
    display.drawText(10, 60, "ESP32", 1);
    delay(20);

    display.setColor(0, 150, 255);
    delay(2);
    char cores_str[32];
    snprintf(cores_str, sizeof(cores_str), "Cores: %d", chip_info.cores);
    display.drawText(10, 80, cores_str, 1);
    delay(20);

    display.setColor(100, 100, 100);
    delay(2);
    display.drawText(10, 180, "B: Back", 1);
    delay(20);
}

void renderErrorScreen(DisplayController& display, const char* error_msg) {
    if (!display.isReady()) {
        return;
    }

    display.setBackgroundColor(100, 20, 20);
    display.clear();
    delay(10);

    display.setColor(255, 100, 100);
    delay(2);
    display.drawText(20, 20, "ERROR", 2);
    delay(10);

    display.setColor(255, 255, 255);
    delay(2);
    display.drawText(10, 60, error_msg, 1);
    delay(10);
}

}  // namespace DemoRendering
