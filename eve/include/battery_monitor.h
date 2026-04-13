#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  EVE_BAT_OK = 0,
  EVE_BAT_WARN = 1,
  EVE_BAT_CRITICAL = 2,
  EVE_BAT_UNKNOWN = 3,
} EveBatStatus;

void eveBatteryInit(void);
/** Rate-limited ADC read; call from main loop. */
void eveBatteryTick(void);

bool eveBatteryHardwareEnabled(void);
bool eveBatteryDataValid(void);
float eveBatteryVoltage(void);
float eveBatteryCurrentA(void);
int eveBatteryPercent(void);
EveBatStatus eveBatteryStatus(void);

bool eveBatteryIsCritical(void);
