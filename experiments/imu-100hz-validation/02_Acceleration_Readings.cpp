#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Wake up MPU6050
  writeRegister(0x6B, 0x00);

  // Accelerometer range: +/- 8g
  writeRegister(0x1C, 0x10);

  // Low-pass filter: approximately 44 Hz
  writeRegister(0x1A, 0x03);

  Serial.println("MPU6050 ready!");
  Serial.println("ax, ay, az, SVM (g)");
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);

  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(MPU_ADDR, 6) != 6) {
    Serial.println("Sensor read failed");
    delay(200);
    return;
  }

  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();

  // Convert raw values into g
  float ax = rawX / 4096.0f;
  float ay = rawY / 4096.0f;
  float az = rawZ / 4096.0f;

  // Signal Vector Magnitude
  float svm = sqrtf(ax*ax + ay*ay + az*az);

  Serial.printf(
    "ax=%.3f ay=%.3f az=%.3f SVM=%.3f g\n",
    ax, ay, az, svm
  );

  delay(200);
}
