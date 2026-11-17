// Sampling task + logging task, feeding the fall-detection rule
// (firmware/src/fall_detector.cpp), plus the output/alarm task (LCD +
// buzzer + cancel button).

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "config.h"
#include "fall_detector.h"
#include "pins.h"

#ifndef FIRMWARE_GIT_COMMIT
#define FIRMWARE_GIT_COMMIT "unknown"
#endif

struct SampleMsg {
  uint32_t counter;
  uint32_t micros;
  float ax_g;
  float ay_g;
  float az_g;
  bool valid;
  SensorState state;
};

static QueueHandle_t g_logQueue;

// Simple one-writer-per-direction flags between samplingTask and
// outputTask: samplingTask publishes the latest state, outputTask
// requests a reset once the alarm ends. A single bool/enum read or
// write is small enough that this plain-volatile pattern (no mutex) is
// the simple option here (CLAUDE.md code rule 7).
static volatile SensorState g_latestState = SensorState::MONITORING;
static volatile bool g_resetRequested = false;

void scanI2cBus() {
  Serial.println("I2C scan starting...");
  int found = 0;
  for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found device at 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("I2C scan done, %d device(s) found.\n", found);
}

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool readWhoAmI(uint8_t &outValue) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_WHO_AM_I);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(MPU6050_ADDR, (uint8_t)1) != 1) {
    return false;
  }
  outValue = Wire.read();
  return true;
}

bool readAccelG(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(MPU6050_ADDR, (uint8_t)6) != 6) {
    return false;
  }
  // Read MSB/LSB into separate statements: the order in which the two
  // Wire.read() calls inside "(a << 8) | b" would execute is not
  // specified by C++, which could silently swap high/low bytes.
  const uint8_t xHigh = Wire.read();
  const uint8_t xLow = Wire.read();
  const uint8_t yHigh = Wire.read();
  const uint8_t yLow = Wire.read();
  const uint8_t zHigh = Wire.read();
  const uint8_t zLow = Wire.read();
  const int16_t rawX = (xHigh << 8) | xLow;
  const int16_t rawY = (yHigh << 8) | yLow;
  const int16_t rawZ = (zHigh << 8) | zLow;
  ax = rawX / ACCEL_SCALE_LSB_PER_G;
  ay = rawY / ACCEL_SCALE_LSB_PER_G;
  az = rawZ / ACCEL_SCALE_LSB_PER_G;
  return true;
}

// High-priority task. Never blocks on Serial or display; the only blocking
// call is the I2C transaction itself, which is the sensor read.
void samplingTask(void *) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);
  uint32_t counter = 0;
  FallDetector detector;

  for (;;) {
    vTaskDelayUntil(&lastWakeTime, period);
    counter++; // increments every scheduled tick, not after a successful read

    if (g_resetRequested) {
      detector.reset();
      g_resetRequested = false;
    }

    SampleMsg msg;
    msg.counter = counter;
    msg.micros = micros();
    msg.valid = readAccelG(msg.ax_g, msg.ay_g, msg.az_g);

    // Called every tick, valid or not: a read failure must reach the
    // detector so it can drop a partially-observed sequence instead of
    // silently skipping ticks (see fall_detector.cpp).
    msg.state = detector.update(msg.valid, msg.ax_g, msg.ay_g, msg.az_g,
                                  msg.micros);
    g_latestState = msg.state;

    xQueueSend(g_logQueue, &msg, 0); // non-blocking; drop if the queue is full
  }
}

// Lower-priority task. Serial is for debugging only (CLAUDE.md code rule 4);
// this is not the on-device alarm output.
void logTask(void *) {
  SampleMsg msg;
  for (;;) {
    if (xQueueReceive(g_logQueue, &msg, portMAX_DELAY) == pdTRUE) {
      if (msg.valid) {
        Serial.printf("%lu,%lu,%.4f,%.4f,%.4f,%s\n", msg.counter, msg.micros,
                       msg.ax_g, msg.ay_g, msg.az_g,
                       sensorStateName(msg.state));
      } else {
        Serial.printf("%lu,%lu,,,,%s\n", msg.counter, msg.micros,
                       sensorStateName(msg.state));
      }
    }
  }
}

// True once the raw button reading has been stable at LOW (pressed, with
// INPUT_PULLUP) for at least BUTTON_DEBOUNCE_MS.
bool debouncedButtonPressed(uint32_t nowMs) {
  static int lastRaw = HIGH;
  static uint32_t lastChangeMs = 0;
  static int stable = HIGH;

  const int raw = digitalRead(BUTTON_PIN);
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChangeMs = nowMs;
  }
  if ((nowMs - lastChangeMs) >= BUTTON_DEBOUNCE_MS) {
    stable = raw;
  }
  return stable == LOW;
}

// Lower-priority task. Owns the LCD, buzzer, and cancel button. Polls
// rather than blocks, per CLAUDE.md: "display and logging run in
// lower-priority tasks fed by a queue" (here fed by g_latestState) and
// "refreshes only on state changes and about once per second".
void outputTask(void *) {
  LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN,
                    LCD_D6_PIN, LCD_D7_PIN);
  lcd.begin(16, 2);
  lcd.print("MONITORING");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ledcSetup(BUZZER_PWM_CHANNEL, BUZZER_PWM_FREQ_HZ, BUZZER_PWM_RESOLUTION_BITS);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);

  bool alarmActive = false;
  uint32_t alarmStartMs = 0;
  SensorState lastShownState = SensorState::MONITORING;
  uint32_t lastDisplayUpdateMs = 0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(OUTPUT_POLL_MS));
    const uint32_t nowMs = millis();
    const SensorState state = g_latestState;

    if (!alarmActive && state == SensorState::FALL_DETECTED) {
      alarmActive = true;
      alarmStartMs = nowMs;
      ledcWrite(BUZZER_PWM_CHANNEL, BUZZER_PWM_DUTY);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FALL DETECTED");
      lcd.setCursor(0, 1);
      lcd.print("Press to cancel");
    }

    if (alarmActive) {
      const bool cancelled = debouncedButtonPressed(nowMs);
      const bool timedOut = (nowMs - alarmStartMs) >= (ALARM_TIMEOUT_S * 1000UL);
      if (cancelled || timedOut) {
        alarmActive = false;
        ledcWrite(BUZZER_PWM_CHANNEL, 0);
        g_resetRequested = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MONITORING");
        lastShownState = SensorState::MONITORING;
        lastDisplayUpdateMs = nowMs;
      }
      continue;
    }

    // Idle status line: only rewritten on a state change or once a
    // second, not every poll.
    if (state != lastShownState ||
        (nowMs - lastDisplayUpdateMs) >= DISPLAY_REFRESH_MS) {
      lcd.setCursor(0, 0);
      lcd.print("                "); // clear the 16-char line
      lcd.setCursor(0, 0);
      lcd.print(sensorStateName(state));
      lastShownState = state;
      lastDisplayUpdateMs = nowMs;
    }
  }
}

void printLogHeader() {
  Serial.printf(
      "# sample_period_ms=%lu dlpf_cfg=0x%02X(44Hz) accel_fs_sel=0x%02X(+/-8g) "
      "position=waist commit=%s build=%s %s\n",
      (unsigned long)SAMPLE_PERIOD_MS, CONFIG_DLPF_44HZ, ACCEL_CONFIG_FS_SEL_8G,
      FIRMWARE_GIT_COMMIT, __DATE__, __TIME__);
  Serial.println("counter,micros,ax,ay,az,state");
}

void setup() {
  Serial.begin(115200);
  delay(200); // let USB-CDC serial come up before first prints

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  scanI2cBus();

  writeRegister(REG_PWR_MGMT_1, 0x00); // wake from sleep

  uint8_t whoAmI = 0;
  if (!readWhoAmI(whoAmI) || whoAmI != WHO_AM_I_EXPECTED) {
    Serial.printf("WARNING: WHO_AM_I = 0x%02X, expected 0x%02X\n", whoAmI,
                   WHO_AM_I_EXPECTED);
  }

  writeRegister(REG_ACCEL_CONFIG, ACCEL_CONFIG_FS_SEL_8G);
  writeRegister(REG_CONFIG, CONFIG_DLPF_44HZ);

  printLogHeader();

  g_logQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(SampleMsg));

  xTaskCreatePinnedToCore(samplingTask, "Sampling", 4096, nullptr, 3, nullptr,
                            1);
  xTaskCreatePinnedToCore(logTask, "Log", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(outputTask, "Output", 4096, nullptr, 1, nullptr, 1);
}

void loop() {
  // All work happens in the FreeRTOS tasks created in setup().
  vTaskDelay(portMAX_DELAY);
}
