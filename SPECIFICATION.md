# Feature Specification: SmartClock MVP — Focus & Sleep

**Feature Branch**: `[###-smartclock-mvp]`

**Created**: 2026-06-08

**Status**: Draft

**Input**: User description: "Build an AIoT SmartClock that helps users focus and improve sleep quality.

For the MVP (first release), only implement:
1. Pomodoro Timer
   - Start, pause, reset timer
   - Configure focus and break durations
   - Track completed sessions
   - Sound notification when a session ends
2. Sleep Monitoring
   - Record sleep start and end times
   - Calculate total sleep duration
   - Measure environmental light and noise levels
   - Generate a basic sleep quality score
   - Display simple recommendations

Future releases may include:
- Seminar Practice
- Alarm Management
- Music Player
- Flappy Bird
- Smart Home integration

The goal of the MVP is to validate the core value of helping users focus and sleep better.

Out of scope:
- Seminar Practice
- Alarm Settings
- Music Player
- Flappy Bird"

## Clarifications

- **Pomodoro durations**: Focus interval 5–120 min, default 25 min; break interval 1–30 min, default 5 min.
- **Session tracking**: Completed sessions are stored with timestamps and accumulated in local history.
- **Sleep quality scoring**: Use weighted metrics: duration 60%, noise 20%, light 20%; thresholds are Good >= 80, Fair 60–79, Poor < 60.
- **Sleep sampling frequency**: Light and noise are sampled every 60 seconds.
- **Power-loss persistence**: Active Pomodoro and sleep sessions are persisted and recovered after reboot using RTC timestamps.
- **Server sync**: Includes Pomodoro history, sleep history, and device settings; append-only uploads, no conflict resolution.
- **Alarm support**: Future releases support multiple one-time alarms only; no repeat schedules or snooze in MVP.
- **Local history**: Stored in ESP32 flash and retained for 30 days or up to 1,000 records.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Pomodoro Focus Timer (Priority: P1)

A student wants a simple timer that supports focus sessions and breaks without using a phone.

**Why this priority**: Core focus support is the primary value of the MVP.

**Independent Test**: Configure a focus and break interval, start the timer, pause it, resume it, and verify the timer state and alert behavior.

**Acceptance Scenarios**:
1. **Given** the device is on the Pomodoro screen, **when** the user sets a focus duration and starts the timer, **then** the countdown begins and updates on screen.
2. **Given** the timer is running, **when** the user presses pause, **then** the countdown stops and can resume from the same remaining time.
3. **Given** the session ends, **when** the timer reaches zero, **then** the device emits a sound notification and switches to the break or idle state.
4. **Given** the user resets the timer, **when** the reset action is triggered, **then** the timer returns to the configured focus duration and clears the current session state.

---

### User Story 2 - Completed Session Tracking (Priority: P1)

A user wants to see how many Pomodoro sessions were completed during the day.

**Why this priority**: Tracking completed sessions provides feedback on focus progress.

**Independent Test**: Complete several focus sessions and verify the completed session counter increments correctly.

**Acceptance Scenarios**:
1. **Given** a session ends normally, **when** completion is recorded, **then** the device increments the daily completed session count.
2. **Given** the user resets the timer mid-session, **when** the session is aborted, **then** the completed session count does not increase.

---

### User Story 3 - Sleep Monitoring Start/End (Priority: P1)

A user wants to record when they go to sleep and when they wake up so the device can calculate total sleep time.

**Why this priority**: Sleep duration is the essential metric for the sleep monitoring MVP.

**Independent Test**: Start sleep monitoring at bedtime, stop it in the morning, and verify the recorded sleep interval and duration.

**Acceptance Scenarios**:
1. **Given** sleep monitoring is enabled, **when** the user marks bedtime, **then** the device records the sleep start time.
2. **Given** the user marks wake-up time, **when** sleep monitoring stops, **then** the device calculates and displays total duration.

---

### User Story 4 - Environmental Sleep Metrics (Priority: P1)

A user wants to record ambient light and noise during sleep to help evaluate sleep quality.

**Why this priority**: Environmental data helps generate a meaningful sleep quality score.

**Independent Test**: Simulate light and noise readings during a sleep interval and verify the device records them and includes them in the sleep summary.

**Acceptance Scenarios**:
1. **Given** sleep monitoring is active, **when** light and noise sensors are sampled, **then** the device stores the metrics with the sleep record.
2. **Given** the user views the sleep summary, **when** the sleep interval is complete, **then** the summary displays the average or peak environmental values.

---

### User Story 5 - Sleep Quality Feedback (Priority: P1)

A user wants a simple sleep quality score and a short recommendation based on duration and environment.

**Why this priority**: Quality feedback makes sleep monitoring actionable.

**Independent Test**: Complete a sleep session with sample metrics and verify the score and recommendation appear.

**Acceptance Scenarios**:
1. **Given** a completed sleep record, **when** the device generates feedback, **then** it displays a sleep quality score and one recommendation.
2. **Given** the sleep duration is short or environmental metrics are poor, **when** the feedback is displayed, **then** the recommendation highlights the issue.

---

### Edge Cases

- What happens when sleep stop is never recorded?
- How does the Pomodoro timer behave if the device loses power mid-session?
- How are sensor failures handled during sleep monitoring?
- How should the device respond if the user tries to configure invalid duration values?
- What happens if the user pauses the timer and leaves it paused overnight?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide a Pomodoro timer with start, pause, reset, and configurable focus/break durations. Focus interval range is 5–120 min (default 25 min); break interval range is 1–30 min (default 5 min).
- **FR-002**: System MUST track completed Pomodoro focus sessions, store them with timestamps, and accumulate them in local history.
- **FR-003**: System MUST emit a sound notification when a Pomodoro session ends.
- **FR-004**: System MUST support sleep monitoring with explicit sleep start and end actions.
- **FR-005**: System MUST calculate total sleep duration from recorded start/end times.
- **FR-006**: System MUST record ambient light and noise levels during sleep monitoring, sampling both every 60 seconds.
- **FR-007**: System MUST generate a basic sleep quality score using weighted metrics (duration 60%, noise 20%, light 20%) and display a simple recommendation with Good/Fair/Poor thresholds.
- **FR-008**: System MUST provide an offline, button-driven interface for the MVP features.
- **FR-009**: System MUST keep Pomodoro and sleep monitoring operational without network connectivity.
- **FR-010**: System MUST preserve core state across power cycles and recover active sessions after reboot using RTC timestamps.
- **FR-011**: System MUST retain local history in ESP32 flash for 30 days or up to 1,000 records.

### Non-functional Requirements

- **NFR-001**: UI interactions for Pomodoro and sleep screens must respond in under 200ms.
- **NFR-002**: The MVP must be designed to use low power during idle and sleep-monitoring modes.
- **NFR-003**: The interface must remain simple and usable with five physical buttons.
- **NFR-004**: The system architecture must remain extensible for future Seminar Practice, Alarms, Music, and games.

### Key Entities

- **PomodoroSession**: Represents a focus interval, break interval, state (running/paused), remaining time, and completion status.
- **SessionSummary**: Tracks daily completed focus sessions and total focus time.
- **SleepRecord**: Represents a sleep interval with start time, end time, total duration, light level history, noise level history, quality score, and recommendation.
- **DeviceState**: Represents current timer state, active screen, and temporary sleep monitoring state.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can start, pause, and reset a Pomodoro timer within 5 button presses.
- **SC-002**: The device emits a notification when a Pomodoro session ends.
- **SC-003**: Sleep duration is calculated and displayed after a sleep interval is stopped.
- **SC-004**: Environmental light and noise values are recorded during sleep sessions.
- **SC-005**: The device presents a sleep quality score and at least one recommendation for each completed sleep record.
- **SC-006**: Core MVP features work fully offline without network access.

## Assumptions

- The MVP will run entirely on an ESP32-based board with a TFT display, buttons, RTC, light sensor, microphone, and speaker.
- No network connectivity or server synchronization is required for MVP validation.
- Seminar practice, alarms, music playback, and mini-games are intentionally excluded from the first release.
- Sleep quality scoring will use weighted metrics with duration 60%, noise 20%, and light 20%.
- Local history is retained in ESP32 flash for 30 days or up to 1,000 records.
- Future sync support will upload Pomodoro history, sleep history, and device settings in append-only form with no conflict resolution.
- Future alarm support will be limited to multiple one-time alarms; repeat schedules and snooze are not part of MVP.
