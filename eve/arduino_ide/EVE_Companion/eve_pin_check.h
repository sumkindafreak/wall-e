/**
 * Boot-time GPIO sanity check (plain C++, unit-tested on a PC).
 *
 * Every ENABLED driver lists the pins it will drive. Two drivers on one GPIO, a GPIO the
 * ESP32-S3 N16R8 module reserves for flash / octal PSRAM, or a GPIO that does not exist is a
 * configuration error: EVE raises the blocking PIN_CONFLICT fault instead of letting two
 * drivers fight over a pin. Pins that are meant to be shared (one I2C bus, one SPI bus) carry the
 * same non-zero group number and may repeat.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

enum : uint8_t {
  EVE_PINGRP_EXCLUSIVE = 0,
  EVE_PINGRP_I2C_SDA = 1,
  EVE_PINGRP_I2C_SCL = 2,
  EVE_PINGRP_SPI_MOSI = 3,
  EVE_PINGRP_SPI_SCK = 4,
  EVE_PINGRP_SPI_MISO = 5,
};

typedef struct {
  const char* name;
  int pin;       /* -1 = not used (ignored) */
  uint8_t group; /* EVE_PINGRP_* */
} EvePinUse;

/** True for GPIOs the ESP32-S3 has and this module does not reserve (N16R8: 26-32 flash, 35-37 PSRAM). */
bool evePinUsableOnS3(int pin);

/**
 * Check a pin list. Returns the number of problems found; the first one is described in msg.
 */
int evePinCheck(const EvePinUse* uses, size_t n, char* msg, size_t msgCap);
