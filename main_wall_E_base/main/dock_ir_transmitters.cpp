/*******************************************************************************
 * dock_ir_transmitters.cpp — ~38 kHz carrier on both IR LEDs for dock TSOPs.
 ******************************************************************************/

#include "dock_ir_transmitters.h"
#include <Arduino.h>

static constexpr uint32_t kCarrierHz = 38000;
static constexpr uint8_t  kPwmBits    = 8;
/* ~50% duty → strong TSOP response */
static constexpr uint32_t kDutyOn     = (1u << (kPwmBits - 1));

void dockIrTransmittersInit(void) {
  ledcAttach(DOCK_IR_TX_LEFT_PIN, kCarrierHz, kPwmBits);
  ledcAttach(DOCK_IR_TX_RIGHT_PIN, kCarrierHz, kPwmBits);
  ledcWrite(DOCK_IR_TX_LEFT_PIN, 0);
  ledcWrite(DOCK_IR_TX_RIGHT_PIN, 0);
}

void dockIrTransmittersSetEnabled(bool on) {
  const uint32_t d = on ? kDutyOn : 0;
  ledcWrite(DOCK_IR_TX_LEFT_PIN, d);
  ledcWrite(DOCK_IR_TX_RIGHT_PIN, d);
}

void dockIrTransmittersUpdate(uint32_t now) {
  (void)now;
}
