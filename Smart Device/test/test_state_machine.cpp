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
 * @test T033.2: Initial state is HOME
 */
void test_initial_state_is_home(void) {
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("HOME", state_machine.getStateName());
}

/**
 * @test T033.3: Transition from HOME to CHECK_IN
 */
void test_transition_home_to_check_in(void) {
    state_machine.transitionNext();
    TEST_ASSERT_EQUAL_INT(1, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("CHECK_IN", state_machine.getStateName());
}

/**
 * @test T033.4: Back navigation from CHECK_IN returns to HOME
 */
void test_transition_back_from_check_in_to_home(void) {
    state_machine.transitionNext();
    state_machine.transitionBack();
    TEST_ASSERT_EQUAL_INT(0, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("HOME", state_machine.getStateName());
}

/**
 * @test T033.5: Forward navigation advances through the full demo flow
 */
void test_full_flow_navigation(void) {
    state_machine.transitionNext();  // HOME -> CHECK_IN
    state_machine.transitionNext();  // CHECK_IN -> RESULT
    state_machine.transitionNext();  // RESULT -> SUPPORT
    state_machine.transitionNext();  // SUPPORT -> ACTIVITY
    state_machine.transitionNext();  // ACTIVITY -> MUSIC_PODCAST
    state_machine.transitionNext();  // MUSIC_PODCAST -> CONVERSATION
    state_machine.transitionNext();  // CONVERSATION -> STATUS
    state_machine.transitionNext();  // STATUS -> REPORT

    TEST_ASSERT_EQUAL_INT(8, (int)state_machine.getCurrentState());
    TEST_ASSERT_EQUAL_STRING("REPORT", state_machine.getStateName());
}

/**
 * @test T033.6: Direct transition to specific state
 */
void test_direct_transition_to_state(void) {
    state_machine.transitionTo(DemoState::STATUS);
    TEST_ASSERT_EQUAL_INT(7, (int)state_machine.getCurrentState());

    state_machine.transitionTo(DemoState::HOME);
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
    TEST_ASSERT_EQUAL_STRING("HOME", state_machine.getStateName());
}

/**
 * @test T033.8: State name accuracy for all states
 */
void test_state_name_accuracy(void) {
    state_machine.transitionTo(DemoState::HOME);
    TEST_ASSERT_EQUAL_STRING("HOME", state_machine.getStateName());

    state_machine.transitionTo(DemoState::CHECK_IN);
    TEST_ASSERT_EQUAL_STRING("CHECK_IN", state_machine.getStateName());

    state_machine.transitionTo(DemoState::RESULT);
    TEST_ASSERT_EQUAL_STRING("RESULT", state_machine.getStateName());

    state_machine.transitionTo(DemoState::SUPPORT);
    TEST_ASSERT_EQUAL_STRING("SUPPORT", state_machine.getStateName());

    state_machine.transitionTo(DemoState::ACTIVITY);
    TEST_ASSERT_EQUAL_STRING("ACTIVITY", state_machine.getStateName());

    state_machine.transitionTo(DemoState::MUSIC_PODCAST);
    TEST_ASSERT_EQUAL_STRING("MUSIC_PODCAST", state_machine.getStateName());

    state_machine.transitionTo(DemoState::CONVERSATION);
    TEST_ASSERT_EQUAL_STRING("CONVERSATION", state_machine.getStateName());

    state_machine.transitionTo(DemoState::STATUS);
    TEST_ASSERT_EQUAL_STRING("STATUS", state_machine.getStateName());

    state_machine.transitionTo(DemoState::REPORT);
    TEST_ASSERT_EQUAL_STRING("REPORT", state_machine.getStateName());
}

// ============================================================================
// Test Runner
// ============================================================================

void run_state_machine_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_state_machine_init);
    RUN_TEST(test_initial_state_is_home);
    RUN_TEST(test_transition_home_to_check_in);
    RUN_TEST(test_transition_back_from_check_in_to_home);
    RUN_TEST(test_full_flow_navigation);
    RUN_TEST(test_direct_transition_to_state);
    RUN_TEST(test_reset_to_initial_state);
    RUN_TEST(test_state_name_accuracy);
    UNITY_END();
}
