/**
 * @file button_manager.cpp
 * @brief Button input handling and debounce implementation
 */

#include "button_manager.h"
#include "pins_config.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <Arduino.h>

// Global button manager instance for ISR access
static ButtonManager* g_button_manager = nullptr;

// ISR flag for button events
static volatile bool button_event = false;

// ============================================================================
// GPIO ISR Callback
// ============================================================================

static void IRAM_ATTR button_isr_handler(void* arg) {
    button_event = true;
}

// ============================================================================
// ButtonManager Implementation
// ============================================================================

ButtonManager::ButtonManager()
    : initialized_(false) {
    // Initialize button states
    for (int i = 0; i < 2; i++) {
        button_states_[i].button_id = (i == 0) ? ButtonID::BUTTON_A : ButtonID::BUTTON_B;
        button_states_[i].press_count = 0;
        button_states_[i].last_press_time_ms = 0;
        button_states_[i].currently_pressed = false;
        previous_pressed_[i] = false;
        debounce_counters_[i] = 0;
    }
    
    // Store global reference for ISR
    g_button_manager = this;
}

bool ButtonManager::init() {
    if (initialized_) {
        return true;
    }

    try {
        Serial.println("[Button] Initializing button manager...");
        
        // Configure GPIO for buttons as input with pull-up
        gpio_config_t io_conf = {};
        
        // Button A and B pins
        io_conf.pin_bit_mask = (1ULL << BUTTON_A_PIN) | (1ULL << BUTTON_B_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_NEGEDGE;  // Trigger on falling edge (active low)

        // Apply GPIO configuration
        gpio_config(&io_conf);
        Serial.printf("[Button] GPIO configured: A=%d, B=%d\n", BUTTON_A_PIN, BUTTON_B_PIN);

        // Install GPIO ISR service
        gpio_install_isr_service(0);
        Serial.println("[Button] ISR service installed");
        
        // Add ISR handlers for both buttons
        gpio_isr_handler_add((gpio_num_t)BUTTON_A_PIN, button_isr_handler, nullptr);
        gpio_isr_handler_add((gpio_num_t)BUTTON_B_PIN, button_isr_handler, nullptr);
        Serial.println("[Button] ISR handlers registered");

        initialized_ = true;
        Serial.println("[Button] Button manager initialized successfully!");
        return true;
    } catch (...) {
        Serial.println("[Button] ERROR: Button initialization failed!");
        initialized_ = false;
        return false;
    }
}

ButtonState ButtonManager::getButtonState(ButtonID button_id) {
    int idx = (button_id == ButtonID::BUTTON_A) ? 0 : 1;
    return button_states_[idx];
}

void ButtonManager::resetPressCount(ButtonID button_id) {
    int idx = (button_id == ButtonID::BUTTON_A) ? 0 : 1;
    button_states_[idx].press_count = 0;
}

void ButtonManager::resetAll() {
    for (int i = 0; i < 2; i++) {
        button_states_[i].press_count = 0;
        button_states_[i].last_press_time_ms = 0;
        button_states_[i].currently_pressed = false;
    }
}

bool ButtonManager::wasPressed(ButtonID button_id) {
    int idx = (button_id == ButtonID::BUTTON_A) ? 0 : 1;
    return just_pressed_[idx];
}

bool ButtonManager::isPressed(ButtonID button_id) {
    int idx = (button_id == ButtonID::BUTTON_A) ? 0 : 1;
    return button_states_[idx].currently_pressed;
}

void ButtonManager::update() {
    if (!initialized_) {
        return;
    }

    bool prev_a = button_states_[0].currently_pressed;
    bool prev_b = button_states_[1].currently_pressed;

    // Update debounce for each button
    debounceButton(0);  // Button A
    debounceButton(1);  // Button B

    just_pressed_[0] = (!prev_a && button_states_[0].currently_pressed);
    just_pressed_[1] = (!prev_b && button_states_[1].currently_pressed);
    
    // Clear ISR flag after processing
    button_event = false;
}

bool ButtonManager::isReady() const {
    return initialized_;
}

void ButtonManager::setupGPIOInterrupts() {
    // Already handled in init()
}

void ButtonManager::debounceButton(uint8_t button_idx) {
    if (button_idx >= 2) return;

    // Read current GPIO state
    bool gpio_pressed = readButtonGPIO(button_idx);

    // Debounce logic: require stable samples
    if (gpio_pressed) {
        debounce_counters_[button_idx]++;
        if (debounce_counters_[button_idx] >= DEBOUNCE_SAMPLES) {
            // Stable pressed state confirmed
            if (!button_states_[button_idx].currently_pressed) {
                // Transition from not-pressed to pressed - increment counter
                button_states_[button_idx].press_count++;
                button_states_[button_idx].last_press_time_ms = esp_timer_get_time() / 1000;
            }
            button_states_[button_idx].currently_pressed = true;
        }
    } else {
        debounce_counters_[button_idx] = 0;
        button_states_[button_idx].currently_pressed = false;
    }
}

bool ButtonManager::readButtonGPIO(uint8_t button_idx) {
    gpio_num_t pin = (button_idx == 0) ? (gpio_num_t)BUTTON_A_PIN : (gpio_num_t)BUTTON_B_PIN;
    int level = gpio_get_level(pin);
    
    // Buttons are active-low (pressed = 0)
    return (level == 0);
}
