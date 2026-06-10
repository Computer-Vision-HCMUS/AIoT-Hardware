# Tasks: SmartClock MVP — Focus & Sleep

**Input**: `SPECIFICATION.md`, `PLAN.md`

**Organization**: Tasks are grouped by user story and foundational work so each story can be implemented and tested independently.

---

## Phase 1: Setup (Shared Infrastructure)

- [ ] T001 [P] Create project structure and empty module files:
  - `SmartDeskBuddy.ino`
  - `config.h`
  - `hal/ISensor.h`, `hal/LightSensor.h/.cpp`, `hal/RtcModule.h/.cpp`, `hal/SoundSensor.h/.cpp`
  - `ui/Screen.h`, `ui/PomodoroScreen.h/.cpp`, `ui/SleepScreen.h/.cpp`
  - `core/StateMachine.h/.cpp`, `core/PomodoroTimer.h/.cpp`, `core/SleepMonitor.h/.cpp`
  - `utils/Logger.h`, `utils/TimeUtils.h`
- [ ] T002 [P] Define MVP constants and pin mappings in `config.h`.
- [ ] T003 [P] Add build and board configuration comments for ESP32/Arduino in `SmartDeskBuddy.ino`.
- [ ] T004 [P] Create a simple boot flow in `SmartDeskBuddy.ino` that initializes hardware and enters the main update loop.

---

## Phase 2: Foundational (Blocking Prerequisites)

- [ ] T005 Implement `hal/ISensor.h` interface and ensure type compatibility for sensor modules.
- [ ] T006 Implement `hal/RtcModule` to read/writer time and persist RTC timestamps.
- [ ] T007 Implement `hal/LightSensor` driver for the BH1750 sensor.
- [ ] T008 Implement `hal/SoundSensor` driver for ambient noise sampling.
- [ ] T009 Implement `ui/Screen` base class with `onEnter()`, `update()`, and `onExit()` methods.
- [ ] T010 Implement `core/PomodoroTimer` logic with start/pause/reset and session completion tracking.
- [ ] T011 Implement `core/SleepMonitor` state tracking for start/end actions and sleep interval duration.
- [ ] T012 Implement persistence helpers in `utils/TimeUtils.h` or a new storage helper to save active session state and history to ESP32 flash.
- [ ] T013 Create `core/StateMachine` to route between Pomodoro and Sleep screens, and support button-driven navigation.

---

## Phase 3: User Story 1 - Pomodoro Focus Timer (Priority: P1)

**Goal**: Deliver the core focus timer experience with configurable intervals, pause/resume, reset, and session tracking.

**Independent Test**: Configure durations, start/pause/resume/reset the Pomodoro timer, and verify session completion notifications and history.

- [ ] T014 Implement Pomodoro timer configuration UI in `ui/PomodoroScreen.cpp`.
- [ ] T015 Implement focus and break duration validation in `core/PomodoroTimer.cpp` using configured bounds (5–120 min focus, 1–30 min break).
- [ ] T016 Implement session completion notification in `core/PomodoroTimer` and connect it to the speaker output.
- [ ] T017 Implement completed session logging in `core/PomodoroTimer` and persist timestamped sessions to flash.
- [ ] T018 Implement Pomodoro screen display and button controls in `ui/PomodoroScreen.cpp`.
- [ ] T019 Integrate Pomodoro timer updates into `SmartDeskBuddy.ino` loop with UI refresh under 200ms.

---

## Phase 4: User Story 2 - Sleep Monitoring (Priority: P1)

**Goal**: Deliver sleep start/end tracking, environmental sampling, duration calculation, and quality scoring.

**Independent Test**: Start a sleep record, sample light/noise every 60 seconds, stop sleep, and verify summary output with quality score.

- [ ] T020 Implement sleep start/end controls in `ui/SleepScreen.cpp`.
- [ ] T021 Implement periodic sampling of light/noise every 60 seconds in `core/SleepMonitor.cpp`.
- [ ] T022 Implement total sleep duration calculation and storage of light/noise metrics in `core/SleepMonitor`.
- [ ] T023 Implement sleep quality scoring in `core/SleepMonitor` using weighted metrics: duration 60%, noise 20%, light 20%.
- [ ] T024 Implement Good/Fair/Poor thresholds in sleep feedback (`Good >= 80`, `Fair 60–79`, `Poor < 60`).
- [ ] T025 Implement sleep summary display in `ui/SleepScreen.cpp` with recommendation text.
- [ ] T026 Persist completed sleep records to ESP32 flash and enforce 30-day / 1,000-record retention.

---

## Phase 5: Polish & Validation

**Purpose**: Ensure MVP quality and cross-cutting correctness.

- [ ] T027 [P] Add state recovery logic in `SmartDeskBuddy.ino` to restore active Pomodoro and sleep sessions on reboot using RTC timestamps.
- [ ] T028 [P] Add local history cleanup logic for 30-day/1,000-record retention in persistence helpers.
- [ ] T029 [P] Validate button navigation and responsiveness across Pomodoro and Sleep screens.
- [ ] T030 [P] Document usage and configuration in `README.md` or new quickstart section.
- [ ] T031 [P] Test UI latency and confirm interactions respond under 200ms.
- [ ] T032 [P] Test offline operation and confirm core Pomodoro/sleep flows work without network connectivity.

---

## Dependencies & Execution Order

- Phase 1 tasks can begin immediately and run in parallel where marked `[P]`.
- Phase 2 tasks must complete before user story implementation begins.
- Phase 3 and Phase 4 tasks can start after Phase 2, and should remain independently testable.
- Phase 5 tasks are final validation and polish after main user stories are complete.
