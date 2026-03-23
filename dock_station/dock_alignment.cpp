/*******************************************************************************
 * dock_alignment.cpp
 * Dock alignment sensors + arrow MOSFET outputs
 *
 * Approach Mode stages (WALL-E reports via ESP-NOW, or fallback to sensors):
 *   FAR (>1m):  Arrows off or slow "ready" pulse — ESP-NOW homing
 *   1m:         Arrows activate — guiding
 *   20cm:       Precision — faster blink
 *   Beam break: Both solid — docking lock
 *
 * Layout (top view):
 *   Left IR    Center IR Beam    Right IR
 *     Sensor      (existing)      Sensor
 *   ← LEFT ARROW              RIGHT ARROW →
 *
 * Logic (when arrows active):
 *   Left sensor  → RIGHT arrow flashes (steer right)
 *   Right sensor → LEFT arrow flashes (steer left)
 *   Neither      → both blink (drive straight)
 ******************************************************************************/

#include "dock_alignment.h"
#include "dock_config.h"
#include "dock_hw.h"
#include "dock_protocol.h"
#include <Arduino.h>

static unsigned long s_lastBlink = 0;
static bool s_blinkState = false;
static uint8_t s_leftCount = 0;
static uint8_t s_rightCount = 0;
static uint8_t s_stageFromWallE = APPROACH_FAR;
static unsigned long s_lastStageMs = 0;
static bool s_lastDocked = false;
static uint8_t s_idleFlashCount = 0;
static bool s_idleLastOn = false;

#define ALIGN_DEBOUNCE_THRESHOLD  3
#define BLINK_MS_NORMAL    300
#define BLINK_MS_PRECISION 120
#define ARR_ON   (MOSFET_ARROW_ACTIVE_LOW ? LOW : HIGH)
#define ARR_OFF  (MOSFET_ARROW_ACTIVE_LOW ? HIGH : LOW)

void dockAlignmentBegin(void) {
  dockConfigureInputPin(PIN_ALIGN_LEFT, INPUT, "alignment left");
  dockConfigureInputPin(PIN_ALIGN_RIGHT, INPUT, "alignment right");

  dockConfigureOutputPin(PIN_ARROW_LEFT, ARR_OFF, "left arrow");
  dockConfigureOutputPin(PIN_ARROW_RIGHT, ARR_OFF, "right arrow");
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

  /* Detect docked -> not-docked transition. We no longer use the flash counter;
   * arrows should simply come back on solid once WALL-E has left.
   */
  if (s_lastDocked != docked) {
    s_lastDocked = docked;
  }

  /* Always debounce sensors (needed for timeout fallback) */
  if (dockDigitalReadSafe(PIN_ALIGN_LEFT, "alignment left", LOW))
    s_leftCount = (s_leftCount < 255) ? s_leftCount + 1 : 255;
  else
    s_leftCount = 0;
  if (dockDigitalReadSafe(PIN_ALIGN_RIGHT, "alignment right", LOW))
    s_rightCount = (s_rightCount < 255) ? s_rightCount + 1 : 255;
  else
    s_rightCount = 0;

  /* Beam break = docked — both arrows OFF (guidance not needed once docked) */
  if (docked) {
    setArrowState(ARR_OFF, ARR_OFF);
    return;
  }

  /* Effective stage: WALL-E's report if recent; else sensor fallback */
  uint8_t effStage = s_stageFromWallE;
  if (now - s_lastStageMs > APPROACH_STAGE_TIMEOUT_MS) {
    bool alignLeft = (s_leftCount > ALIGN_DEBOUNCE_THRESHOLD);
    bool alignRight = (s_rightCount > ALIGN_DEBOUNCE_THRESHOLD);
    effStage = (alignLeft || alignRight) ? APPROACH_1M : APPROACH_FAR;
  }

  /* FAR (>1m): arrows solid ON while dock is waiting / after exit */
  if (effStage == APPROACH_FAR) {
    setArrowState(ARR_ON, ARR_ON);
    return;
  }

  bool left = (s_leftCount > ALIGN_DEBOUNCE_THRESHOLD);
  bool right = (s_rightCount > ALIGN_DEBOUNCE_THRESHOLD);

  uint32_t blinkMs = (effStage == APPROACH_20CM) ? BLINK_MS_PRECISION : BLINK_MS_NORMAL;
  if (now - s_lastBlink > blinkMs) {
    s_blinkState = !s_blinkState;
    s_lastBlink = now;
  }

  if (left) {
    setArrowState(ARR_OFF, s_blinkState ? ARR_ON : ARR_OFF);
  } else if (right) {
    setArrowState(s_blinkState ? ARR_ON : ARR_OFF, ARR_OFF);
  } else {
    setArrowState(s_blinkState ? ARR_ON : ARR_OFF, s_blinkState ? ARR_ON : ARR_OFF);
  }
}
