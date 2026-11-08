#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Scanning I2C bus...");

  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      Serial.printf("Device found at 0x%02X\n", address);
    }
  }

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75); // WHO_AM_I register

  if (Wire.endTransmission(false) == 0 &&
      Wire.requestFrom(MPU_ADDR, 1) == 1) {
    Serial.printf("WHO_AM_I = 0x%02X\n", Wire.read());
  } else {
    Serial.println("MPU6050 read failed");
  }
}

void loop() {
}
