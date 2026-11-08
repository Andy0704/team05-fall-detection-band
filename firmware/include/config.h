// Sampling, sensor-configuration, and fall-rule constants.
// Values below marked "first guess" or "handout example" are starting
// points only; see CLAUDE.md "Named constants" and PROJECT_STATUS.md
// "Threshold register" before trusting them for a report.
#pragma once

#include <cstddef>
#include <cstdint>

// Design choice: 100 Hz target sampling rate (CLAUDE.md "Design decisions").
constexpr uint32_t SAMPLE_PERIOD_MS = 10;

// MPU6050 datasheet: +/-8 g range gives 4096 LSB/g sensitivity.
constexpr float ACCEL_SCALE_LSB_PER_G = 4096.0f;

// ACCEL_CONFIG (0x1C) bits 4:3 = FS_SEL. FS_SEL=2 selects +/-8 g
// (MPU6050 register map datasheet, CLAUDE.md "Sensor config").
constexpr uint8_t ACCEL_CONFIG_FS_SEL_8G = 0x10;

// CONFIG (0x1A) bits 2:0 = DLPF_CFG. DLPF_CFG=3 selects 44 Hz accelerometer
// bandwidth (MPU6050 register map datasheet table 7, CLAUDE.md first
// choice, to confirm with own data).
constexpr uint8_t CONFIG_DLPF_44HZ = 0x03;

// Design choice: buffers ~320 ms of samples at 100 Hz between the sampling
// task and the logging task.
constexpr size_t LOG_QUEUE_LENGTH = 32;

// Fall-detection rule (CLAUDE.md "Design decisions": free-fall -> impact
// within a window -> trunk tilt + inactivity). None of these are
// validated against real fall data yet (PROJECT_STATUS.md threshold
// register).

// Handout example: SVM below this marks the start of free-fall.
constexpr float FREEFALL_THRESHOLD_G = 0.4f;

// First guess: 5 samples at the 100 Hz target, must stay below
// FREEFALL_THRESHOLD_G for at least this long to count as a real
// free-fall (not just a glitch).
constexpr uint32_t FREEFALL_MIN_MS = 50;

// Handout example: SVM above this after a free-fall marks impact.
constexpr float IMPACT_THRESHOLD_G = 2.5f;

// Handout example: impact must occur within this long after free-fall
// is confirmed, or the episode is dropped.
constexpr uint32_t IMPACT_WINDOW_MS = 500;

// First guess: trunk tilt at impact must exceed this to count as a
// fall-like posture change, not just a hard footstep or bump.
constexpr float TILT_THRESHOLD_DEG = 60.0f;

// First guess: after a tilted impact, SVM must stay within
// STANDING_SVM_G +/- INACTIVITY_SVM_DEV_G for this long (tracked with a
// running min/max, no variance) before the fall is confirmed.
constexpr uint32_t INACTIVITY_MS = 2000;
constexpr float INACTIVITY_SVM_DEV_G = 0.15f;

// Nominal SVM at rest (gravity only, no motion).
constexpr float STANDING_SVM_G = 1.0f;

// Output/alarm (CLAUDE.md "Design decisions": buzzer + LCD "FALL
// DETECTED", cancel button, alarm ends on button press or timeout).

// First guess: how long the alarm sounds before auto-cancelling if the
// button is never pressed.
constexpr uint32_t ALARM_TIMEOUT_S = 30;

// Design choice: how often the output task polls the button and checks
// whether the display needs refreshing.
constexpr uint32_t OUTPUT_POLL_MS = 20;

// First guess: a raw button reading must stay stable for this long to
// count as a real press, not switch bounce.
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;

// Design choice: refresh the idle status line at most this often, so the
// display isn't rewritten every 10 ms sample (CLAUDE.md: "refreshes only
// on state changes and about once per second").
constexpr uint32_t DISPLAY_REFRESH_MS = 1000;

// Buzzer PWM (works for both active and passive buzzers: a passive
// buzzer sounds the tone itself, an active buzzer just gets switched at
// this rate). 2 kHz is an audible first guess, not measured against the
// actual buzzer yet.
constexpr uint8_t BUZZER_PWM_CHANNEL = 0;
constexpr uint32_t BUZZER_PWM_FREQ_HZ = 2000;
constexpr uint8_t BUZZER_PWM_RESOLUTION_BITS = 8;
constexpr uint32_t BUZZER_PWM_DUTY = 128; // ~50% of 2^8, first guess
