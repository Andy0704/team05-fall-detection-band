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
