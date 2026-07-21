#ifndef DEMO_APP_H
#define DEMO_APP_H

#include <cstdint>
#include "display_controller.h"
#include "button_manager.h"
#include "demo_state_machine.h"
#include "navigation.h"
#include "hal/audio_manager.h"

class DemoApp {
public:
    DemoApp();

    bool init();
    bool update();
    bool isRunning() const;
    void stop();

private:
    DisplayController* display_;
    ButtonManager*     buttons_;
    DemoStateMachine*  state_machine_;
    AudioManager*      audio_;

    bool      demo_running_;
    uint64_t  boot_time_ms_;
    uint64_t  last_transition_ms_;
    DemoState last_rendered_state_;
    AppState  app_state_;
    bool      needs_redraw_;  // set true when button pressed (cursor moves same screen)
};

#endif  // DEMO_APP_H
