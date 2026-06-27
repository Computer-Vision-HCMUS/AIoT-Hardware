/**
 * @file test_state_machine.cpp
 * @brief Unit tests for DemoStateMachine class (T033)
 * 
 * Tests state transitions and navigation logic.
 */

#include <unity.h>
#include "../include/demo_state_machine.h"

// Create global state machine for testing
DemoStateMachine state_machine;

/**
 * @brief Setup before each test
 */
void setUp(void) {
    TEST_ASSERT_TRUE(state_machine.init());
}

/**
 * @brief Cleanup after each test
 */
void tearDown(void) {
    state_machine.reset();
}

/**
 * @test T033.1: StateMachine initialization
 */
void test_state_machine_init(void) {
    TEST_ASSERT_TRUE(state_machine.isReady());
}

/**
 * @test T033.2: Initial state is WELCOME
 */
void test_initial_state_is_welcome(void) {
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("WELCOME", state_machine.getStateName());
}

/**
 * @test T033.3: Transition from WELCOME to DEVICE_INFO
 */
void test_transition_welcome_to_device_info(void) {
    state_machine.transitionNext();
    TEST_ASSERT_EQUAL_INT(1, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("DEVICE_INFO", state_machine.getStateName());
}

/**
 * @test T033.4: Transition from DEVICE_INFO to WELCOME
 */
void test_transition_device_info_to_welcome(void) {
    state_machine.transitionNext();  // WELCOME -> DEVICE_INFO
    state_machine.transitionNext();  // DEVICE_INFO -> WELCOME
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("WELCOME", state_machine.getStateName());
}

/**
 * @test T033.5: Cyclic transitions
 */
void test_cyclic_transitions(void) {
    // Cycle through states multiple times
    for (int i = 0; i < 5; i++) {
        state_machine.transitionNext();
        DemoState state = state_machine.getCurrentState();
        TEST_ASSERT_TRUE((state == DemoState::DEVICE_INFO || state == DemoState::WELCOME));
    }
}

/**
 * @test T033.6: Direct transition to specific state
 */
void test_direct_transition_to_state(void) {
    state_machine.transitionTo(DemoState::DEVICE_INFO);
    TEST_ASSERT_EQUAL_INT(1, (int)state_machine.getCurrentState());
    
    state_machine.transitionTo(DemoState::WELCOME);
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
}

/**
 * @test T033.7: Reset to initial state
 */
void test_reset_to_initial_state(void) {
    state_machine.transitionNext();
    TEST_ASSERT_EQUAL_INT(1, (int)state_machine.getCurrentState());
    
    state_machine.reset();
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("WELCOME", state_machine.getStateName());
}

/**
 * @test T033.8: Error state transitions to WELCOME
 */
void test_error_state_returns_to_welcome(void) {
    state_machine.transitionTo(DemoState::ERROR_STATE);
    TEST_ASSERT_EQUAL_INT(2, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("ERROR", state_machine.getStateName());
    
    state_machine.transitionNext();
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("WELCOME", state_machine.getStateName());
}

/**
 * @test T033.9: State name accuracy for all states
 */
void test_state_name_accuracy(void) {
    state_machine.transitionTo(DemoState::WELCOME);
    TEST_ASSERT_EQUAL_STRING("WELCOME", state_machine.getStateName());
    
    state_machine.transitionTo(DemoState::DEVICE_INFO);
    TEST_ASSERT_EQUAL_STRING("DEVICE_INFO", state_machine.getStateName());
    
    state_machine.transitionTo(DemoState::ERROR_STATE);
    TEST_ASSERT_EQUAL_STRING("ERROR", state_machine.getStateName());
}

// ============================================================================
// Test Runner
// ============================================================================

void run_state_machine_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_state_machine_init);
    RUN_TEST(test_initial_state_is_welcome);
    RUN_TEST(test_transition_welcome_to_device_info);
    RUN_TEST(test_transition_device_info_to_welcome);
    RUN_TEST(test_cyclic_transitions);
    RUN_TEST(test_direct_transition_to_state);
    RUN_TEST(test_reset_to_initial_state);
    RUN_TEST(test_error_state_returns_to_welcome);
    RUN_TEST(test_state_name_accuracy);
    UNITY_END();
}
