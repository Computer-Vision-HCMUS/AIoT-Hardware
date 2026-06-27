/**
 * @file demo_main.cpp
 * @brief Hardware demo application logic
 * 
 * Coordinates display, button input, and state machine for the demo.
 */

#include "demo.h"
#include "display_controller.h"
#include "button_manager.h"
#include "demo_state_machine.h"
#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>

// ============================================================================
// Global Demo Components
// ============================================================================

static DisplayController* g_display = nullptr;
static ButtonManager* g_buttons = nullptr;
static DemoStateMachine* g_state_machine = nullptr;

static bool g_demo_running = false;
static uint64_t g_boot_time_ms = 0;
static uint64_t g_last_transition_ms = 0;  // Prevent rapid state transitions
static const uint64_t MIN_TRANSITION_INTERVAL_MS = 1000;  // 1 second minimum between transitions
static DemoState g_last_rendered_state = DemoState::WELCOME;  // Track last rendered state to avoid flicker

// ============================================================================
// Color Definitions (RGB565 compatible)
// ============================================================================

struct Color {
    uint8_t r, g, b;
};

static const Color COLOR_WHITE = {255, 255, 255};
static const Color COLOR_BLACK = {0, 0, 0};
static const Color COLOR_BLUE = {0, 100, 200};
static const Color COLOR_GREEN = {0, 200, 100};
static const Color COLOR_DARK_GRAY = {64, 64, 64};

// ============================================================================
// Screen Rendering Functions
// ============================================================================

static void renderWelcomeScreen() {
    if (!g_display) return;

    // Clear display
    g_display->setBackgroundColor(0, 0, 0);  // Black
    g_display->clear();
    delay(20);

    // Set white text color
    g_display->setColor(255, 255, 255);  // WHITE
    delay(2);

    // Draw title (centered for 240x240 display)
    g_display->drawText(30, 20, "AIoT Demo", 2);
    delay(20);

    // Draw device name
    g_display->setColor(0, 255, 0);  // Green
    delay(2);
    g_display->drawText(20, 60, "Device: ESP32", 1);
    delay(20);

    // Draw status
    g_display->setColor(0, 150, 255);  // Blue
    delay(2);
    g_display->drawText(20, 80, "Status: OK", 1);
    delay(20);

    // Draw uptime
    g_display->setColor(200, 200, 200);  // Gray
    delay(2);
    uint64_t uptime_s = (esp_timer_get_time() - g_boot_time_ms) / 1000000;
    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "Up: %llu s", uptime_s);
    g_display->drawText(20, 100, uptime_str, 1);
    delay(20);

    // Draw button hint
    g_display->setColor(100, 100, 100);  // Dark gray
    delay(2);
    g_display->drawText(10, 180, "B: Info", 1);
    delay(20);
}

static void renderDeviceInfoScreen() {
    if (!g_display) return;

    // Clear display
    g_display->setBackgroundColor(0, 0, 0);
    g_display->clear();
    delay(20);

    // Draw title
    g_display->setColor(255, 255, 255);
    delay(2);
    g_display->drawText(30, 20, "Info", 2);
    delay(20);

    // Get device info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    // Draw chip model
    g_display->setColor(0, 255, 0);  // Green
    delay(2);
    g_display->drawText(10, 60, "ESP32", 1);
    delay(20);

    // Draw cores
    g_display->setColor(0, 150, 255);  // Blue
    delay(2);
    char cores_str[32];
    snprintf(cores_str, sizeof(cores_str), "Cores: %d", chip_info.cores);
    g_display->drawText(10, 80, cores_str, 1);
    delay(20);

    // Draw button hint
    g_display->setColor(100, 100, 100);  // Gray
    delay(2);
    g_display->drawText(10, 180, "B: Back", 1);
    delay(20);
}

static void renderErrorScreen(const char* error_msg) {
    if (!g_display) return;

    // Clear display with dark red background
    g_display->setBackgroundColor(100, 20, 20);
    g_display->clear();
    delay(10);

    // Draw error title
    g_display->setColor(255, 100, 100);  // Light red
    delay(2);
    g_display->drawText(20, 20, "ERROR", 2);
    delay(10);

    // Draw error message
    g_display->setColor(255, 255, 255);  // White
    delay(2);
    g_display->drawText(10, 60, error_msg, 1);
    delay(10);
}

// ============================================================================
// Demo Public Interface
// ============================================================================

bool demo_init() {
    if (g_demo_running) {
        return true;
    }

    // Record boot time
    g_boot_time_ms = esp_timer_get_time();

    // Create component instances
    g_display = new DisplayController();
    g_buttons = new ButtonManager();
    g_state_machine = new DemoStateMachine();

    // Initialize components
    if (!g_display || !g_display->init()) {
        renderErrorScreen("Display init failed!");
        return false;
    }

    if (!g_buttons || !g_buttons->init()) {
        g_display->clear();
        g_display->setColor(255, 0, 0);
        g_display->drawText(20, 20, "Button init failed!", 2);
        return false;
    }

    if (!g_state_machine || !g_state_machine->init()) {
        g_display->clear();
        g_display->setColor(255, 0, 0);
        g_display->drawText(20, 20, "State machine init failed!", 2);
        return false;
    }

    g_demo_running = true;
    return true;
}

bool demo_update() {
    if (!g_demo_running) {
        return false;
    }

    // Update button states
    if (g_buttons) {
        g_buttons->update();

        // Check for navigation input (Button B) - with transition debounce
        if (g_buttons->wasPressed(ButtonID::BUTTON_B)) {
            uint64_t now_ms = esp_timer_get_time() / 1000;  // Convert to milliseconds
            if (now_ms - g_last_transition_ms > MIN_TRANSITION_INTERVAL_MS) {
                if (g_state_machine) {
                    g_state_machine->transitionNext();
                    g_last_transition_ms = now_ms;
                    Serial.println("[Demo] Button B: State changed");
                }
            }
        }
    }

    // Only render if state has changed (avoid flicker)
    if (g_state_machine) {
        DemoState current_state = g_state_machine->getCurrentState();

        // Render only when state transitions
        if (current_state != g_last_rendered_state) {
            switch (current_state) {
                case DemoState::WELCOME:
                    renderWelcomeScreen();
                    break;

                case DemoState::DEVICE_INFO:
                    renderDeviceInfoScreen();
                    break;

                case DemoState::ERROR_STATE:
                    renderErrorScreen("System Error Detected!");
                    g_state_machine->transitionTo(DemoState::WELCOME);
                    break;

                default:
                    renderWelcomeScreen();
                    break;
            }
            
            g_last_rendered_state = current_state;
        }
    }

    return true;
}

bool demo_is_running() {
    return g_demo_running;
}

void demo_stop() {
    g_demo_running = false;

    if (g_display) {
        delete g_display;
        g_display = nullptr;
    }

    if (g_buttons) {
        delete g_buttons;
        g_buttons = nullptr;
    }

    if (g_state_machine) {
        delete g_state_machine;
        g_state_machine = nullptr;
    }
}
