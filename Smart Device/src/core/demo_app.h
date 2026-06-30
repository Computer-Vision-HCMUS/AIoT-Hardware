#ifndef DEMO_APP_H
#define DEMO_APP_H

#include <cstdint>
#include "display_controller.h"
#include "button_manager.h"
#include "demo_state_machine.h"

class DemoApp {
public:
    DemoApp();

    bool init();
    bool update();
    bool isRunning() const;
    void stop();

private:
    DisplayController* display_;
    ButtonManager* buttons_;
    DemoStateMachine* state_machine_;

    bool demo_running_;
    uint64_t boot_time_ms_;
    uint64_t last_transition_ms_;
    DemoState last_rendered_state_;

    static constexpr uint64_t MIN_TRANSITION_INTERVAL_MS = 1000;
};

#endif  // DEMO_APP_H
