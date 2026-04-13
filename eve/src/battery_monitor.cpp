#include "battery_monitor.h"
#include "config.h"

#include <Arduino.h>

static const float ADC_REF_V = 3.3f;
static const int ADC_RES = 4095;

static float s_v = 0.f;
static float s_a = 0.f;
static int s_pct = 0;
static EveBatStatus s_st = EVE_BAT_UNKNOWN;
static bool s_valid = false;
static bool s_hw = false;
static uint32_t s_lastMs = 0;
static bool s_verbose = true;

static const float DIV_RATIO = (float)EVE_BAT_R2 / (float)(EVE_BAT_R1 + EVE_BAT_R2);

static float readVoltageAdc(void) {
  uint32_t sum = 0;
  for (int i = 0; i < EVE_BAT_SAMPLES; i++) {
    sum += analogRead(EVE_BAT_ADC_PIN);
    delayMicroseconds(200);
  }
  float adcAvg = (float)sum / (float)EVE_BAT_SAMPLES;
  float adcV = (adcAvg / (float)ADC_RES) * ADC_REF_V;
  /* Divider node voltage → pack voltage (R1 upper, R2 lower to GND). */
  float vpack = (adcV / DIV_RATIO) * EVE_BAT_CALIB;
  return vpack;
}

static float readCurrentAdc(void) {
#if EVE_CUR_ADC_PIN < 0
  return 0.f;
#else
  uint32_t sum = 0;
  for (int i = 0; i < EVE_CUR_SAMPLES; i++) {
    sum += analogRead(EVE_CUR_ADC_PIN);
    delayMicroseconds(150);
  }
  float adcV = ((float)sum / (float)EVE_CUR_SAMPLES / (float)ADC_RES) * ADC_REF_V;
  return (adcV - EVE_CUR_ZERO_V) / EVE_CUR_SENSITIVITY_V_PER_A;
#endif
}

static void apply(float v, float a) {
  if (v < EVE_BAT_MIN_V) {
    s_v = v;
    s_a = a;
    s_pct = 0;
    s_valid = false;
    s_st = EVE_BAT_UNKNOWN;
    if (s_verbose) {
      Serial.printf("[EVE][BAT] %.2fV below %.2fV min — invalid / no divider\n", (double)v,
                    (double)EVE_BAT_MIN_V);
    }
    return;
  }

  if (v > EVE_BAT_MAX_V) v = EVE_BAT_MAX_V;

  int pct = (int)(((v - EVE_BAT_MIN_V) / (EVE_BAT_MAX_V - EVE_BAT_MIN_V)) * 100.0f);
  pct = constrain(pct, 0, 100);
  s_v = v;
  s_a = a;
  s_pct = pct;
  s_valid = true;
  if (pct <= EVE_BAT_CRIT_PCT)
    s_st = EVE_BAT_CRITICAL;
  else if (pct <= EVE_BAT_WARN_PCT)
    s_st = EVE_BAT_WARN;
  else
    s_st = EVE_BAT_OK;

  if (s_verbose)
    Serial.printf("[EVE][BAT] %.2fV %.2fA %d%% st=%d\n", (double)s_v, (double)s_a, s_pct, (int)s_st);
}

void eveBatteryInit(void) {
  s_hw = false;
  s_valid = false;
  s_st = EVE_BAT_UNKNOWN;
  s_lastMs = 0;

#if !EVE_ENABLE_BATTERY_MONITOR
  Serial.println(F("[EVE][BAT] monitor disabled (config)"));
  return;
#endif

#if EVE_BAT_ADC_PIN < 0
  Serial.println(F("[EVE][BAT] no voltage pin — monitor off"));
  return;
#endif

  analogReadResolution(12);
  analogSetPinAttenuation(EVE_BAT_ADC_PIN, ADC_11db);
#if EVE_CUR_ADC_PIN >= 0
  pinMode(EVE_CUR_ADC_PIN, INPUT);
  analogSetPinAttenuation(EVE_CUR_ADC_PIN, ADC_11db);
#endif

  s_hw = true;
  s_verbose = true;
  float v = readVoltageAdc();
  float a = readCurrentAdc();
  apply(v, a);
  s_verbose = false;
  s_lastMs = millis();
  Serial.printf("[EVE][BAT] init V on GPIO%d", EVE_BAT_ADC_PIN);
#if EVE_CUR_ADC_PIN >= 0
  Serial.printf(" | I on GPIO%d", EVE_CUR_ADC_PIN);
#endif
  Serial.println();
}

void eveBatteryTick(void) {
#if !EVE_ENABLE_BATTERY_MONITOR || !s_hw || EVE_BAT_ADC_PIN < 0
  return;
#endif
  uint32_t now = millis();
  if (now - s_lastMs < EVE_BAT_POLL_MS) return;
  s_lastMs = now;
  float v = readVoltageAdc();
  float a = readCurrentAdc();
  apply(v, a);
}

bool eveBatteryHardwareEnabled(void) { return s_hw; }
bool eveBatteryDataValid(void) { return s_valid; }
float eveBatteryVoltage(void) { return s_v; }
float eveBatteryCurrentA(void) { return s_a; }
int eveBatteryPercent(void) { return s_pct; }
EveBatStatus eveBatteryStatus(void) { return s_st; }

bool eveBatteryIsCritical(void) {
  return s_hw && s_valid && s_pct <= EVE_BAT_CRIT_PCT;
}
