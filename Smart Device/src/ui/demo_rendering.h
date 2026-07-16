/**
 * @file demo_rendering.h
 * @brief Fallback rendering utilities (error screens only)
 *
 * All primary screen rendering is handled by the ScreenHandlers namespace.
 * This module provides the error fallback only.
 */

#ifndef DEMO_RENDERING_H
#define DEMO_RENDERING_H

#include "display_controller.h"

namespace DemoRendering {
void renderErrorScreen(DisplayController& display, const char* error_msg);
}  // namespace DemoRendering

#endif  // DEMO_RENDERING_H
