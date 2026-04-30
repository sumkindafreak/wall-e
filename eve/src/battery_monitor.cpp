#include "battery_monitor.h"
#include "config.h"

#include <Arduino.h>
#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
#include <Wire.h>
#include <Adafruit_INA219.h>
#endif

static float s_v = 0.f;
static float s_a = 0.f;
static int s_pct = 0;
static EveBatStatus s_st = EVE_BAT_UNKNOWN;
static bool s_valid = false;
static bool s_hw = false;
static uint32_t s_lastMs = 0;
static bool s_verbose = true;

static const float BAT_HARD_INVALID_V = 0.20f;

#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
static Adafruit_INA219 s_ina219(EVE_INA219_I2C_ADDR);

static float readVoltageIna219(void) { return s_ina219.getBusVoltage_V(); }

static float readCurrentIna219(void) { return s_ina219.getCurrent_mA() / 1000.0f; }

#elif EVE_ENABLE_BATTERY_MONITOR

static const float ADC_REF_V = 3.3f;
static const int ADC_RES = 4095;
static const float DIV_RATIO = (float)EVE_BAT_R2 / (float)(EVE_BAT_R1 + EVE_BAT_R2);

static float readVoltageAdc(void) {
  uint32_t sum = 0;
  for (int i = 0; i < EVE_BAT_SAMPLES; i++) {
    sum += analogRead(EVE_BAT_ADC_PIN);
    delayMicroseconds(200);
  }
  float adcAvg = (float)sum / (float)EVE_BAT_SAMPLES;
  float adcV = (adcAvg / (float)ADC_RES) * ADC_REF_V;
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

#endif /* legacy ADC */

static void apply(float v, float a) {
  if (v <= BAT_HARD_INVALID_V) {
    s_v = v;
    s_a = a;
    s_pct = 0;
    s_valid = false;
    s_st = EVE_BAT_UNKNOWN;
    if (s_verbose) {
      Serial.printf("[EVE][BAT] %.2fV invalid reading — check sensor / wiring\n", (double)v);
    }
    return;
  }

  if (v < EVE_BAT_MIN_V) {
    s_v = v;
    s_a = a;
    s_pct = 0;
    s_valid = true;
    s_st = EVE_BAT_CRITICAL;
    if (s_verbose) {
      Serial.printf("[EVE][BAT] %.2fV below %.2fV min — CRITICAL\n", (double)v, (double)EVE_BAT_MIN_V);
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

#if EVE_BATTERY_INA219
#if EVE_INA219_SDA >= 0 && EVE_INA219_SCL >= 0
  Wire.begin((int)EVE_INA219_SDA, (int)EVE_INA219_SCL);
#else
  Wire.begin();
#endif
  if (!s_ina219.begin(&Wire)) {
    Serial.println(F("[EVE][BAT] INA219 not found on I2C — check SDA/SCL, addr 0x40, power"));
    return;
  }
  s_ina219.setCalibration_32V_2A();
  s_hw = true;
  s_verbose = true;
  float v = readVoltageIna219();
  float a = readCurrentIna219();
  apply(v, a);
  s_verbose = false;
  s_lastMs = millis();
  Serial.printf("[EVE][BAT] INA219 OK addr=0x%02x SDA=%d SCL=%d\n",
                (unsigned)EVE_INA219_I2C_ADDR, (int)EVE_INA219_SDA, (int)EVE_INA219_SCL);
  return;
#elif EVE_BAT_ADC_PIN < 0
  Serial.println(F("[EVE][BAT] ADC disabled (set EVE_BAT_ADC_PIN or enable INA219)"));
  return;
#else
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
  Serial.printf("[EVE][BAT] init ADC V on GPIO%d", EVE_BAT_ADC_PIN);
#if EVE_CUR_ADC_PIN >= 0
  Serial.printf(" | I on GPIO%d", EVE_CUR_ADC_PIN);
#endif
  Serial.println();
#endif
}

void eveBatteryTick(void) {
#if !EVE_ENABLE_BATTERY_MONITOR || !s_hw
  return;
#endif
#if EVE_BATTERY_INA219
#elif EVE_BAT_ADC_PIN < 0
  return;
#endif
  uint32_t now = millis();
  if (now - s_lastMs < EVE_BAT_POLL_MS) return;
  s_lastMs = now;
#if EVE_BATTERY_INA219
  float v = readVoltageIna219();
  float a = readCurrentIna219();
#else
  float v = readVoltageAdc();
  float a = readCurrentAdc();
#endif
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
