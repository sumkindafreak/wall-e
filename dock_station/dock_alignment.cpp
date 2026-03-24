/*******************************************************************************
 * dock_alignment.cpp
 * Dock alignment sensors + arrow MOSFET outputs
 *
 * WALL-E carries IR transmitters (~38 kHz); dock uses TSOP-style receivers on
 * PIN_ALIGN_LEFT / PIN_ALIGN_RIGHT (LOW = IR seen). Classification and Serial
 * guidance live in dock_ir_guidance.cpp.
 *
 * Arrow mapping (guiding stages 1m / 20cm):
 *   Left RX only  -> flash left arrow  (robot should steer left)
 *   Right RX only -> flash right arrow
 *   Both          -> both solid (move forward)
 *   Neither       -> alternating arrows (search); LOST timeout -> arrows off
 ******************************************************************************/

#include "dock_alignment.h"
#include "dock_config.h"
#include "dock_hw.h"
#include "dock_ir_guidance.h"
#include "dock_protocol.h"
#include <Arduino.h>

static unsigned long s_lastBlink = 0;
static bool s_blinkState = false;
static uint8_t s_stageFromWallE = APPROACH_FAR;
static unsigned long s_lastStageMs = 0;
static bool s_lastDocked = false;

#define BLINK_MS_NORMAL    300
#define BLINK_MS_PRECISION 120
#define BLINK_MS_SEARCH    500
#define ARR_ON   (MOSFET_ARROW_ACTIVE_LOW ? LOW : HIGH)
#define ARR_OFF  (MOSFET_ARROW_ACTIVE_LOW ? HIGH : LOW)

void dockAlignmentBegin(void) {
  dockConfigureInputPin(PIN_ALIGN_LEFT, INPUT, "alignment left");
  dockConfigureInputPin(PIN_ALIGN_RIGHT, INPUT, "alignment right");

  dockConfigureOutputPin(PIN_ARROW_LEFT, ARR_OFF, "left arrow");
  dockConfigureOutputPin(PIN_ARROW_RIGHT, ARR_OFF, "right arrow");

  dockIrGuidanceBegin();
}

static void setArrowState(uint8_t leftLevel, uint8_t rightLevel) {
  dockWriteOutputPin(PIN_ARROW_LEFT, leftLevel, "left arrow");
  dockWriteOutputPin(PIN_ARROW_RIGHT, rightLevel, "right arrow");
}

void dockAlignmentSetStage(uint8_t stage) {
  if (stage <= APPROACH_DOCKED) {
    s_stageFromWallE = stage;
    s_lastStageMs = millis();
  }
}

void dockAlignmentUpdate(bool docked) {
  unsigned long now = millis();

  if (s_lastDocked != docked) {
    s_lastDocked = docked;
  }

  if (docked) {
    setArrowState(ARR_OFF, ARR_OFF);
    return;
  }

  uint8_t effStage = s_stageFromWallE;
  if (now - s_lastStageMs > APPROACH_STAGE_TIMEOUT_MS) {
    effStage = dockIrGuidanceAnyReceiverActive() ? APPROACH_1M : APPROACH_FAR;
  }

  if (effStage == APPROACH_FAR) {
    setArrowState(ARR_ON, ARR_ON);
    return;
  }

  if (dockIrGuidanceGuidancePaused()) {
    setArrowState(ARR_OFF, ARR_OFF);
    return;
  }

  uint8_t sid = dockIrGuidanceStableId();
  uint32_t blinkMs = (effStage == APPROACH_20CM) ? BLINK_MS_PRECISION : BLINK_MS_NORMAL;
  if (sid == DOCK_IR_ALIGN_LOST) {
    blinkMs = BLINK_MS_SEARCH;
  }
  if (now - s_lastBlink > blinkMs) {
    s_blinkState = !s_blinkState;
    s_lastBlink = now;
  }

  if (sid == DOCK_IR_ALIGN_CENTER) {
    setArrowState(ARR_ON, ARR_ON);
    return;
  }

  if (sid == DOCK_IR_ALIGN_LEFT) {
    setArrowState(s_blinkState ? ARR_ON : ARR_OFF, ARR_OFF);
    return;
  }

  if (sid == DOCK_IR_ALIGN_RIGHT) {
    setArrowState(ARR_OFF, s_blinkState ? ARR_ON : ARR_OFF);
    return;
  }

  /* LOST: alternate arrows (search) */
  setArrowState(s_blinkState ? ARR_ON : ARR_OFF, s_blinkState ? ARR_OFF : ARR_ON);
}
