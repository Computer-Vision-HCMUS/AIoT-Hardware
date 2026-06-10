# Implementation Plan: SmartClock MVP — Focus & Sleep

**Branch**: `[###-smartclock-mvp]` | **Date**: 2026-06-08 | **Spec**: `SPECIFICATION.md`

**Input**: Feature specification from current root spec file.

## Summary

Deliver an ESP32-S3 edge firmware MVP that validates the core value of helping users focus and sleep better. The device will support a customizable Pomodoro timer and a sleep monitoring flow with duration tracking, environmental sampling, and a simple quality score. All functionality must work offline, be controlled through five physical buttons, and preserve active state across reboot using RTC timestamps.

## Technical Context

**Language/Version**: C++ using the Arduino framework for ESP32.

**Primary Dependencies**:
- Arduino ESP32 core
- TFT display library (ILI9341 or equivalent)
- BH1750 light sensor library
- DS3231 RTC library
- I2S microphone support
- Preferences / SPIFFS / LittleFS for local persistence

**Storage**: ESP32 flash for local history, active state, and configuration.

**Testing**: Manual hardware validation on the ESP32 board; unit-level logic checks for timer and sleep score computations as feasible.

**Target Platform**: ESP32-S3 DevKitC-1 with TFT display, 5 physical buttons, microphone, BH1750 light sensor, DS3231 RTC, and onboard speaker.

**Project Type**: Embedded firmware / IoT edge device.

**Performance Goals**: UI interactions respond in under 200ms; offline-first behavior; low-power idle and sleep-monitoring operation.

**Constraints**: 5-button navigation, offline-only MVP, 30-day or 1,000-record history retention, sleep sampling every 60 seconds.

**Scale/Scope**: Single-device firmware MVP limited to Pomodoro and sleep monitoring; no alarms, music, seminar, or games in this release.

## Constitution Check

- Keep dependencies downward-only: `SmartDeskBuddy.ino` → `core`, `ui`, `hal`, `utils`.
- Maintain single responsibility per class/module.
- Use interface abstractions for sensors and screens.
- Prefer composition over inheritance for hardware wrappers.
- Use dependency injection through constructors.
- Avoid `delay()` in the main loop; use `millis()`.

## Project Structure

### Documentation (this feature)

```text
PLAN.md
SPECIFICATION.md
```

### Source Code (repository root)

```text
SmartDeskBuddy.ino
config.h

hal/
├── ISensor.h
├── LightSensor.h
├── LightSensor.cpp
├── RtcModule.h
├── RtcModule.cpp
├── SoundSensor.h
├── SoundSensor.cpp

ui/
├── Screen.h
├── PomodoroScreen.h
├── PomodoroScreen.cpp
├── SleepScreen.h
├── SleepScreen.cpp

core/
├── StateMachine.h
├── StateMachine.cpp
├── PomodoroTimer.h
├── PomodoroTimer.cpp
├── SleepMonitor.h
├── SleepMonitor.cpp

utils/
├── Logger.h
├── TimeUtils.h
```

**Structure Decision**: Use a single embedded firmware project with a clear HAL/core/UI split matching the project constitution. No separate backend or external service code is included in the MVP.

## Complexity Tracking

No constitution violations are expected for this MVP. The plan follows the existing architecture principles and keeps scope constrained to core features.
