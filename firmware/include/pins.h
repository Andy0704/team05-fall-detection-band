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

// LCD1602 (QAPASS 1602A, parallel HD44780, no I2C backpack), button, and
// buzzer. Chosen from the ESP32-S3-DevKitC-1 left header row (GPIO
// 4/5/6/7/15/16/17/18/8/3/46/9/10/11/12/13/14), skipping pins already used
// (4, 8, 9) and the strapping pins (3, 46). Wired 2026-11-09.
// LCD is driven in 4-bit mode: RS, EN, D4-D7. RW is tied to GND (write
// only). V0 (contrast) goes through a potentiometer, not a GPIO.
constexpr int LCD_RS_PIN = 5;
constexpr int LCD_EN_PIN = 6;
constexpr int LCD_D4_PIN = 7;
constexpr int LCD_D5_PIN = 15;
constexpr int LCD_D6_PIN = 16;
constexpr int LCD_D7_PIN = 17;
constexpr int BUTTON_PIN = 18;
constexpr int BUZZER_PIN = 10;

// MPU6050 register map (datasheet / register map doc, not measured).
constexpr uint8_t MPU6050_ADDR = 0x68;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_WHO_AM_I = 0x75;
constexpr uint8_t WHO_AM_I_EXPECTED = 0x68;
constexpr uint8_t REG_CONFIG = 0x1A;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
