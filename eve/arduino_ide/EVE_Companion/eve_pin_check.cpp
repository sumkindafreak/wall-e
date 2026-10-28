#include "eve_pin_check.h"

#include <stdio.h>

bool evePinUsableOnS3(int pin) {
  if (pin < 0 || pin > 48) return false;
  if (pin >= 22 && pin <= 25) return false; /* not bonded out on the ESP32-S3 */
  if (pin >= 26 && pin <= 32) return false; /* SPI flash */
  if (pin >= 35 && pin <= 37) return false; /* octal PSRAM (N16R8 / R8 modules) */
  return true;
}

int evePinCheck(const EvePinUse* uses, size_t n, char* msg, size_t msgCap) {
  int problems = 0;
  if (msg && msgCap) msg[0] = '\0';
  for (size_t i = 0; i < n; i++) {
    const EvePinUse& a = uses[i];
    if (a.pin < 0) continue;
    if (!evePinUsableOnS3(a.pin)) {
      if (problems == 0 && msg) snprintf(msg, msgCap, "%s uses GPIO%d, which is reserved or does not exist on the ESP32-S3 N16R8", a.name, a.pin);
      problems++;
      continue;
    }
    for (size_t j = i + 1; j < n; j++) {
      const EvePinUse& b = uses[j];
      if (b.pin != a.pin) continue;
      const bool sharedBus = a.group != EVE_PINGRP_EXCLUSIVE && a.group == b.group;
      if (sharedBus) continue;
      if (problems == 0 && msg) snprintf(msg, msgCap, "GPIO%d is configured for both %s and %s", a.pin, a.name, b.name);
      problems++;
    }
  }
  return problems;
}
