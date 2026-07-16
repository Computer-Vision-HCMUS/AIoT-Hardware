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

namespace {
constexpr uint8_t kButtonCount = 5;
constexpr uint32_t kDebounceWindowMs = DEBOUNCE_TIME_MS;
constexpr uint32_t kDebounceSamples = DEBOUNCE_SAMPLES;
}

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
    for (uint8_t i = 0; i < kButtonCount; ++i) {
        button_states_[i].button_id = static_cast<ButtonID>(i);
        button_states_[i].press_count = 0;
        button_states_[i].last_press_time_ms = 0;
        button_states_[i].currently_pressed = false;
        previous_pressed_[i] = false;
        debounce_counters_[i] = 0;
        just_pressed_[i] = false;
    }

    g_button_manager = this;
}

bool ButtonManager::init() {
    if (initialized_) {
        return true;
    }

    try {
        Serial.println("[Button] Initializing button manager...");
        
        gpio_config_t io_conf = {};
        uint64_t pin_mask = (1ULL << BUTTON_A_PIN) | (1ULL << BUTTON_B_PIN) | (1ULL << START_PIN) |
                            (1ULL << NEXT_PIN) | (1ULL << BACK_PIN);
        io_conf.pin_bit_mask = pin_mask;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = BUTTON_INTR_TYPE;  // Trigger configured in pins_config.h

        // Apply GPIO configuration
        gpio_config(&io_conf);
        Serial.printf("[Button] GPIO configured: MODE=%d ACTION=%d START=%d NEXT=%d BACK=%d\n",
                      MODE_PIN, ACTION_PIN, START_PIN, NEXT_PIN, BACK_PIN);

        gpio_install_isr_service(0);
        Serial.println("[Button] ISR service installed");

        gpio_isr_handler_add((gpio_num_t)BUTTON_A_PIN, button_isr_handler, nullptr);
        gpio_isr_handler_add((gpio_num_t)BUTTON_B_PIN, button_isr_handler, nullptr);
        gpio_isr_handler_add((gpio_num_t)START_PIN, button_isr_handler, nullptr);
        gpio_isr_handler_add((gpio_num_t)NEXT_PIN, button_isr_handler, nullptr);
        gpio_isr_handler_add((gpio_num_t)BACK_PIN, button_isr_handler, nullptr);
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
    uint8_t idx = static_cast<uint8_t>(button_id);
    if (idx >= ButtonManager::BUTTON_COUNT) {
        idx = 0;
    }
    return button_states_[idx];
}

void ButtonManager::resetPressCount(ButtonID button_id) {
    uint8_t idx = static_cast<uint8_t>(button_id);
    if (idx >= ButtonManager::BUTTON_COUNT) {
        return;
    }
    button_states_[idx].press_count = 0;
}

void ButtonManager::resetAll() {
    for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
        button_states_[i].press_count = 0;
        button_states_[i].last_press_time_ms = 0;
        button_states_[i].currently_pressed = false;
        debounce_counters_[i] = 0;
        previous_pressed_[i] = false;
        just_pressed_[i] = false;
    }
    button_event = false;
}

bool ButtonManager::wasPressed(ButtonID button_id) {
    uint8_t idx = static_cast<uint8_t>(button_id);
    if (idx >= BUTTON_COUNT) {
        return false;
    }
    return just_pressed_[idx];
}

bool ButtonManager::isPressed(ButtonID button_id) {
    uint8_t idx = static_cast<uint8_t>(button_id);
    if (idx >= BUTTON_COUNT) {
        return false;
    }
    return button_states_[idx].currently_pressed;
}

void ButtonManager::update() {
    if (!initialized_) {
        return;
    }

    for (uint8_t i = 0; i < ButtonManager::BUTTON_COUNT; ++i) {
        bool prev_state = button_states_[i].currently_pressed;
        debounceButton(i);
        just_pressed_[i] = (!prev_state && button_states_[i].currently_pressed);
    }

    button_event = false;
}

bool ButtonManager::isReady() const {
    return initialized_;
}

void ButtonManager::setupGPIOInterrupts() {
    // Already handled in init()
}

void ButtonManager::debounceButton(uint8_t button_idx) {
    if (button_idx >= ButtonManager::BUTTON_COUNT) {
        return;
    }

    bool gpio_pressed = readButtonGPIO(button_idx);
    if (gpio_pressed) {
        debounce_counters_[button_idx]++;
        if (debounce_counters_[button_idx] >= kDebounceSamples) {
            if (!button_states_[button_idx].currently_pressed) {
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
    gpio_num_t pin = GPIO_NUM_NC;
    switch (button_idx) {
        case 0:
            pin = (gpio_num_t)BUTTON_A_PIN;
            break;
        case 1:
            pin = (gpio_num_t)BUTTON_B_PIN;
            break;
        case 2:
            pin = (gpio_num_t)START_PIN;
            break;
        case 3:
            pin = (gpio_num_t)NEXT_PIN;
            break;
        case 4:
            pin = (gpio_num_t)BACK_PIN;
            break;
        default:
            pin = (gpio_num_t)BUTTON_A_PIN;
            break;
    }

    int level = gpio_get_level(pin);

    // Buttons are active-low (pressed = 0)
    return (level == 0);
}
