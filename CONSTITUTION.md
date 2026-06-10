# SmartDesk Buddy — Project Constitution

## 1. Project Identity

- **Name:** SmartDesk Buddy
- **Platform:** ESP32-S3 DevKitC-1 (N8R8)
- **Type:** AIoT Edge Device — Productivity & Wellness Companion
- **Language:** C++ (Arduino framework)

## 2. Core Objectives

1. **STUDY:** Support learning with Pomodoro timer and presentation evaluation.
2. **SLEEP:** Monitor and improve sleep quality.
3. **HOME:** Deliver real-time study environment monitoring.
4. **RELAX:** Provide on-device entertainment.

## 3. Hardware Stack

- **ESP32-S3 N8R8:** Main MCU, AI inference, WiFi.
- **TFT 2.4" ILI9341:** Full UI display (SPI).
- **INMP441:** Audio capture for sound level and seminar recording (I2S port 0).
- **BH1750:** Light sensor (I2C).
- **DS3231:** Real-time clock with alarm (I2C + interrupt).
- **MAX98357A + Speaker:** Audio playback with MicroSD music storage (I2S port 1).
- **MicroSD:** Music file storage on shared SPI bus.
- **Passive Buzzer:** Alert sound output (PWM).
- **5 Buttons:** Navigation controls.

## 4. File Structure

```
SmartDeskBuddy/
├── SmartDeskBuddy.ino      # Entry point — object creation + begin()/update()
├── config.h                # Constants: pins, WiFi, thresholds, intervals
│
├── hal/                    # Hardware Abstraction Layer
│   ├── ISensor.h           # Abstract sensor interface
│   ├── SoundSensor.h/.cpp  # INMP441 implementation
│   ├── LightSensor.h/.cpp  # BH1750 implementation
│   ├── RtcModule.h/.cpp    # DS3231 implementation
│   └── AudioPlayer.h/.cpp  # MAX98357A + SD audio playback
│
├── ui/                     # Display rendering, no business logic
│   ├── Screen.h            # Abstract screen base
│   ├── HomeScreen.h/.cpp
│   ├── StudyScreen.h/.cpp
│   ├── SleepScreen.h/.cpp
│   └── RelaxScreen.h/.cpp
│
├── core/                   # Business logic
│   ├── StateMachine.h/.cpp
│   ├── PomodoroTimer.h/.cpp
│   ├── SleepMonitor.h/.cpp
│   ├── AlertManager.h/.cpp
│   └── DataCollector.h/.cpp
│
├── network/
│   ├── WiFiManager.h/.cpp
│   └── FirebaseClient.h/.cpp
│
└── utils/
    ├── Logger.h            # Header-only debug log
    └── TimeUtils.h         # Header-only time helpers
```

## 5. Architecture Principles

- **Dependency direction:** only downward dependencies.
- **Module flow:** `SmartDeskBuddy.ino` → `core` / `ui` / `network` / `hal`.
- **UI layers** depend on `hal` through interfaces only.
- **Utility code** is shared by lower-level modules.

### Guidelines

1. **Single Responsibility:** one class does one job.
2. **Encapsulation:** all member variables are private with `_` prefix.
3. **Interfaces:** sensors implement `ISensor`; screens inherit `Screen`.
4. **Composition:** classes own library instances instead of inheriting them.
5. **Dependency Injection:** dependencies are passed through constructors.

## 6. Coding Conventions

- One class per header/source pair.
- Use `#pragma once` in headers.
- Include the class header first in its `.cpp` file.
- Import heavy libraries in `.cpp`, not `.h`.
- Use PascalCase for file and class names.
- Use private members with `_` prefix (example: `_db`, `_ok`).
- Define constants in `config.h` or as `static constexpr`.
- Avoid `delay()` in `loop()`; use `millis()`.
- Avoid magic numbers; put them in `config.h`.
- Keep `main.ino` limited to object creation and `begin()`/`update()` calls.

## 7. State Machine

- Modes: `HOME`, `STUDY`, `SLEEP`, `RELAX`.
- `Btn1`: cycle through modes.
- `Btn2–5`: handled locally by each screen.
- Transition order: `onExit()` → `onEnter()` → `update()`.

```
[HOME] ──Btn1──► [STUDY] ──Btn1──► [SLEEP] ──Btn1──► [RELAX] ──Btn1──► [HOME]
```

## 8. Data Flow

- Sensors read data in `hal/`.
- `DataCollector` builds JSON payloads.
- `FirebaseClient` uploads data to Firebase RTDB.
- Publish interval: `30 seconds`.

## 9. Feature Scope

### In Scope

- Pomodoro timer with configurable 25/5 minute cycles.
- Seminar recording with AI evaluation.
- Sleep monitoring using sound, light, and duration.
- Sleep alarm via DS3231.
- Home screen showing real-time sound and light.
- Flappy Bird mini-game on relax screen.
- Music player from SD card.
- Firebase syncing of sensor data.

### Out of Scope for v1

- Web dashboard frontend.
- OTA firmware updates.
- Multi-device support.
- Touch-screen input.

## 10. Non-Functional Requirements

- UI refresh rate: ≥ 2 Hz for monitoring screens, ≥ 30 fps for game.
- Sensor read latency: < 100 ms.
- Firebase sync interval: 30 s.
- Seminar upload timeout: 10 s with error display on failure.
- Boot time: < 5 s to reach HOME screen.
- Button debounce: 20 ms.
