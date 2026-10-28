/**
 * EVE battery monitor — reads the INA219 (or the legacy ADC divider) and hands each sample to
 * eve_battery_logic for filtering, plausibility checks and hysteresis.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "eve_control_core.h" /* EveBatLevel, EveBatteryReport */

typedef enum {
  EVE_BAT_OK = 0,
  EVE_BAT_WARN = 1,
  EVE_BAT_CRITICAL = 2,
  EVE_BAT_UNKNOWN = 3,
} EveBatStatus;

void eveBatteryInit(void);
/** Rate-limited sensor read (EVE_BAT_POLL_MS); call from loop(). Returns true when a sample was taken. */
bool eveBatteryTick(void);

/** Sensor detected / configured and answering. */
bool eveBatteryHardwareEnabled(void);
/** Latest sample was plausible. */
bool eveBatteryDataValid(void);
/** Filtered pack voltage (last raw value when the latest sample is invalid). */
float eveBatteryVoltage(void);
/** Filtered current in the sensor's own sign convention (A). */
float eveBatteryCurrentA(void);
int eveBatteryPercent(void);
EveBatStatus eveBatteryStatus(void);
bool eveBatteryIsCritical(void);

/** Trustworthy level for the state machine (UNKNOWN when invalid / no sensor). */
EveBatLevel eveBatteryLevel(void);
/** -1 unknown (charge direction not configured or no current reading), 0 no, 1 yes. */
int8_t eveBatteryCharging(void);
/** Measured full charge (only possible when charging is measurable). */
bool eveBatteryFull(void);
/** Why the latest sample was rejected ("ok" when valid). */
const char* eveBatteryInvalidReason(void);
/** Fill the heartbeat report. */
void eveBatteryFillReport(EveBatteryReport* out);
