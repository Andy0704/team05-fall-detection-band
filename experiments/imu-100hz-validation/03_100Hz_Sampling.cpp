#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

// Sampling settings
const uint32_t SAMPLE_PERIOD_US = 10000; // 10 ms = 100 Hz

uint32_t nextSampleUs;
uint32_t sampleNumber = 0;

uint32_t windowStartUs;
uint32_t windowSamples = 0;
uint32_t windowMissed = 0;

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool readAcceleration(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(MPU_ADDR, 6) != 6) {
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
  int16_t rawX = (xHigh << 8) | xLow;
  int16_t rawY = (yHigh << 8) | yLow;
  int16_t rawZ = (zHigh << 8) | zLow;

  ax = rawX / 4096.0f;
  ay = rawY / 4096.0f;
  az = rawZ / 4096.0f;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Wake up MPU6050
  writeRegister(0x6B, 0x00);

  // Accelerometer range: +/- 8 g
  writeRegister(0x1C, 0x10);

  // Low-pass filter: approximately 44 Hz
  writeRegister(0x1A, 0x03);

  Serial.println("# MPU6050 100 Hz sampling test");
  Serial.println("# sample,timestamp_us,ax_g,ay_g,az_g,svm_g");

  nextSampleUs = micros();
  windowStartUs = nextSampleUs;
}

void loop() {
  uint32_t now = micros();

  // Wait for the next scheduled sample
  if ((int32_t)(now - nextSampleUs) < 0) {
    return;
  }

  // Detect missed sampling periods
  uint32_t skipped =
      (now - nextSampleUs) / SAMPLE_PERIOD_US;

  windowMissed += skipped;
  sampleNumber += skipped + 1;
  nextSampleUs += (skipped + 1) * SAMPLE_PERIOD_US;

  float ax, ay, az;
  uint32_t timestamp = micros();

  if (readAcceleration(ax, ay, az)) {
    float svm = sqrtf(ax * ax + ay * ay + az * az);

    windowSamples++;

    Serial.printf(
      "%lu,%lu,%.4f,%.4f,%.4f,%.4f\n",
      (unsigned long)sampleNumber,
      (unsigned long)timestamp,
      ax, ay, az, svm
    );
  } else {
    Serial.println("# Sensor read failed");
  }

  // Report the actual measured rate every 5 seconds
  uint32_t elapsedUs = micros() - windowStartUs;

  if (elapsedUs >= 5000000UL) {
    float actualRate =
        windowSamples * 1000000.0f / elapsedUs;

    Serial.printf(
      "# Rate: %.2f Hz, missed ticks: %lu\n",
      actualRate,
      (unsigned long)windowMissed
    );

    windowStartUs = micros();
    windowSamples = 0;
    windowMissed = 0;
  }
}
