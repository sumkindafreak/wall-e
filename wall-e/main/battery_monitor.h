#pragma once

// ============================================================
//  WALL-E Battery Monitor
//
//  VOLTAGE: 5V-output sensor (e.g. module that outputs 0–5V for 0–12V battery).
//   Use a divider so 5V max → 3.3V at ADC (e.g. R1=10k sensor→ADC, R2=20k ADC→GND).
//   Sensor 5V → battery = BATTERY_MAX_V; code scales: battery_V = sensor_V * (BATTERY_MAX_V / 5).
//
//  CURRENT: Sensor on 12V line; most modules output 0–5V (or 2.5V at 0A for bidirectional).
//   Use a divider so 5V max → 3.3V at ADC. 0A = CUR_ZERO_V (1.65V at ADC = 2.5V sensor).
//   current_A = (ADC_voltage - CUR_ZERO_V) / CUR_SENSITIVITY_V_PER_A.
// ============================================================

#include <Arduino.h>

// --- Voltage: 5V sensor output → divider → ADC ---
#define BAT_ADC_PIN     1       // GPIO 1
#define VOLTAGE_SENSOR_OUTPUT_MAX_V  5.0f   // Sensor outputs 0–5V for full battery range

// Divider on voltage sensor output (5V → 3.3V at ADC). Example: 10k (sensor→ADC), 20k (ADC→GND)
#define BAT_R1          10000   // 10kΩ  (sensor output to ADC pin)
#define BAT_R2          20000   // 20kΩ  (ADC pin to GND)  → 5V sensor gives 3.33V at ADC

// --- Battery pack parameters (12V) ---
#define BATTERY_MAX_V   12.6f   // 3S LiPo full; or 12.8f / 13.8f for lead-acid
#define BATTERY_MIN_V   10.5f   // Safe cutoff

// --- Current: 12V-line sensor, usually 5V output → divider → ADC ---
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
