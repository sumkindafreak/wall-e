// ============================================================
//  WALL-E Battery Monitor Implementation
// ============================================================

#include "battery_monitor.h"
#include <Arduino.h>

static BatteryData _bat = {};
static unsigned long _lastRead = 0;
static bool _verboseSampleLog = true;

// Voltage: ADC sees divider of 5V sensor output. Vadc = sensor_V * (R2/(R1+R2)), so sensor_V = Vadc / DIVIDER_RATIO.
static const float DIVIDER_RATIO = (float)BAT_R2 / (float)(BAT_R1 + BAT_R2);

// ============================================================
//  Init
// ============================================================
void batteryInit() {
  pinMode(BAT_ADC_PIN, INPUT);
  pinMode(CUR_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(CUR_ADC_PIN, ADC_11db);

  _bat.valid  = true;
  _bat.status = BAT_UNKNOWN;
  _bat.currentA = 0.0f;

  batteryHandle();
  _lastRead = 0;

  Serial.printf("[Battery] V on GPIO %d | I on GPIO %d | R1=%d R2=%d | %.1fV-%.1fV\n",
    BAT_ADC_PIN, CUR_ADC_PIN, BAT_R1, BAT_R2, BATTERY_MIN_V, BATTERY_MAX_V);
}

// ============================================================
//  Read with oversampling
// ============================================================
static float readVoltage() {
  uint32_t sum = 0;
  for (int i = 0; i < BAT_SAMPLES; i++) {
    sum += analogRead(BAT_ADC_PIN);
    delay(2);
  }
  float adcAvg   = (float)sum / (float)BAT_SAMPLES;
  float adcVolt  = (adcAvg / ADC_RESOLUTION) * ADC_REF_V;
  float sensorV  = adcVolt / DIVIDER_RATIO;  // 0–5V at sensor
  float v        = sensorV * (BATTERY_MAX_V / VOLTAGE_SENSOR_OUTPUT_MAX_V) * BAT_V_CALIB;
  if (_verboseSampleLog) {
    Serial.printf("[Battery] raw ADC=%d adcV=%.3f sensorV=%.2f calcV=%.2f\n",
                  (int)(sum / BAT_SAMPLES), adcVolt, sensorV, v);
  }
  return v;
}

// Current: ADC voltage → (V - zero) / sensitivity = amps
static float readCurrent() {
  uint32_t sum = 0;
  for (int i = 0; i < CUR_SAMPLES; i++) {
    sum += analogRead(CUR_ADC_PIN);
    delay(1);
  }
  float adcVolt = ((float)sum / (float)CUR_SAMPLES / ADC_RESOLUTION) * ADC_REF_V;
  float amps = (adcVolt - CUR_ZERO_V) / CUR_SENSITIVITY_V_PER_A;
  return amps;
}

// ============================================================
//  Apply one V/A pair to global BatteryData
// ============================================================
static void applyBatteryReadings(float v, float a) {
  if (v < BATTERY_MIN_V) {
    _bat.voltage = v;
    _bat.currentA = a;
    _bat.percent = 0;
    _bat.valid = false;
    _bat.status = BAT_UNKNOWN;
    if (_verboseSampleLog) {
      Serial.printf("[Battery] %.2fV < %.1fV (min rail) — invalid sensor\n", v, BATTERY_MIN_V);
      Serial.printf("[Battery] %.2fV | %.2fA | --%% | status=invalid\n", v, a);
    }
    return;
  }

  if (v > BATTERY_MAX_V) {
    if (_verboseSampleLog)
      Serial.printf("[Battery] voltage clamped from %.2fV to max %.1fV\n", v, BATTERY_MAX_V);
    v = BATTERY_MAX_V;
  }

  int pct = (int)(((v - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V)) * 100.0f);
  pct = constrain(pct, 0, 100);

  _bat.voltage = v;
  _bat.currentA = a;
  _bat.percent = pct;
  _bat.valid = true;

  if (pct <= BAT_CRIT_PCT)
    _bat.status = BAT_CRITICAL;
  else if (pct <= BAT_WARN_PCT)
    _bat.status = BAT_WARNING;
  else
    _bat.status = BAT_OK;

  if (_verboseSampleLog)
    Serial.printf("[Battery] %.2fV | %.2fA | %d%% | status=%d\n", v, a, pct, (int)_bat.status);
}

// ============================================================
//  Handle
// ============================================================
bool batteryHandle() {
  if ((millis() - _lastRead) < BAT_POLL_MS) return false;
  _lastRead = millis();

  float v = readVoltage();
  float a = readCurrent();
  applyBatteryReadings(v, a);
  return true;
}

void batterySampleNow(void) {
  float v = readVoltage();
  float a = readCurrent();
  applyBatteryReadings(v, a);
  _lastRead = millis();
}

void batterySetVerboseSampleLog(bool on) {
  _verboseSampleLog = on;
}

// ============================================================
//  Public accessors
// ============================================================
const BatteryData& batteryGetData() {
  return _bat;
}

String batteryGetStatusJSON() {
  String s = "{";
  s += "\"voltage\":"   + String(_bat.voltage, 2) + ",";
  s += "\"current\":"   + String(_bat.currentA, 2) + ",";
  s += "\"percent\":"   + String(_bat.percent) + ",";
  s += "\"status\":"    + String((int)_bat.status) + ",";
  s += "\"valid\":"     + String(_bat.valid ? "true" : "false");
  s += "}";
  return s;
}
