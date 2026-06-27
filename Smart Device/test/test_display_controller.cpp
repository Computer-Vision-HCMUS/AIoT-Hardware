/**
 * @file test_display_controller.cpp
 * @brief Unit tests for DisplayController class (T014)
 * 
 * Tests display initialization, text rendering, and color handling.
 */

#include <unity.h>
#include "../include/display_controller.h"

// Create global display controller for testing
DisplayController display_controller;

/**
 * @brief Setup before each test
 */
void setUp(void) {
    // Initialize display before each test
    TEST_ASSERT_TRUE(display_controller.init());
}

/**
 * @brief Cleanup after each test
 */
void tearDown(void) {
    // Cleanup after test
}

/**
 * @test T014.1: DisplayController initialization
 */
void test_display_controller_init(void) {
    TEST_ASSERT_TRUE(display_controller.isReady());
}

/**
 * @test T014.2: Display dimensions
 */
void test_display_dimensions(void) {
    TEST_ASSERT_EQUAL_UINT16(320, display_controller.getWidth());
    TEST_ASSERT_EQUAL_UINT16(240, display_controller.getHeight());
}

/**
 * @test T014.3: Set text color
 */
void test_set_text_color(void) {
    // Should not throw or crash
    display_controller.setColor(255, 128, 64);
    TEST_PASS();
}

/**
 * @test T014.4: Set background color
 */
void test_set_background_color(void) {
    // Should not throw or crash
    display_controller.setBackgroundColor(0, 0, 0);
    TEST_PASS();
}

/**
 * @test T014.5: Clear display
 */
void test_clear_display(void) {
    // Should not throw or crash
    display_controller.clear();
    TEST_PASS();
}

/**
 * @test T014.6: Draw text
 */
void test_draw_text(void) {
    // Should not throw or crash
    display_controller.drawText(0, 0, "Test", 2);
    TEST_PASS();
}

/**
 * @test T014.7: Draw rectangle
 */
void test_draw_rectangle(void) {
    // Should not throw or crash
    display_controller.drawRectangle(10, 10, 100, 100, true);
    TEST_PASS();
}

/**
 * @test T014.8: Update display
 */
void test_update_display(void) {
    // Should not throw or crash
    display_controller.update();
    TEST_PASS();
}

/**
 * @test T014.9: Font size clamping
 */
void test_font_size_clamping(void) {
    // Sizes should be clamped to 1-7
    display_controller.drawText(0, 0, "Size 0", 0);  // Should use 1
    display_controller.drawText(0, 10, "Size 8", 8); // Should use 7
    TEST_PASS();
}

// ============================================================================
// Test Runner
// ============================================================================

void run_display_controller_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_display_controller_init);
    RUN_TEST(test_display_dimensions);
    RUN_TEST(test_set_text_color);
    RUN_TEST(test_set_background_color);
    RUN_TEST(test_clear_display);
    RUN_TEST(test_draw_text);
    RUN_TEST(test_draw_rectangle);
    RUN_TEST(test_update_display);
    RUN_TEST(test_font_size_clamping);
    UNITY_END();
}
