/*******************************************************************************
 * ir_beacon_receivers.cpp
 * WALL-E front IR transmitters (940 nm) for dock TSOP receivers.
 * Pins unchanged: IR_BEACON_LEFT_PIN / IR_BEACON_RIGHT_PIN (see .h).
 *
 * TSOP/VS1838B need ~38 kHz carrier — we drive LEDs with LEDC PWM, not DC.
 ******************************************************************************/

#include "ir_beacon_receivers.h"
#include <Arduino.h>

#define IR_PWM_FREQ_HZ   38000u
#define IR_PWM_RES_BITS  8
#define IR_PWM_DUTY_ON   128u

#if defined(ARDUINO_ARCH_ESP32)
static const uint8_t kChL = 8;
static const uint8_t kChR = 9;
#endif

static bool s_txEnabled = false;

#if defined(ARDUINO_ARCH_ESP32)
static void ledcApplyOutputs(void) {
  uint32_t duty = s_txEnabled ? IR_PWM_DUTY_ON : 0u;
  ledcWrite(kChL, duty);
  ledcWrite(kChR, duty);
}
#endif

void irBeaconInit(void) {
#if defined(ARDUINO_ARCH_ESP32)
  ledcSetup((uint8_t)kChL, IR_PWM_FREQ_HZ, IR_PWM_RES_BITS);
  ledcSetup((uint8_t)kChR, IR_PWM_FREQ_HZ, IR_PWM_RES_BITS);
  ledcAttachPin(IR_BEACON_LEFT_PIN, (uint8_t)kChL);
  ledcAttachPin(IR_BEACON_RIGHT_PIN, (uint8_t)kChR);
  s_txEnabled = false;
  ledcApplyOutputs();
#else
  pinMode(IR_BEACON_LEFT_PIN, OUTPUT);
  pinMode(IR_BEACON_RIGHT_PIN, OUTPUT);
  digitalWrite(IR_BEACON_LEFT_PIN, LOW);
  digitalWrite(IR_BEACON_RIGHT_PIN, LOW);
#endif
}

void irBeaconSetTransmitEnabled(bool on) {
  s_txEnabled = on;
#if defined(ARDUINO_ARCH_ESP32)
  ledcApplyOutputs();
#else
  digitalWrite(IR_BEACON_LEFT_PIN, on ? HIGH : LOW);
  digitalWrite(IR_BEACON_RIGHT_PIN, on ? HIGH : LOW);
#endif
}

void irBeaconUpdate(uint32_t now) {
  (void)now;
}

uint16_t irBeaconGetLeft(void) { return s_txEnabled ? 1u : 0u; }
uint16_t irBeaconGetRight(void) { return s_txEnabled ? 1u : 0u; }

int16_t irBeaconGetBalance(void) {
  /* Legacy API — alignment uses dock beacon ir_align_hint on base. */
  return 0;
}

bool irBeaconAnyDetected(void) {
  /* Legacy: was “either local receiver saw dock”. TX side: on when actively transmitting. */
  return s_txEnabled;
}
