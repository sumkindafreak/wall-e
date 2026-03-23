/*******************************************************************************
 * ir_beacon_receivers.cpp
 * Left/right IR beacon receiver readings for dock alignment
 ******************************************************************************/

#include "ir_beacon_receivers.h"
#include <Arduino.h>

#define IR_SMOOTH        0.3f   /* Low-pass: new = alpha*raw + (1-alpha)*prev */
#define IR_DETECT_THRESH 100    /* ADC above this = beacon detected */

static uint16_t s_left  = 0;
static uint16_t s_right = 0;
static uint32_t s_lastUpdateMs = 0;
#define IR_POLL_MS 20

void irBeaconInit(void) {
  pinMode(IR_BEACON_LEFT_PIN,  INPUT);
  pinMode(IR_BEACON_RIGHT_PIN, INPUT);
  s_left  = 0;
  s_right = 0;
}

void irBeaconUpdate(uint32_t now) {
  if (now - s_lastUpdateMs < IR_POLL_MS) return;
  s_lastUpdateMs = now;

  uint16_t l = analogRead(IR_BEACON_LEFT_PIN);
  uint16_t r = analogRead(IR_BEACON_RIGHT_PIN);

  /* Simple low-pass to reduce noise */
  s_left  = (uint16_t)(IR_SMOOTH * l + (1.0f - IR_SMOOTH) * s_left);
  s_right = (uint16_t)(IR_SMOOTH * r + (1.0f - IR_SMOOTH) * s_right);
}

uint16_t irBeaconGetLeft(void)  { return s_left;  }
uint16_t irBeaconGetRight(void) { return s_right; }

int16_t irBeaconGetBalance(void) {
  return (int16_t)s_right - (int16_t)s_left;
}

bool irBeaconAnyDetected(void) {
  return (s_left > IR_DETECT_THRESH || s_right > IR_DETECT_THRESH);
}
