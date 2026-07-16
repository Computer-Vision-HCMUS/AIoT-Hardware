/**
 * @file button_manager.h
 * @brief Button input handling and debounce manager
 * 
 * Manages GPIO interrupts for physical buttons and provides debounce filtering.
 * Separates GPIO interrupt logic from application per Constitution Principle III.
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <cstdint>
#include <ctime>

/**
 * @enum ButtonID
 * @brief Symbolic names for physical buttons
 */
enum class ButtonID : uint8_t {
    MODE = 0,
    ACTION = 1,
    START = 2,
    NEXT = 3,
    BACK = 4
};

/**
 * @struct ButtonState
 * @brief Current state of a single button
 */
struct ButtonState {
    ButtonID button_id;           ///< Which button
    uint32_t press_count;         ///< Total number of presses detected
    uint64_t last_press_time_ms;  ///< Timestamp of last confirmed press
    bool currently_pressed;       ///< Current press state (debounced)
};

/**
 * @class ButtonManager
 * @brief Manages button input detection and debouncing
 * 
 * Handles GPIO interrupt setup, debounce filtering, and state tracking.
 * Detects button presses at 10+ presses/second target rate.
 */
class ButtonManager {
public:
    /**
     * @brief Construct a new ButtonManager instance
     */
    ButtonManager();

    /**
     * @brief Initialize button manager and GPIO interrupts
     * @return true if initialization successful, false otherwise
     */
    bool init();

    /**
     * @brief Get the current state of a button
     * @param button_id Which button to query
     * @return ButtonState structure with current button state
     */
    ButtonState getButtonState(ButtonID button_id);

    /**
     * @brief Reset press count for a button
     * @param button_id Which button to reset
     */
    void resetPressCount(ButtonID button_id);

    /**
     * @brief Reset all button states
     */
    void resetAll();

    /**
     * @brief Check if button was pressed since last check
     * @param button_id Which button to query
     * @return true if button was pressed
     */
    bool wasPressed(ButtonID button_id);

    /**
     * @brief Check if button is currently pressed (after debounce)
     * @param button_id Which button to query
     * @return true if button is physically pressed
     */
    bool isPressed(ButtonID button_id);

    /**
     * @brief Update button states (should be called regularly from main loop)
     * Call this from your main demo loop for debounce processing.
     */
    void update();

    /**
     * @brief Check if button manager is initialized
     * @return true if initialized and ready
     */
    bool isReady() const;

private:
    bool initialized_;
    static constexpr uint8_t BUTTON_COUNT = 5;
    ButtonState button_states_[BUTTON_COUNT];
    bool previous_pressed_[BUTTON_COUNT];
    uint32_t debounce_counters_[BUTTON_COUNT];
    bool just_pressed_[BUTTON_COUNT];

    /**
     * @brief Internal GPIO interrupt handler
     * Sets up the ISR for button detection
     */
    void setupGPIOInterrupts();

    /**
     * @brief Process debounce logic for a button
     * @param button_idx Button index (0=A, 1=B)
     */
    void debounceButton(uint8_t button_idx);

    /**
     * @brief Read raw GPIO state for a button
     * @param button_idx Button index (0=A, 1=B)
     * @return true if button GPIO is active (low on this hardware)
     */
    bool readButtonGPIO(uint8_t button_idx);
};

#endif // BUTTON_MANAGER_H
