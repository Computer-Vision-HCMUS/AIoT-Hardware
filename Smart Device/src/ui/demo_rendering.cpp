/**
 * @file demo_rendering.cpp
 * @brief Fallback error screen rendering
 */

#include "ui/demo_rendering.h"
#include <cstdio>

namespace DemoRendering {

void renderErrorScreen(DisplayController& display, const char* error_msg) {
    if (!display.isReady()) return;

    display.setBackgroundColor(100, 20, 20);
    display.clear();

    display.setColor(255, 100, 100);
    display.drawText(8, 8, "ERROR", 2);

    display.setColor(255, 255, 255);
    display.drawText(8, 46, error_msg, 1);
}

}  // namespace DemoRendering
