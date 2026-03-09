#pragma once

// ============================================================
//  WALL-E Battery Monitor
//
//  VOLTAGE: 5V rail sensor — displays 5V rail health (same battery → buck reg → 5V)
//   Sensor on 5V rail: 5V output when rail is 5V. Use divider so 5V max → 3.3V at ADC.
//
//  CURRENT: Sensor on 12V line (same battery) — measures amps drawn
//   2.5V at 0A, divider → ADC. current_A = (ADC_V - CUR_ZERO_V) / CUR_SENSITIVITY.
//   Both voltage (5V) and current (12V line) sync: same battery, same poll cycle.
// ============================================================

#include <Arduino.h>

// --- Voltage: 5V rail sensor → divider → ADC ---
#define BAT_ADC_PIN     1       // GPIO 1
#define VOLTAGE_SENSOR_OUTPUT_MAX_V  5.0f   // Sensor outputs 5V when 5V rail is 5V

// Divider on voltage sensor (5V max → 3.3V at ADC). R1=10k sensor→ADC, R2=20k ADC→GND
#define BAT_R1          10000
#define BAT_R2          20000

// --- 5V rail parameters (what you're measuring) ---
#define BATTERY_MAX_V   5.2f    // 5V rail healthy (slight headroom)
#define BATTERY_MIN_V   4.0f    // 5V rail low / regulator dropout
// Calibrated: display was 4.00V, multimeter 4.91V → BAT_V_CALIB = 4.91/4.0
#define BAT_V_CALIB     (4.91f/4.0f)

// --- Current: 12V-line sensor (same battery, measures amps) ---
#define CUR_ADC_PIN     2       // GPIO 2
#define CUR_ZERO_V      1.65f   // ADC voltage at 0A (2.5V sensor output through 10k/20k divider = 1.65V)
#define CUR_SENSITIVITY_V_PER_A  0.122f  // V per A at ADC (e.g. 185mV/A sensor: 0.185 * 3.3/5 ≈ 0.122)
#define CUR_SAMPLES     8

// --- ADC reference ---
#define ADC_REF_V       3.3f    // ESP32-S3 ADC reference
#define ADC_RESOLUTION  4095    // 12-bit

// --- Alert thresholds (%) ---
#define BAT_WARN_PCT    30      // amber warning
#define BAT_CRIT_PCT    15      // red critical

// --- Poll interval ---
#define BAT_POLL_MS     10000   // every 10 seconds

// --- ADC smoothing samples ---
#define BAT_SAMPLES     16

typedef enum {
  BAT_OK       = 0,
  BAT_WARNING  = 1,
  BAT_CRITICAL = 2,
  BAT_UNKNOWN  = 3
} BatStatus;

struct BatteryData {
  float     voltage;     // measured pack voltage (V)
  float     currentA;    // measured current (A), positive = draw
  int       percent;     // 0-100
  BatStatus status;
  bool      valid;
};

void                  batteryInit();
bool                  batteryHandle();    // Call in loop(); returns true when a new reading was taken
const BatteryData&    batteryGetData();
String                batteryGetStatusJSON();
