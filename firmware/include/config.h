// Sampling and sensor-configuration constants.
// Fall-rule thresholds (free-fall, impact, tilt, ...) are not implemented
// yet and will be added here once there is data behind them.
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
