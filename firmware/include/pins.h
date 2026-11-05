// Pin and register constants for the fall-detection band.
// Sources are noted per constant. Do not invent new pin numbers here;
// confirm with the team and update CLAUDE.md when wiring changes.
#pragma once

#include <cstdint>

// I2C bus (CLAUDE.md "Hardware" section).
constexpr int I2C_SDA_PIN = 8;
constexpr int I2C_SCL_PIN = 9;

// MPU6050 INT pin (CLAUDE.md "Hardware" section, wired 2026-11-05).
constexpr int MPU6050_INT_PIN = 4;

// MPU6050 register map (datasheet / register map doc, not measured).
constexpr uint8_t MPU6050_ADDR = 0x68;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_WHO_AM_I = 0x75;
constexpr uint8_t WHO_AM_I_EXPECTED = 0x68;
constexpr uint8_t REG_CONFIG = 0x1A;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
