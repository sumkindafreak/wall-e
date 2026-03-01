// ==========================================================
// I2C Devices Implementation
// ==========================================================

#include "i2c_devices.h"
#include <Wire.h>

void i2cInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // Fast mode
  Wire.setTimeOut(50); // 50ms timeout to prevent hangs
  delay(100);
  Serial.println(F("[I2C] Init OK (400kHz, 50ms timeout)"));
}

void i2cReset() {
  Serial.println(F("[I2C] ⚠️  EMERGENCY RESET"));
  Wire.end();
  delay(100);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(50);
  delay(100);
  Serial.println(F("[I2C] Reset complete"));
}

void i2cScan() {
  Serial.println(F("\n--- I2C Bus Scan ---"));
  Serial.printf("SDA=GPIO%d  SCL=GPIO%d\n", I2C_SDA, I2C_SCL);
  
  int count = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  [0x%02X] found", addr);
      if (addr == ADS1115_ADDR) Serial.print(F(" (ADS1115)"));
      else if (addr == SX1509_ADDR) Serial.print(F(" (SX1509)"));
      else if (addr == 0x3C || addr == 0x3D) Serial.print(F(" (OLED?)"));
      Serial.println();
      count++;
    }
  }
  
  if (count == 0) {
    Serial.println(F("  ❌ No I2C devices found"));
  } else {
    Serial.printf("  ✓ Total: %d device(s)\n", count);
  }
  Serial.println(F("--- End Scan ---\n"));
}
