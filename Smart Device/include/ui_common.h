#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <cstdint>
#include <string>
#include <memory>

// Forward-declare DisplayController to avoid include-path issues in IDEs.
class DisplayController;

namespace UICommon {

// Basic primitives
void drawLabel(DisplayController& display, uint16_t x, uint16_t y,
               const std::string& text, uint8_t fontSize,
               uint8_t r, uint8_t g, uint8_t b);

void drawDivider(DisplayController& display, uint16_t y);

void drawCard(DisplayController& display, uint16_t x, uint16_t y,
              uint16_t width, uint16_t height,
              const std::string& title, const std::string& detail);

// Context-sensitive button legend — shows all 5 physical button functions
// Labels correspond to buttons: MODE, ACTION, START, NEXT, BACK
void drawButtonLegend(DisplayController& display,
                      const std::string& modeLabel,
                      const std::string& actionLabel,
                      const std::string& startLabel,
                      const std::string& nextLabel,
                      const std::string& backLabel);

// Legacy single-string hint bar (kept for compatibility)
void drawHintBar(DisplayController& display, const std::string& hintText);

// Full screen frame: clear + title + subtitle + divider (NO legend — call drawButtonLegend after)
void drawScreenFrame(DisplayController& display,
                     const std::string& title,
                     const std::string& subtitle);

}  // namespace UICommon

#endif  // UI_COMMON_H
