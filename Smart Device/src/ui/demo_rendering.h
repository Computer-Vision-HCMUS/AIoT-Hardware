#ifndef DEMO_RENDERING_H
#define DEMO_RENDERING_H

#include <cstdint>
#include "display_controller.h"

namespace DemoRendering {
void renderWelcomeScreen(DisplayController& display, uint64_t boot_time_ms);
void renderDeviceInfoScreen(DisplayController& display);
void renderErrorScreen(DisplayController& display, const char* error_msg);
}  // namespace DemoRendering

#endif  // DEMO_RENDERING_H
