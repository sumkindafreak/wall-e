// ============================================================
// WALL-E Flashlight Control — LDR + MOSFET
// ============================================================

#include "flashlight_control.h"
#include "servo_manager.h"
#include <Arduino.h>

static bool s_flashlightOn = false;
static unsigned long s_lastRead = 0;
static unsigned long s_lastChange = 0;

static void setFlashlightOutput(bool on) {
#if WALLE_BASE_FLASHLIGHT_ON_PCA9685
  if (!servoAuxSetDigital(BASE_FLASHLIGHT_PCA_CHANNEL, on)) {
    static uint32_t lastWarn = 0;
    if (millis() - lastWarn > 2000u) {
      lastWarn = millis();
      Serial.println(F("[Flashlight] WARN: PCA9685 aux output unavailable"));
    }
  }
#else
  digitalWrite(FLASHLIGHT_PIN, on ? HIGH : LOW);
#endif
}

void flashlightInit() {
#if WALLE_BASE_FLASHLIGHT_ON_PCA9685
  setFlashlightOutput(false);
  Serial.printf("[Flashlight] LDR GPIO %d, MOSFET via PCA9685 channel %d | dark above %d\n",
                LDR_ADC_PIN,
                BASE_FLASHLIGHT_PCA_CHANNEL,
                LDR_DARK_RAW);
#else
  pinMode(FLASHLIGHT_PIN, OUTPUT);
  digitalWrite(FLASHLIGHT_PIN, LOW);
  Serial.printf("[Flashlight] LDR GPIO %d, MOSFET GPIO %d | dark above %d\n",
                LDR_ADC_PIN,
                FLASHLIGHT_PIN,
                LDR_DARK_RAW);
#endif

  pinMode(LDR_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LDR_ADC_PIN, ADC_11db);
  s_flashlightOn = false;
  s_lastRead = 0;
  s_lastChange = millis();
}

void flashlightHandle() {
  if ((millis() - s_lastRead) < LDR_POLL_MS) return;
  s_lastRead = millis();

  uint32_t sum = 0;
  for (int i = 0; i < LDR_SAMPLES; i++) {
    sum += analogRead(LDR_ADC_PIN);
    delay(1);
  }
  const int raw = (int)(sum / LDR_SAMPLES);

  bool wantOn;
#if LDR_DARK_WHEN_LOW
  if (s_flashlightOn) wantOn = (raw <= LDR_DARK_RAW + LDR_HYST);
  else wantOn = (raw <= LDR_DARK_RAW);
#else
  if (s_flashlightOn) wantOn = (raw >= LDR_DARK_RAW - LDR_HYST);
  else wantOn = (raw >= LDR_DARK_RAW);
#endif

  const unsigned long now = millis();
  const unsigned long elapsed = now - s_lastChange;
  if (wantOn != s_flashlightOn) {
    if (s_flashlightOn && elapsed < LDR_MIN_ON_MS) wantOn = true;
    if (!s_flashlightOn && elapsed < LDR_MIN_OFF_MS) wantOn = false;
  }

  if (wantOn != s_flashlightOn) {
    s_flashlightOn = wantOn;
    s_lastChange = now;
    setFlashlightOutput(s_flashlightOn);
    Serial.printf("[Flashlight] %s (LDR raw=%d)\n",
                  s_flashlightOn ? "ON" : "OFF",
                  raw);
  }
}
