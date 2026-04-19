/**
 * EVE power monitor implementation.
 *
 * Reads battery voltage (via voltage divider) and current (via ACS712) on two
 * ESP32-S3 ADC pins.  Applies EMA + moving-average double-filtering for stable
 * readings, computes named battery states with hysteresis, and fires local
 * reaction hooks whenever the state changes.
 *
 * All calibration constants live in power_monitor.h.
 */

#include "power_monitor.h"
#include "config.h"
#include "neopixel_control.h"   /* NeoPixel patterns used in reaction hooks */

// ---------------------------------------------------------------------------
// Guard — entire implementation compiled away when feature is disabled
// ---------------------------------------------------------------------------
#if EVE_ENABLE_POWER_MONITOR

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

/** Exponential moving average accumulators. */
static float s_voltEma = 0.0f;
static float s_currEma = 0.0f;

/** Moving-average circular buffers (filled with EMA output). */
static float    s_voltBuf[POWER_MA_WINDOW];
static float    s_currBuf[POWER_MA_WINDOW];
static uint8_t  s_maIdx = 0;       /* shared write index for both buffers */

static bool        s_initialized = false;
static PowerStatus s_status      = {};
static BatteryState s_prevState  = BatteryState::BATTERY_OK;

/** Rate-limiter for ADC reads. */
static uint32_t s_lastTickMs = 0;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/** Convert 12-bit ADC count to voltage at the ADC pin. */
static inline float countsToVoltage(int counts) {
  return (float)counts * (ADC_REF_V / (float)ADC_COUNTS);
}

/**
 * Read an ADC pin multiple times with small delays and return the mean as a
 * voltage.  Multiple reads reduce noise from the ADC's sample-and-hold.
 */
static float adcAveragedVolts(int pin) {
  long sum = 0;
  for (int i = 0; i < POWER_MA_WINDOW; i++) {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }
  return countsToVoltage((int)(sum / POWER_MA_WINDOW));
}

/**
 * Update one moving-average circular buffer slot and return the window mean.
 * s_maIdx must already point to the next write slot.
 */
static float maInsert(float* buf, float newVal) {
  buf[s_maIdx] = newVal;
  float sum = 0.0f;
  for (int i = 0; i < POWER_MA_WINDOW; i++) {
    sum += buf[i];
  }
  return sum / (float)POWER_MA_WINDOW;
}

/** Clamp a float to [lo, hi]. */
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------------------------------------------------------------------
// State helper: map current BatteryState to a human-readable label
// ---------------------------------------------------------------------------
static const char* stateLabel(BatteryState s) {
  switch (s) {
    case BatteryState::BATTERY_OK:       return "OK";
    case BatteryState::BATTERY_LOW:      return "LOW";
    case BatteryState::BATTERY_CRITICAL: return "CRITICAL";
    case BatteryState::BATTERY_CHARGING: return "CHARGING";
    case BatteryState::BATTERY_FULL:     return "FULL";
    default:                             return "UNKNOWN";
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void powerMonitorInit(void) {
  /* Set 12-bit resolution and DB_11 attenuation (≈ 0 – 3.1 V) on both pins. */
  analogReadResolution(ADC_BITS);
  analogSetPinAttenuation(EVE_BAT_VOLT_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(EVE_BAT_CURR_ADC_PIN, ADC_11db);

#if EVE_CHARGER_PRESENT_PIN >= 0
  pinMode(EVE_CHARGER_PRESENT_PIN, INPUT_PULLUP);
#endif

  /* Seed EMA and MA buffers with initial readings so filters converge immediately. */
  float initAdcV = adcAveragedVolts(EVE_BAT_VOLT_ADC_PIN);
  float initV    = initAdcV / VOLTAGE_DIVIDER_RATIO;

  float initAdcC = adcAveragedVolts(EVE_BAT_CURR_ADC_PIN);
  float initC    = (initAdcC - ACS712_ZERO_OFFSET_V) / ACS712_SENSITIVITY_V_PER_A;

  s_voltEma = initV;
  s_currEma = initC;
  for (int i = 0; i < POWER_MA_WINDOW; i++) {
    s_voltBuf[i] = initV;
    s_currBuf[i] = initC;
  }
  s_maIdx = 0;

  /* Fill initial status. */
  s_status.voltage      = initV;
  s_status.current      = initC;
  s_status.percent      = getBatteryPercent();
  s_status.charging     = false;
  s_status.state        = BatteryState::BATTERY_OK;
  s_status.heartbeat    = 0;
  s_status.timestamp_ms = millis();

  s_prevState   = s_status.state;
  s_initialized = true;
  s_lastTickMs  = millis();

  Serial.println(F("[EVE][PowerMon] Initialised"));
  Serial.printf("[EVE][PowerMon] Seed: V=%.2f V  I=%.3f A  pct=%u%%\n",
                initV, initC, s_status.percent);
}

void powerMonitorTick(void) {
  if (!s_initialized) return;

  uint32_t now = millis();
  if ((now - s_lastTickMs) < (uint32_t)POWER_TICK_INTERVAL_MS) {
    return;  /* rate-limit ADC reads */
  }
  s_lastTickMs = now;

  /* Advance the shared MA window index once per tick so both buffers
     always write to the same slot each cycle. */
  s_maIdx = (s_maIdx + 1) % POWER_MA_WINDOW;

  /* ---- Voltage ---- */
  float rawAdcV = adcAveragedVolts(EVE_BAT_VOLT_ADC_PIN);
  float batV    = rawAdcV / VOLTAGE_DIVIDER_RATIO;
  s_voltEma     = POWER_EMA_ALPHA * batV + (1.0f - POWER_EMA_ALPHA) * s_voltEma;
  float filtV   = maInsert(s_voltBuf, s_voltEma);

  /* ---- Current ---- */
  float rawAdcC = adcAveragedVolts(EVE_BAT_CURR_ADC_PIN);
  float cur     = (rawAdcC - ACS712_ZERO_OFFSET_V) / ACS712_SENSITIVITY_V_PER_A;
  s_currEma     = POWER_EMA_ALPHA * cur + (1.0f - POWER_EMA_ALPHA) * s_currEma;
  float filtC   = maInsert(s_currBuf, s_currEma);

  /* ---- Store ---- */
  s_status.voltage      = filtV;
  s_status.current      = filtC;
  s_status.heartbeat++;
  s_status.timestamp_ms = now;

  /* Recompute derived values (percent, charging, state) and fire hooks. */
  updateBatteryState();
}

float readVoltage(void) {
  return s_status.voltage;
}

float readCurrent(void) {
  return s_status.current;
}

uint8_t getBatteryPercent(void) {
  /* Map filtered voltage linearly to 0-100 %, clamped to the valid range. */
  float v    = s_status.voltage;
  float span = BATTERY_V_MAX - BATTERY_V_MIN;
  if (span <= 0.0f) return 0;
  float pct = (v - BATTERY_V_MIN) / span * 100.0f;
  pct = clampf(pct, 0.0f, 100.0f);
  return (uint8_t)pct;
}

void updateBatteryState(void) {
  uint8_t pct = getBatteryPercent();
  s_status.percent = pct;

  /* ---- Charging detection with hysteresis ---- */
  float i = s_status.current;
  bool wasCharging = s_status.charging;

  /*
   * Charging convention: current is negative when the charger pushes current
   * into the battery (depends on your ACS712 orientation).
   * BATTERY_CHARGING_CURRENT_A is defined as a negative threshold (e.g. -0.10).
   * BATTERY_CHARGING_HYST_A is a positive margin to prevent toggling.
   */
  if (!wasCharging) {
    /* Start charging when current drops below the threshold. */
    if (i <= BATTERY_CHARGING_CURRENT_A) {
      s_status.charging = true;
    }
  } else {
    /* Stop charging only when current rises above (threshold + hysteresis). */
    if (i > (BATTERY_CHARGING_CURRENT_A + BATTERY_CHARGING_HYST_A)) {
      s_status.charging = false;
    }
  }

#if EVE_CHARGER_PRESENT_PIN >= 0
  /*
   * Hardware charger-present pin: override software detection when present.
   * LOW = charger detected (assumes active-low with pull-up).
   */
  if (digitalRead(EVE_CHARGER_PRESENT_PIN) == LOW) {
    s_status.charging = true;
  }
#endif

  /* ---- Determine named state ---- */
  BatteryState newState;

  if (s_status.charging && pct >= BATTERY_PERCENT_FULL_THRESH) {
    newState = BatteryState::BATTERY_FULL;
  } else if (s_status.charging) {
    newState = BatteryState::BATTERY_CHARGING;
  } else if (pct < BATTERY_PERCENT_CRITICAL_THRESH) {
    newState = BatteryState::BATTERY_CRITICAL;
  } else if (pct < BATTERY_PERCENT_LOW_THRESH) {
    newState = BatteryState::BATTERY_LOW;
  } else {
    newState = BatteryState::BATTERY_OK;
  }

  s_status.state = newState;

  /* ---- Debug serial output ---- */
  Serial.printf("[EVE][PowerMon] V=%.2fV  I=%.3fA  pct=%u%%  chg=%d  state=%s\n",
                s_status.voltage, s_status.current, pct,
                (int)s_status.charging, stateLabel(newState));

  /* ---- Fire reaction hooks only when state changes ---- */
  if (newState != s_prevState) {
    Serial.printf("[EVE][PowerMon] State change: %s -> %s\n",
                  stateLabel(s_prevState), stateLabel(newState));
    s_prevState = newState;

    switch (newState) {
      case BatteryState::BATTERY_OK:       onBatteryOk();       break;
      case BatteryState::BATTERY_LOW:      onBatteryLow();      break;
      case BatteryState::BATTERY_CRITICAL: onBatteryCritical(); break;
      case BatteryState::BATTERY_CHARGING: onBatteryCharging(); break;
      case BatteryState::BATTERY_FULL:     onBatteryFull();     break;
      default: break;
    }
  }
}

const PowerStatus& getPowerStatus(void) {
  return s_status;
}

// ---------------------------------------------------------------------------
// Local reaction hooks
// Implement your NeoPixel / servo / audio effects here.
// neopixelSetPattern() is safe to call even when EVE_ENABLE_NEOPIXEL == 0
// (it compiles to a no-op in that case).
// ---------------------------------------------------------------------------

void onBatteryOk(void) {
  Serial.println(F("[EVE][PowerMon] Battery OK — normal operation"));
  neopixelSetPattern(1);  /* pattern 1 = normal / idle glow */

  /* Future: resume normal animations, normal servo motion, etc. */
}

void onBatteryLow(void) {
  Serial.println(F("[EVE][PowerMon] Battery LOW — warning mode"));
  neopixelSetPattern(2);  /* pattern 2 = slow amber pulse */

  /* Future: play warning sound, reduce servo motion speed, dim eyes. */
}

void onBatteryCritical(void) {
  Serial.println(F("[EVE][PowerMon] Battery CRITICAL — protective mode"));
  neopixelSetPattern(3);  /* pattern 3 = fast red blink */

  /* Future: stop non-essential actuators, play urgent sound, request dock. */
}

void onBatteryCharging(void) {
  Serial.println(F("[EVE][PowerMon] Battery CHARGING — aura active"));
  neopixelSetPattern(4);  /* pattern 4 = slow green/blue breathing */

  /* Future: play charging sound, show charging animation on eyes/TFT. */
}

void onBatteryFull(void) {
  Serial.println(F("[EVE][PowerMon] Battery FULL — ready"));
  neopixelSetPattern(5);  /* pattern 5 = bright white flash then steady */

  /* Future: play ready sound, wake-up animation, resume full motion profile. */
}

// ---------------------------------------------------------------------------
// Stubs compiled when power monitor is disabled
// ---------------------------------------------------------------------------
#else /* EVE_ENABLE_POWER_MONITOR == 0 */

/* Provide zero-cost stubs so callers compile without #ifdef guards. */
static PowerStatus s_statusStub = {};

void    powerMonitorInit(void) {}
void    powerMonitorTick(void) {}
float   readVoltage(void)      { return 0.0f; }
float   readCurrent(void)      { return 0.0f; }
uint8_t getBatteryPercent(void){ return 0; }
void    updateBatteryState(void) {}
const PowerStatus& getPowerStatus(void) { return s_statusStub; }

void onBatteryOk(void)       {}
void onBatteryLow(void)      {}
void onBatteryCritical(void) {}
void onBatteryCharging(void) {}
void onBatteryFull(void)     {}

#endif /* EVE_ENABLE_POWER_MONITOR */
