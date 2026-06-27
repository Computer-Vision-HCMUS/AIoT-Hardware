/**
 * @file demo_state_machine.h
 * @brief State machine for hardware demo navigation and screen transitions
 * 
 * Manages demo flow with state transitions between different display screens.
 * Clean state transitions for edge case handling and maintainability.
 */

#ifndef DEMO_STATE_MACHINE_H
#define DEMO_STATE_MACHINE_H

#include <cstdint>

/**
 * @enum DemoState
 * @brief States for the hardware demo
 */
enum class DemoState : uint8_t {
    WELCOME,        ///< Welcome screen with device name and status
    DEVICE_INFO,    ///< Device information screen (MAC, version, etc)
    ERROR_STATE     ///< Error or fault state
};

/**
 * @class DemoStateMachine
 * @brief Manages navigation between demo screens
 * 
 * Handles state transitions and provides methods to query current state.
 * Demo cycles: WELCOME → DEVICE_INFO → WELCOME (repeating)
 */
class DemoStateMachine {
public:
    /**
     * @brief Construct a new DemoStateMachine instance
     */
    DemoStateMachine();

    /**
     * @brief Initialize the state machine
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Get the current demo state
     * @return Current DemoState
     */
    DemoState getCurrentState() const;

    /**
     * @brief Transition to the next state in the demo sequence
     * Cycles: WELCOME → DEVICE_INFO → WELCOME
     */
    void transitionNext();

    /**
     * @brief Transition to a specific state
     * @param new_state Target state
     */
    void transitionTo(DemoState new_state);

    /**
     * @brief Reset to initial state (WELCOME)
     */
    void reset();

    /**
     * @brief Get string description of current state
     * @return State name as string (for logging/display)
     */
    const char* getStateName() const;

    /**
     * @brief Check if state machine is ready
     * @return true if initialized
     */
    bool isReady() const;

private:
    bool initialized_;
    DemoState current_state_;

    /**
     * @brief Get the next state in sequence
     * @param current Current state
     * @return Next state in cycle
     */
    DemoState getNextState(DemoState current);
};

#endif // DEMO_STATE_MACHINE_H
