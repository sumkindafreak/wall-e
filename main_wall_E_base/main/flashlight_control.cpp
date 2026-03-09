// ============================================================
//  WALL-E Flashlight Control — LDR + MOSFET
// ============================================================

#include "flashlight_control.h"
#include <Arduino.h>

static bool _flashlightOn = false;
static unsigned long _lastRead = 0;
static unsigned long _lastChange = 0;

void flashlightInit() {
  pinMode(FLASHLIGHT_PIN, OUTPUT);
  digitalWrite(FLASHLIGHT_PIN, LOW);
  pinMode(LDR_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LDR_ADC_PIN, ADC_11db);
  _lastRead = 0;
  Serial.printf("[Flashlight] LDR on GPIO %d, MOSFET on GPIO %d | dark above %d\n",
    LDR_ADC_PIN, FLASHLIGHT_PIN, LDR_DARK_RAW);
}

void flashlightHandle() {
  if ((millis() - _lastRead) < LDR_POLL_MS) return;
  _lastRead = millis();

  uint32_t sum = 0;
  for (int i = 0; i < LDR_SAMPLES; i++) {
    sum += analogRead(LDR_ADC_PIN);
    delay(1);
  }
  int raw = (int)(sum / LDR_SAMPLES);

  // Light on when dark. Two wirings: dark = high raw (0) or dark = low raw (1)
  bool wantOn;
#if LDR_DARK_WHEN_LOW
  if (_flashlightOn) {
    wantOn = (raw <= LDR_DARK_RAW + LDR_HYST);
  } else {
    wantOn = (raw <= LDR_DARK_RAW);
  }
#else
  if (_flashlightOn) {
    wantOn = (raw >= LDR_DARK_RAW - LDR_HYST);
  } else {
    wantOn = (raw >= LDR_DARK_RAW);
  }
#endif

  // Enforce min on/off time so we don't oscillate if flashlight light hits the LDR
  unsigned long now = millis();
  unsigned long elapsed = now - _lastChange;
  if (wantOn != _flashlightOn) {
    if (_flashlightOn && elapsed < LDR_MIN_ON_MS)  wantOn = true;   // stay on a bit longer
    if (!_flashlightOn && elapsed < LDR_MIN_OFF_MS) wantOn = false;  // stay off a bit longer
  }

  if (wantOn != _flashlightOn) {
    _flashlightOn = wantOn;
    _lastChange = now;
    digitalWrite(FLASHLIGHT_PIN, _flashlightOn ? HIGH : LOW);
    Serial.printf("[Flashlight] %s (LDR raw=%d)\n", _flashlightOn ? "ON" : "OFF", raw);
  }
}
