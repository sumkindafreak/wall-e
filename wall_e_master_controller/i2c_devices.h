// ==========================================================
// I2C Devices — Init + Scanner
// ==========================================================

#ifndef I2C_DEVICES_H
#define I2C_DEVICES_H

#include <Arduino.h>

// I2C Configuration
#define I2C_SDA 27
#define I2C_SCL 22

// I2C Addresses
#define ADS1115_ADDR 0x48
#define SX1509_ADDR  0x3E

void i2cInit();
void i2cScan();
void i2cReset(); // Emergency reset

#endif
