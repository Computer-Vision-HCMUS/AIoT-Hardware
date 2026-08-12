/**
 * @file display_controller.h
 * @brief TFT Display abstraction layer for hardware demo
 * 
 * Provides a clean interface for display operations.
 * Separates display driver details from application logic per Constitution Principle III.
 */

#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <cstdint>
#include <string>

/**
 * @class DisplayController
 * @brief Abstracts TFT display rendering operations
 * 
 * Handles low-level SPI communication, font rendering, and screen updates.
 * Application code calls only these high-level display methods.
 */
class DisplayController {
public:
    /**
     * @brief Construct a new DisplayController instance
     */
    DisplayController();

    /**
     * @brief Initialize the TFT display
     * @return true if initialization successful, false otherwise
     */
    bool init();

    /**
     * @brief Set the current text color
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     */
    void setColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Set the background color
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     */
    void setBackgroundColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Draw text at specified position
     * @param x X coordinate (pixels)
     * @param y Y coordinate (pixels)
     * @param text Text string to draw
     * @param fontSize Font size (1-7, scaled from base font)
     */
    void drawText(uint16_t x, uint16_t y, const std::string& text, uint8_t fontSize = 2);

    /**
     * @brief Draw text with its right edge aligned to a coordinate
     */
    void drawTextRightAligned(uint16_t rightX, uint16_t y,
                              const std::string& text, uint8_t fontSize = 2);

    /**
     * @brief Draw a filled rectangle
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Rectangle width in pixels
     * @param height Rectangle height in pixels
     * @param filled True to fill, false for outline
     */
    void drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool filled = true);

    /**
     * @brief Draw a rounded rectangle
     */
    void drawRoundedRectangle(uint16_t x, uint16_t y, uint16_t width,
                              uint16_t height, uint16_t radius,
                              bool filled = true);

    /**
     * @brief Clear the entire display with background color
     */
    void clear();

    /**
     * @brief Update/refresh the display
     */
    void update();

    /**
     * @brief Get display width in pixels
     * @return Display width
     */
    uint16_t getWidth() const;

    /**
     * @brief Get display height in pixels
     * @return Display height
     */
    uint16_t getHeight() const;

    /**
     * @brief Check if display is initialized
     * @return true if initialized and ready
     */
    bool isReady() const;

private:
    bool initialized_;
    uint16_t width_;
    uint16_t height_;
    uint8_t text_color_r_, text_color_g_, text_color_b_;
    uint8_t bg_color_r_, bg_color_g_, bg_color_b_;

    /**
     * @brief Internal SPI initialization for display communication
     */
    void initSPI();

    /**
     * @brief Send command to display
     * @param cmd Command byte
     */
    void sendCommand(uint8_t cmd);

    /**
     * @brief Send data to display
     * @param data Data byte
     */
    void sendData(uint8_t data);
};

#endif // DISPLAY_CONTROLLER_H
