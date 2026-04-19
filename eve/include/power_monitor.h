/**
 * EVE power monitor — battery voltage (voltage divider) + ACS712 current sensor.
 *
 * Hardware assumptions (ESP32-S3, 3.3 V system):
 *   • Voltage: battery → R1/R2 divider → ADC pin.
 *   • Current: ACS712 OUT pin → ADC pin (output centred at ~1.65 V at 0 A).
 *
 * All calibration constants live at the top of this header so you can tune them
 * in one place without hunting through source files.
 *
 * Feature gate: set EVE_ENABLE_POWER_MONITOR 1 in config.h.
 * When disabled every public function compiles to a safe no-op so the rest of the
 * firmware can call them unconditionally.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Calibration constants — edit these to match your hardware
// ---------------------------------------------------------------------------

/**
 * Voltage divider ratio: R2 / (R1 + R2).
 *   Example with R1 = 30 kΩ and R2 = 10 kΩ:  ratio = 10 / (30 + 10) = 0.25
 *   → A 12 V battery gives 3.0 V at the ADC pin (within 3.1 V range).
 *   Adjust so the maximum expected battery voltage × ratio < ADC_REF_V.
 */
#define VOLTAGE_DIVIDER_RATIO        0.25f

/**
 * Battery voltage range used to estimate 0 – 100 %.
 * For a single-cell Li-ion: 3.0 V empty → 4.2 V full.
 * For a 3S Li-ion pack:     9.0 V empty → 12.6 V full.
 * Change to match your actual battery pack.
 */
#define BATTERY_V_MIN                3.0f
#define BATTERY_V_MAX                4.2f

/** ESP32-S3 ADC: 12-bit resolution, DB_11 attenuation ≈ 3.1 V full-scale. */
#define ADC_BITS                     12
#define ADC_COUNTS                   4095        /* (1 << ADC_BITS) - 1 */
#define ADC_REF_V                    3.1f        /* V at full ADC count with DB_11 */

/**
 * ACS712 zero-current output voltage at the ADC pin.
 * With a 3.3 V supply the sensor outputs Vcc/2 = 1.65 V at 0 A.
 * If your sensor uses a different supply or you have a level-shift divider,
 * adjust this value by measuring the ADC pin voltage at known 0 A.
 */
#define ACS712_ZERO_OFFSET_V         1.65f

/**
 * ACS712 sensitivity in volts per ampere at the ADC pin.
 *   5 A variant:  185 mV/A → 0.185
 *  20 A variant:  100 mV/A → 0.100
 *  30 A variant:   66 mV/A → 0.066
 * If your sensor output goes through a voltage divider before the ADC, multiply
 * the above value by that divider ratio.
 */
#define ACS712_SENSITIVITY_V_PER_A   0.185f

/**
 * Exponential Moving Average smoothing factor (0 < α ≤ 1).
 * Lower values = smoother but slower to react to real changes.
 * 0.1 gives roughly a 10-sample lag at 20 Hz update rate.
 */
#define POWER_EMA_ALPHA              0.1f

/** Number of samples in the moving-average window applied after EMA. */
#define POWER_MA_WINDOW              8

/** How often powerMonitorTick() actually samples the ADC (ms). 50 ms = 20 Hz. */
#define POWER_TICK_INTERVAL_MS       50

// ---------------------------------------------------------------------------
// Battery state thresholds (all in percent, 0-100)
// ---------------------------------------------------------------------------
/** Battery is considered FULL above this percentage. */
#define BATTERY_PERCENT_FULL_THRESH      95
/** Battery is considered LOW below this percentage. */
#define BATTERY_PERCENT_LOW_THRESH       30
/** Battery is considered CRITICAL below this percentage. */
#define BATTERY_PERCENT_CRITICAL_THRESH  10

/**
 * Charging detection current thresholds (amperes).
 * A negative current (current flowing INTO the battery) above CHARGING_A
 * magnitude triggers BATTERY_CHARGING.  Hysteresis prevents rapid toggling.
 * Convention: positive current = discharging (load drawing power).
 *             negative current = charging (charger pushing current in).
 */
#define BATTERY_CHARGING_CURRENT_A   (-0.10f)   /* threshold: charging if I <= this */
#define BATTERY_CHARGING_HYST_A      0.05f       /* hysteresis band */

// ---------------------------------------------------------------------------
// Named battery states
// ---------------------------------------------------------------------------
enum class BatteryState : uint8_t {
  BATTERY_OK       = 0,  /* normal operation */
  BATTERY_LOW      = 1,  /* below LOW_THRESH — warn */
  BATTERY_CRITICAL = 2,  /* below CRITICAL_THRESH — protect */
  BATTERY_CHARGING = 3,  /* charger detected */
  BATTERY_FULL     = 4   /* fully charged */
};

// ---------------------------------------------------------------------------
// Power status data structure (sent to WALL-E and used for local reactions)
// ---------------------------------------------------------------------------
struct PowerStatus {
  float        voltage;       /* filtered battery voltage (V) */
  float        current;       /* filtered current (A): positive = draw, negative = charging */
  uint8_t      percent;       /* 0-100 battery estimate */
  bool         charging;      /* true while charger is pushing current */
  BatteryState state;         /* named state */
  uint32_t     heartbeat;     /* increments on every powerMonitorTick() call */
  uint32_t     timestamp_ms;  /* millis() at last update */
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/** Initialise ADC pins and pre-fill filters. Call once from setup(). */
void    powerMonitorInit(void);

/**
 * Read ADC, update filters, recompute battery state, fire reaction hooks if
 * state changed.  Call from loop() — internally rate-limited to
 * POWER_TICK_INTERVAL_MS so safe to call every loop iteration.
 */
void    powerMonitorTick(void);

/** Return the most recently filtered battery voltage (V). */
float   readVoltage(void);

/** Return the most recently filtered current (A). Negative = charging. */
float   readCurrent(void);

/** Return battery percentage 0-100, derived from filtered voltage. */
uint8_t getBatteryPercent(void);

/**
 * Recompute and store the battery state from the current voltage / current /
 * percent values.  Called automatically by powerMonitorTick(); exposed for
 * testing or external triggering.
 */
void    updateBatteryState(void);

/** Return a const reference to the latest PowerStatus snapshot. */
const PowerStatus& getPowerStatus(void);

// ---------------------------------------------------------------------------
// Local reaction hooks — called automatically when battery state changes.
// Safe no-ops when EVE_ENABLE_POWER_MONITOR == 0.
// Wire up NeoPixel patterns, servo poses, audio clips, etc. inside each.
// ---------------------------------------------------------------------------
void onBatteryOk(void);
void onBatteryLow(void);
void onBatteryCritical(void);
void onBatteryCharging(void);
void onBatteryFull(void);
