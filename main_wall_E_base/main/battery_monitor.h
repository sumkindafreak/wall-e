#pragma once

// ============================================================
// WALL-E Battery / rail monitor
// ============================================================

#include <Arduino.h>
#include "base_board_pins.h"

#define BAT_ADC_PIN     BASE_PIN_BATTERY_ADC
#define CUR_ADC_PIN     BASE_PIN_CURRENT_ADC

#define VOLTAGE_SENSOR_OUTPUT_MAX_V  5.0f

#define BAT_R1          10000
#define BAT_R2          20000

#define BATTERY_MAX_V   5.2f
#define BATTERY_MIN_V   4.0f
#define BAT_V_CALIB     (4.92f/3.26f)

#define CUR_ZERO_V      1.65f
#define CUR_SENSITIVITY_V_PER_A  0.122f
#define CUR_SAMPLES     8

#define ADC_REF_V       3.3f
#define ADC_RESOLUTION  4095

#define BAT_WARN_PCT    30
#define BAT_CRIT_PCT    15

#define BAT_POLL_MS     10000
#define BAT_SAMPLES     16

typedef enum {
  BAT_OK       = 0,
  BAT_WARNING  = 1,
  BAT_CRITICAL = 2,
  BAT_UNKNOWN  = 3
} BatStatus;

struct BatteryData {
  float     voltage;
  float     currentA;
  int       percent;
  BatStatus status;
  bool      valid;
};

void               batteryInit();
bool               batteryHandle();
const BatteryData& batteryGetData();
String             batteryGetStatusJSON();
