/*******************************************************************************
 * dock_ir_guidance.cpp
 * IR beacon alignment helpers for dock (receivers on PIN_ALIGN_LEFT / RIGHT).
 * TSOP4838 / VS1838B: output LOW when modulated IR is detected, HIGH when idle.
 ******************************************************************************/

#include "dock_ir_guidance.h"
#include "dock_config.h"
#include "dock_hw.h"
#include "dock_protocol.h"
#include <Arduino.h>

#define IR_DEBOUNCE_THRESHOLD  3
#define IR_SAMPLE_INTERVAL_MS  16

typedef enum {
  IR_STABLE_LOST = 0,
  IR_STABLE_LEFT,
  IR_STABLE_RIGHT,
  IR_STABLE_CENTER
} IrStable_t;

static IrStable_t s_stable = IR_STABLE_LOST;
static IrStable_t s_prevPrinted = IR_STABLE_LOST;
static uint8_t s_leftHits = 0;
static uint8_t s_rightHits = 0;
static uint32_t s_lastSampleMs = 0;
static uint32_t s_lostSinceMs = 0;
static bool s_paused = false;
static bool s_haveLostAnchor = false;

static void readIRSensorsInstant(bool *leftDet, bool *rightDet) {
  int l = dockDigitalReadSafe(PIN_ALIGN_LEFT, "alignment left", HIGH);
  int r = dockDigitalReadSafe(PIN_ALIGN_RIGHT, "alignment right", HIGH);
  /* LOW = IR detected (TSOP active output) */
  *leftDet = (l == LOW);
  *rightDet = (r == LOW);
}

void readIRSensors(bool *leftDetected, bool *rightDetected) {
  if (!leftDetected || !rightDetected) return;
  readIRSensorsInstant(leftDetected, rightDetected);
}

static bool s_serialVerboseLast = true;

static void printCurrentGuidanceBlock(void) {
  switch (s_stable) {
    case IR_STABLE_LEFT:
      Serial.println(F("LEFT DETECTED"));
      Serial.println(F("WALL-E too far right -> steer LEFT"));
      break;
    case IR_STABLE_RIGHT:
      Serial.println(F("RIGHT DETECTED"));
      Serial.println(F("WALL-E too far left -> steer RIGHT"));
      break;
    case IR_STABLE_CENTER:
      Serial.println(F("CENTERED"));
      Serial.println(F("Aligned -> move forward"));
      break;
    default:
      Serial.println(F("SIGNAL LOST"));
      Serial.println(F("Signal lost -> stop or search"));
      break;
  }
}

void processDockGuidance(void) {
  if (!s_serialVerboseLast) return;
  printCurrentGuidanceBlock();
}

static IrStable_t classify(bool l, bool r) {
  if (l && r) return IR_STABLE_CENTER;
  if (l && !r) return IR_STABLE_LEFT;
  if (!l && r) return IR_STABLE_RIGHT;
  return IR_STABLE_LOST;
}

const char *getDockAlignment(void) {
  switch (s_stable) {
    case IR_STABLE_LEFT:   return "LEFT";
    case IR_STABLE_RIGHT:  return "RIGHT";
    case IR_STABLE_CENTER: return "CENTER";
    default:               return "LOST";
  }
}

uint8_t dockIrGuidanceStableId(void) { return (uint8_t)s_stable; }

uint8_t dockIrGuidanceGetBeaconHint(void) {
  if (s_paused) return DOCK_IR_ALIGN_PAUSE;
  switch (s_stable) {
    case IR_STABLE_LEFT:   return DOCK_IR_ALIGN_LEFT;
    case IR_STABLE_RIGHT:  return DOCK_IR_ALIGN_RIGHT;
    case IR_STABLE_CENTER: return DOCK_IR_ALIGN_CENTER;
    default:               return DOCK_IR_ALIGN_LOST;
  }
}

bool dockIrGuidanceGuidancePaused(void) { return s_paused; }

bool dockIrGuidanceAnyReceiverActive(void) {
  return s_stable == IR_STABLE_LEFT || s_stable == IR_STABLE_RIGHT || s_stable == IR_STABLE_CENTER;
}

void dockIrGuidanceBegin(void) {
  s_stable = IR_STABLE_LOST;
  s_prevPrinted = IR_STABLE_LOST;
  s_leftHits = s_rightHits = 0;
  s_lastSampleMs = 0;
  s_lostSinceMs = 0;
  s_paused = false;
  s_haveLostAnchor = false;
}

static void processDockGuidanceOnTransition(bool serialVerbose) {
  if (!serialVerbose) return;
  if (s_stable == s_prevPrinted) return;
  printCurrentGuidanceBlock();
  s_prevPrinted = s_stable;
}

void dockIrGuidanceUpdate(uint32_t now_ms, bool robotDocked, bool serialVerbose) {
  if (robotDocked) {
    IrStable_t was = s_stable;
    s_stable = IR_STABLE_CENTER;
    s_paused = false;
    s_haveLostAnchor = false;
    s_leftHits = s_rightHits = 0;
    s_serialVerboseLast = serialVerbose;
    if (serialVerbose && was != IR_STABLE_CENTER) {
      Serial.println(F("CENTERED"));
      Serial.println(F("Aligned -> move forward"));
    }
    s_prevPrinted = IR_STABLE_CENTER;
    return;
  }

  if ((int32_t)(now_ms - s_lastSampleMs) < (int32_t)IR_SAMPLE_INTERVAL_MS) {
    return;
  }
  s_lastSampleMs = now_ms;

  bool l, r;
  readIRSensorsInstant(&l, &r);

  if (l)
    s_leftHits = (s_leftHits < 255) ? s_leftHits + 1 : 255;
  else
    s_leftHits = 0;
  if (r)
    s_rightHits = (s_rightHits < 255) ? s_rightHits + 1 : 255;
  else
    s_rightHits = 0;

  bool debL = s_leftHits > IR_DEBOUNCE_THRESHOLD;
  bool debR = s_rightHits > IR_DEBOUNCE_THRESHOLD;
  s_stable = classify(debL, debR);

  if (s_stable == IR_STABLE_LOST) {
    if (!s_haveLostAnchor) {
      s_lostSinceMs = now_ms;
      s_haveLostAnchor = true;
    }
    if (!s_paused && (now_ms - s_lostSinceMs) >= (uint32_t)DOCK_IR_LOST_TIMEOUT_MS) {
      s_paused = true;
      if (serialVerbose) {
        Serial.println(F("SIGNAL LOST (timeout) -> guidance paused"));
      }
    }
  } else {
    s_haveLostAnchor = false;
    s_paused = false;
  }

  s_serialVerboseLast = serialVerbose;
  processDockGuidanceOnTransition(serialVerbose);
}

bool dockIrGuidanceDebugLeftOn(void) {
#if DOCK_IR_NEOPIXEL_DEBUG
  return s_leftHits > IR_DEBOUNCE_THRESHOLD;
#else
  return false;
#endif
}

bool dockIrGuidanceDebugRightOn(void) {
#if DOCK_IR_NEOPIXEL_DEBUG
  return s_rightHits > IR_DEBOUNCE_THRESHOLD;
#else
  return false;
#endif
}
