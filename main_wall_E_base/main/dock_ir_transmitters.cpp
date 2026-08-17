/*******************************************************************************
 * dock_ir_transmitters.cpp — ~38 kHz carrier on both IR LEDs for dock TSOPs.
 ******************************************************************************/

#include "dock_ir_transmitters.h"
#include "ledc_compat.h"
#include <Arduino.h>

static constexpr uint32_t kCarrierHz = 38000;
static constexpr uint8_t kPwmBits = 8;
static constexpr uint32_t kDutyOn = (1u << (kPwmBits - 1));

void dockIrTransmittersInit(void) {
  const bool leftOk = walleLedcAttach(DOCK_IR_TX_LEFT_PIN,
                                      WALLE_LEDC_CH_DOCK_IR_L,
                                      kCarrierHz,
                                      kPwmBits);
  const bool rightOk = walleLedcAttach(DOCK_IR_TX_RIGHT_PIN,
                                       WALLE_LEDC_CH_DOCK_IR_R,
                                       kCarrierHz,
                                       kPwmBits);
  (void)walleLedcWrite(DOCK_IR_TX_LEFT_PIN, WALLE_LEDC_CH_DOCK_IR_L, 0);
  (void)walleLedcWrite(DOCK_IR_TX_RIGHT_PIN, WALLE_LEDC_CH_DOCK_IR_R, 0);

  if (!leftOk || !rightOk) {
    Serial.println(F("[DockIR] ERROR: LEDC carrier attach failed"));
  } else {
    Serial.printf("[DockIR] TX ready L=%d R=%d at %lu Hz\n",
                  DOCK_IR_TX_LEFT_PIN,
                  DOCK_IR_TX_RIGHT_PIN,
                  (unsigned long)kCarrierHz);
  }
}

void dockIrTransmittersSetEnabled(bool on) {
  const uint32_t duty = on ? kDutyOn : 0u;
  (void)walleLedcWrite(DOCK_IR_TX_LEFT_PIN,
                       WALLE_LEDC_CH_DOCK_IR_L,
                       duty);
  (void)walleLedcWrite(DOCK_IR_TX_RIGHT_PIN,
                       WALLE_LEDC_CH_DOCK_IR_R,
                       duty);
}

void dockIrTransmittersUpdate(uint32_t now) {
  (void)now;
}
