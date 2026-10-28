/**
 * EVE battery monitor (Arduino side).
 *
 * INA219 path (EVE_BATTERY_INA219 = 1, the configured build):
 *   - The INA219 measures "bus voltage" on its V- / IN- pin, i.e. after the shunt. The pack
 *     voltage is bus + shunt drop, so both are read and added.
 *   - Calibration 32 V / 2 A (Adafruit default for the 0.1 ohm shunt): shunt range +/-320 mV,
 *     i.e. about +/-3.2 A. A shunt reading at the limit means the current overflowed -> current
 *     marked invalid (voltage still usable).
 *   - Every I2C read is checked with success(); a failed read is an invalid sample.
 * Legacy ADC path (EVE_BATTERY_INA219 = 0): divider on EVE_BAT_ADC_PIN, read with the factory-
 *   calibrated analogReadMilliVolts(). A pin voltage near the ADC ceiling is treated as clipped
 *   (invalid) rather than as a full battery.
 */
#include "battery_monitor.h"

#include <math.h>

#include "config.h"
#include "eve_battery_logic.h"
#include "eve_safety.h"

#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
#include <Adafruit_INA219.h>
#include <Wire.h>
#endif

// -----------------------------------------------------------------------------
// Configuration taken from config.h
// -----------------------------------------------------------------------------
static const EveBatteryConfig kCfg = {
    EVE_BAT_MIN_V,
    EVE_BAT_MAX_V,
    EVE_BAT_PLAUSIBLE_MIN_V,
    EVE_BAT_PLAUSIBLE_MAX_V,
    EVE_BAT_WARN_PCT,
    EVE_BAT_CRIT_PCT,
    EVE_BAT_HYST_PCT,
    (int8_t)EVE_INA219_CHARGE_SIGN,
    EVE_BAT_CHARGE_DETECT_A,
    EVE_BAT_FULL_TAPER_A,
    0.35f, /* EMA weight per 2 s sample: ~5 s settling, rides through motor-current sags */
    3,     /* three invalid samples in a row (~6 s) -> level UNKNOWN */
};

static EveBatteryState s_st;
static bool s_hw = false;
static float s_rawV = 0.f;
static uint32_t s_lastPollMs = 0;
static uint32_t s_lastLogMs = 0;
#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
static uint32_t s_lastInitTryMs = 0; /* INA219 retry timer */
#endif
static bool s_logged = false;

static const char* levelName(EveBatLevel l) {
  switch (l) {
    case EVE_BATLVL_OK: return "ok";
    case EVE_BATLVL_WARN: return "warn";
    case EVE_BATLVL_CRITICAL: return "critical";
    default: return "unknown";
  }
}

static const char* chargeName(int8_t c) { return c < 0 ? "unknown" : (c ? "yes" : "no"); }

// -----------------------------------------------------------------------------
// Sensor reads
// -----------------------------------------------------------------------------
#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
static Adafruit_INA219 s_ina219(EVE_INA219_I2C_ADDR);

static bool tryBeginIna219(void) {
  if (!s_ina219.begin(&Wire)) return false;
  s_ina219.setCalibration_32V_2A();
  return true;
}

static void readSample(float* v, bool* vOk, float* a, bool* aOk) {
  const float shuntMv = s_ina219.getShuntVoltage_mV();
  const bool okShunt = s_ina219.success();
  const float busV = s_ina219.getBusVoltage_V();
  const bool okBus = s_ina219.success();
  const float curMa = s_ina219.getCurrent_mA();
  const bool okCur = s_ina219.success();
  *vOk = okShunt && okBus;
  *v = busV + shuntMv / 1000.0f;
  *aOk = okCur && okShunt && fabsf(shuntMv) < 319.0f;
  *a = curMa / 1000.0f;
}

#elif EVE_ENABLE_BATTERY_MONITOR

static const float kDivRatio = (float)EVE_BAT_R2 / (float)(EVE_BAT_R1 + EVE_BAT_R2);
/* ESP32-S3 ADC with 11 dB attenuation reads reliably up to roughly 3.1 V. */
static const uint32_t kAdcClipMv = 3050;

static uint32_t readPinMv(int pin, int samples) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) sum += analogReadMilliVolts(pin);
  return sum / (uint32_t)samples;
}

static void readSample(float* v, bool* vOk, float* a, bool* aOk) {
  const uint32_t pinMv = readPinMv(EVE_BAT_ADC_PIN, EVE_BAT_SAMPLES);
  *v = ((float)pinMv / 1000.0f / kDivRatio) * EVE_BAT_CALIB;
  *vOk = pinMv < kAdcClipMv; /* clipped = we cannot know the real voltage */
#if EVE_CUR_ADC_PIN >= 0
  const uint32_t curMv = readPinMv(EVE_CUR_ADC_PIN, EVE_CUR_SAMPLES);
  *a = ((float)curMv / 1000.0f - EVE_CUR_ZERO_V) / EVE_CUR_SENSITIVITY_V_PER_A;
  *aOk = curMv > 50 && curMv < kAdcClipMv;
#else
  *a = 0.f;
  *aOk = false;
#endif
}
#endif

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void eveBatteryInit(void) {
  eveBatteryLogicInit(&s_st);
  s_hw = false;
  s_lastPollMs = 0;

#if !EVE_ENABLE_BATTERY_MONITOR
  Serial.println(F("[EVE][BAT] monitor disabled (config) - battery state reported as unknown"));
  return;
#elif EVE_BATTERY_INA219
#if EVE_INA219_SDA >= 0 && EVE_INA219_SCL >= 0
  Wire.begin((int)EVE_INA219_SDA, (int)EVE_INA219_SCL);
#else
  Wire.begin();
#endif
  s_lastInitTryMs = millis();
  s_hw = tryBeginIna219();
  if (!s_hw) {
    Serial.printf("[EVE][BAT] INA219 not found at 0x%02x (SDA=%d SCL=%d) - battery unknown, will retry\n",
                  (unsigned)EVE_INA219_I2C_ADDR, (int)EVE_INA219_SDA, (int)EVE_INA219_SCL);
    eveSafetySetFault(EVE_FAULT_BATTERY_SENSOR, true);
    return;
  }
  Serial.printf("[EVE][BAT] INA219 OK addr=0x%02x SDA=%d SCL=%d pack %.1f-%.1f V, charge sign %s\n",
                (unsigned)EVE_INA219_I2C_ADDR, (int)EVE_INA219_SDA, (int)EVE_INA219_SCL, (double)EVE_BAT_MIN_V,
                (double)EVE_BAT_MAX_V,
                EVE_INA219_CHARGE_SIGN == 0 ? "not set (charging reported as unknown)"
                                            : (EVE_INA219_CHARGE_SIGN > 0 ? "+" : "-"));
#else
#if EVE_BAT_ADC_PIN < 0
  Serial.println(F("[EVE][BAT] ADC pin not set - battery unknown"));
  return;
#else
  analogReadResolution(12);
  analogSetPinAttenuation(EVE_BAT_ADC_PIN, ADC_11db);
#if EVE_CUR_ADC_PIN >= 0
  pinMode(EVE_CUR_ADC_PIN, INPUT);
  analogSetPinAttenuation(EVE_CUR_ADC_PIN, ADC_11db);
#endif
  s_hw = true;
  const float fullPinV = EVE_BAT_MAX_V * kDivRatio;
  Serial.printf("[EVE][BAT] ADC divider on GPIO%d: full pack = %.2f V at the pin\n", EVE_BAT_ADC_PIN, (double)fullPinV);
  if (fullPinV > 3.1f) {
    Serial.println(F("[EVE][BAT] WARNING: divider puts more than ~3.1 V on the ADC pin at full charge."));
    Serial.println(F("[EVE][BAT]          Readings above the ADC range are marked INVALID (and >3.3 V can harm the pin)."));
  }
#endif
#endif
  eveBatteryTick();
}

bool eveBatteryTick(void) {
#if !EVE_ENABLE_BATTERY_MONITOR
  return false;
#else
  const uint32_t now = millis();
  if (s_lastPollMs != 0 && (uint32_t)(now - s_lastPollMs) < EVE_BAT_POLL_MS) return false;
  s_lastPollMs = now == 0 ? 1 : now;

#if EVE_BATTERY_INA219
  if (!s_hw) {
    /* Retry a missing INA219 every 10 s (loose connector, late power-up). */
    if ((uint32_t)(now - s_lastInitTryMs) < 10000u) return false;
    s_lastInitTryMs = now;
    s_hw = tryBeginIna219();
    if (!s_hw) return false;
    Serial.println(F("[EVE][BAT] INA219 found on retry"));
  }
#elif EVE_BAT_ADC_PIN < 0
  return false;
#endif

  float v = 0.f, a = 0.f;
  bool vOk = false, aOk = false;
  readSample(&v, &vOk, &a, &aOk);
  s_rawV = v;
  const bool changed = eveBatteryLogicUpdate(&s_st, &kCfg, v, vOk, a, aOk);

  /* A sensor that keeps failing is a (non-blocking) fault: EVE runs, battery stays "unknown". */
  const bool sensorFault = !s_st.valid && s_st.invalidRun >= kCfg.invalidToUnknown;
  eveSafetySetFault(EVE_FAULT_BATTERY_SENSOR, sensorFault);

  const bool periodic = !s_logged || (uint32_t)(now - s_lastLogMs) >= EVE_BAT_LOG_MS;
  if (changed || periodic) {
    s_logged = true;
    s_lastLogMs = now;
    if (s_st.valid) {
      Serial.printf("[EVE][BAT] %.2fV %+.2fA%s %d%% level=%s charging=%s%s\n", (double)s_st.vFilt, (double)s_st.aFilt,
                    s_st.currentValid ? "" : "(invalid)", s_st.pct, levelName(s_st.level), chargeName(s_st.charging),
                    s_st.full ? " FULL" : "");
    } else {
      Serial.printf("[EVE][BAT] invalid sample (%.2fV raw): %s [%u in a row] level=%s\n", (double)v, s_st.why,
                    (unsigned)s_st.invalidRun, levelName(s_st.level));
    }
  }
  return true;
#endif
}

bool eveBatteryHardwareEnabled(void) { return s_hw; }
bool eveBatteryDataValid(void) { return s_hw && s_st.valid; }
float eveBatteryVoltage(void) { return (s_st.valid && s_st.everValid) ? s_st.vFilt : s_rawV; }
float eveBatteryCurrentA(void) { return s_st.currentValid ? s_st.aFilt : 0.f; }
int eveBatteryPercent(void) { return s_st.valid ? s_st.pct : 0; }

EveBatStatus eveBatteryStatus(void) {
  switch (s_st.level) {
    case EVE_BATLVL_OK: return EVE_BAT_OK;
    case EVE_BATLVL_WARN: return EVE_BAT_WARN;
    case EVE_BATLVL_CRITICAL: return EVE_BAT_CRITICAL;
    default: return EVE_BAT_UNKNOWN;
  }
}

bool eveBatteryIsCritical(void) { return s_hw && s_st.level == EVE_BATLVL_CRITICAL; }
EveBatLevel eveBatteryLevel(void) { return s_hw ? s_st.level : EVE_BATLVL_UNKNOWN; }
int8_t eveBatteryCharging(void) { return s_hw ? s_st.charging : (int8_t)-1; }
bool eveBatteryFull(void) { return s_hw && s_st.full; }
const char* eveBatteryInvalidReason(void) { return s_hw ? s_st.why : "no sensor"; }

void eveBatteryFillReport(EveBatteryReport* out) {
  if (!out) return;
  out->hw = s_hw;
  out->valid = eveBatteryDataValid();
  out->v = eveBatteryVoltage();
  out->a = eveBatteryCurrentA();
  out->pct = eveBatteryPercent();
  out->st = (uint8_t)eveBatteryStatus();
  out->charging = eveBatteryCharging();
  out->full = eveBatteryFull();
}
