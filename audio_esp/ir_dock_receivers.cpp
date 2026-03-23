/**
 * ir_dock_receivers.cpp — IR dock left/right with debounce
 */
#include "ir_dock_receivers.h"

static DockIrState s_state = DOCK_IR_NONE;
static bool s_leftRaw = false;
static bool s_rightRaw = false;
static unsigned long s_leftStableTime = 0;
static unsigned long s_rightStableTime = 0;
static bool s_leftLast = false;
static bool s_rightLast = false;
static int s_unstableCount = 0;
#define UNSTABLE_THRESHOLD 5

static bool readIr(uint8_t pin) {
  bool v = (digitalRead(pin) == LOW);
#if IR_ACTIVE_LOW
  return v;  /* LOW = IR detected */
#else
  return !v;
#endif
}

void irDockInit() {
  pinMode(PIN_IR_DOCK_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_DOCK_RIGHT, INPUT_PULLUP);
  s_leftRaw = readIr(PIN_IR_DOCK_LEFT);
  s_rightRaw = readIr(PIN_IR_DOCK_RIGHT);
  s_leftLast = s_leftRaw;
  s_rightLast = s_rightRaw;
  s_leftStableTime = millis();
  s_rightStableTime = millis();
  s_state = DOCK_IR_NONE;
  s_unstableCount = 0;
  DEBUG_LOG("IR dock init");
}

void irDockTick() {
  unsigned long now = millis();
  bool leftNow = readIr(PIN_IR_DOCK_LEFT);
  bool rightNow = readIr(PIN_IR_DOCK_RIGHT);

  if (leftNow != s_leftLast) s_leftStableTime = now;
  if (rightNow != s_rightLast) s_rightStableTime = now;
  s_leftLast = leftNow;
  s_rightLast = rightNow;

  if (now - s_leftStableTime >= IR_DEBOUNCE_MS) s_leftRaw = leftNow;
  if (now - s_rightStableTime >= IR_DEBOUNCE_MS) s_rightRaw = rightNow;

  DockIrState next;
  if (s_leftRaw && s_rightRaw)
    next = DOCK_IR_BOTH;
  else if (s_leftRaw)
    next = DOCK_IR_LEFT;
  else if (s_rightRaw)
    next = DOCK_IR_RIGHT;
  else
    next = DOCK_IR_NONE;

  if (next != s_state) {
    s_unstableCount++;
    if (s_unstableCount >= UNSTABLE_THRESHOLD)
      s_state = DOCK_IR_UNSTABLE;
    else
      s_state = next;
  } else {
    s_unstableCount = 0;
    s_state = next;
  }
}

DockIrState irDockGetState() { return s_state; }
