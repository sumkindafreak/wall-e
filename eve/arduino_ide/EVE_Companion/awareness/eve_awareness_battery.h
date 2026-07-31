/**
 * EVE Phase O-3 — battery health bands (stable API; thresholds in config / battery_monitor).
 */
#pragma once

#include <stdint.h>
#include "battery_monitor.h"

typedef enum {
  EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN = 0,
  EVE_AWARENESS_BATTERY_HEALTH_OK,
  EVE_AWARENESS_BATTERY_HEALTH_WARN,
  EVE_AWARENESS_BATTERY_HEALTH_CRITICAL,
} EveAwarenessBatteryHealth;

static inline uint8_t eveAwarenessBatteryHealthFromStatus(EveBatStatus status, bool valid) {
  if (!valid) {
    return EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN;
  }
  switch (status) {
    case EVE_BAT_OK:
      return EVE_AWARENESS_BATTERY_HEALTH_OK;
    case EVE_BAT_WARN:
      return EVE_AWARENESS_BATTERY_HEALTH_WARN;
    case EVE_BAT_CRITICAL:
      return EVE_AWARENESS_BATTERY_HEALTH_CRITICAL;
    default:
      return EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN;
  }
}

static inline const char* eveAwarenessBatteryHealthName(uint8_t health) {
  switch (health) {
    case EVE_AWARENESS_BATTERY_HEALTH_OK:
      return "OK";
    case EVE_AWARENESS_BATTERY_HEALTH_WARN:
      return "WARN";
    case EVE_AWARENESS_BATTERY_HEALTH_CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

/** Derived: positive charge current into pack (battery subsystem fact). */
static inline bool eveAwarenessChargingFromCurrentA(float currentA, bool valid,
                                                    float minChargeA) {
  return valid && currentA > minChargeA;
}

/** Derived: SoC at or below low threshold. */
static inline bool eveAwarenessBatteryLowFromPercent(int8_t percent, int lowPctThreshold) {
  return percent >= 0 && percent <= lowPctThreshold;
}
