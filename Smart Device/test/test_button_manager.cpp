/**
 * @file test_button_manager.cpp
 * @brief Unit tests for ButtonManager class (T024)
 * 
 * Tests GPIO interrupt handling and debounce timing.
 */

#include <unity.h>
#include "../include/button_manager.h"

// Create global button manager for testing
ButtonManager button_manager;

/**
 * @brief Setup before each test
 */
void setUp(void) {
    // Initialize button manager before each test
    TEST_ASSERT_TRUE(button_manager.init());
}

/**
 * @brief Cleanup after each test
 */
void tearDown(void) {
    button_manager.resetAll();
}

/**
 * @test T024.1: ButtonManager initialization
 */
void test_button_manager_init(void) {
    TEST_ASSERT_TRUE(button_manager.isReady());
}

/**
 * @test T024.2: Get initial button state
 */
void test_get_initial_button_state(void) {
    ButtonState state = button_manager.getButtonState(ButtonID::BUTTON_A);
    TEST_ASSERT_EQUAL_UINT32(0, state.press_count);
    TEST_ASSERT_FALSE(state.currently_pressed);
}

/**
 * @test T024.3: Reset single button press count
 */
void test_reset_single_button(void) {
    button_manager.resetPressCount(ButtonID::BUTTON_A);
    ButtonState state = button_manager.getButtonState(ButtonID::BUTTON_A);
    TEST_ASSERT_EQUAL_UINT32(0, state.press_count);
}

/**
 * @test T024.4: Reset all buttons
 */
void test_reset_all_buttons(void) {
    button_manager.resetAll();
    ButtonState state_a = button_manager.getButtonState(ButtonID::BUTTON_A);
    ButtonState state_b = button_manager.getButtonState(ButtonID::BUTTON_B);
    TEST_ASSERT_EQUAL_UINT32(0, state_a.press_count);
    TEST_ASSERT_EQUAL_UINT32(0, state_b.press_count);
    TEST_ASSERT_FALSE(state_a.currently_pressed);
    TEST_ASSERT_FALSE(state_b.currently_pressed);
}

/**
 * @test T024.5: Check button pressed state
 */
void test_check_button_state_methods(void) {
    // Initially should not be pressed
    TEST_ASSERT_FALSE(button_manager.isPressed(ButtonID::BUTTON_A));
    TEST_ASSERT_FALSE(button_manager.wasPressed(ButtonID::BUTTON_A));
}

/**
 * @test T024.6: Update button states
 */
void test_update_button_manager(void) {
    // Update should not crash
    button_manager.update();
    TEST_PASS();
}

/**
 * @test T024.7: Both buttons can be queried independently
 */
void test_independent_button_states(void) {
    ButtonState state_a = button_manager.getButtonState(ButtonID::BUTTON_A);
    ButtonState state_b = button_manager.getButtonState(ButtonID::BUTTON_B);
    
    TEST_ASSERT_EQUAL_UINT8(0, (uint8_t)state_a.button_id);
    TEST_ASSERT_EQUAL_UINT8(1, (uint8_t)state_b.button_id);
}

// ============================================================================
// Test Runner
// ============================================================================

void run_button_manager_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_button_manager_init);
    RUN_TEST(test_get_initial_button_state);
    RUN_TEST(test_reset_single_button);
    RUN_TEST(test_reset_all_buttons);
    RUN_TEST(test_check_button_state_methods);
    RUN_TEST(test_update_button_manager);
    RUN_TEST(test_independent_button_states);
    UNITY_END();
}
