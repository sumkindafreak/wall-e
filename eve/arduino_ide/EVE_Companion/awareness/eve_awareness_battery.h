/**
 * EVE Phase O-3 — battery health bands (stable API; thresholds in config / battery_monitor).
 */
#pragma once

#include <stdint.h>

typedef enum {
  EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN = 0,
  EVE_AWARENESS_BATTERY_HEALTH_OK,
  EVE_AWARENESS_BATTERY_HEALTH_WARN,
  EVE_AWARENESS_BATTERY_HEALTH_CRITICAL,
} EveAwarenessBatteryHealth;

/** Match EveBatStatus in battery_monitor.h (host tests use these literals). */
#define EVE_AWARENESS_BAT_STATUS_OK 0
#define EVE_AWARENESS_BAT_STATUS_WARN 1
#define EVE_AWARENESS_BAT_STATUS_CRITICAL 2
#define EVE_AWARENESS_BAT_STATUS_UNKNOWN 3

static inline uint8_t eveAwarenessBatteryHealthFromStatus(int status, bool valid) {
  if (!valid) {
    return EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN;
  }
  switch (status) {
    case EVE_AWARENESS_BAT_STATUS_OK:
      return EVE_AWARENESS_BATTERY_HEALTH_OK;
    case EVE_AWARENESS_BAT_STATUS_WARN:
      return EVE_AWARENESS_BATTERY_HEALTH_WARN;
    case EVE_AWARENESS_BAT_STATUS_CRITICAL:
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
