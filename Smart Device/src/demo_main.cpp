/**
 * @file demo_main.cpp
 * @brief Hardware demo application logic
 *
 * Coordinates display, button input, and state machine for the demo.
 */

#include "demo.h"
#include "core/demo_app.h"
#include <Arduino.h>

namespace {
DemoApp* g_demo_app = nullptr;
}

bool demo_init() {
    if (g_demo_app == nullptr) {
        g_demo_app = new DemoApp();
    }

    if (!g_demo_app->init()) {
        return false;
    }

    return true;
}

bool demo_update() {
    if (!g_demo_app) {
        return false;
    }

    return g_demo_app->update();
}

bool demo_is_running() {
    return g_demo_app != nullptr && g_demo_app->isRunning();
}

void demo_stop() {
    if (g_demo_app) {
        g_demo_app->stop();
        delete g_demo_app;
        g_demo_app = nullptr;
    }
}
