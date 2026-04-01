/*******************************************************************************
 * dock_callout.cpp
 * Call WALL-E push button + MOSFET light show
 *
 * Push once = callout starts (arrows + internal LED chase). Callout stops when:
 *   - WALL-E docks (beam broken), or
 *   - User presses the button again (cancel).
 ******************************************************************************/

#include "dock_callout.h"
#include "dock_config.h"
#include "dock_hw.h"
#include "dock_ir_guidance.h"
#include "dock_sensors.h"
#include <Arduino.h>

#define CALLOUT_DEBOUNCE_MS   50
#define STEP_MS               120
#define ARR_ON   (MOSFET_ARROW_ACTIVE_LOW ? LOW : HIGH)
#define ARR_OFF  (MOSFET_ARROW_ACTIVE_LOW ? HIGH : LOW)
#define INT_ON   (MOSFET_INTERNAL_ACTIVE_LOW ? LOW : HIGH)
#define INT_OFF  (MOSFET_INTERNAL_ACTIVE_LOW ? HIGH : LOW)

static bool s_callout = false;
static bool s_last_switch = true;   /* true = released (pullup), false = pressed */
static bool s_was_pressed = false;  /* for push-button edge: only toggle on new press */
static uint32_t s_switch_stable_ms = 0;
static uint8_t s_phase = 0;
static uint32_t s_last_step_ms = 0;
static bool s_last_docked = false;
static uint8_t s_idle_led_flash_count = 0;
static bool s_idle_led_last_on = false;

/* Arrows only when TSOP receivers see WALL-E's modulated IR — avoids hot MOSFETs when idle/callout. */
static uint8_t gateArrow(uint8_t wantOn) {
  return dockIrGuidanceAnyReceiverActive() ? wantOn : ARR_OFF;
}

static void setCalloutOutputs(uint8_t leftLevel, uint8_t rightLevel, uint8_t internalLevel) {
  dockWriteOutputPin(PIN_ARROW_LEFT, gateArrow(leftLevel), "left arrow");
  dockWriteOutputPin(PIN_ARROW_RIGHT, gateArrow(rightLevel), "right arrow");
  dockWriteOutputPin(PIN_INTERNAL_LED, internalLevel, "internal LED");
}

void dockCalloutBegin(void) {
  dockConfigureInputPin(PIN_CALL_SWITCH, INPUT_PULLUP, "call switch");
  s_last_switch = (dockDigitalReadSafe(PIN_CALL_SWITCH, "call switch", HIGH) == LOW);
  s_switch_stable_ms = millis();
  s_last_docked = dockRobotInSlot();
}

bool dockCalloutIsActive(void) {
  return s_callout;
}

void dockCalloutUpdate(void) {
  uint32_t now = millis();
  bool sw = (dockDigitalReadSafe(PIN_CALL_SWITCH, "call switch", HIGH) == LOW);  /* LOW = pressed */
  bool docked = dockRobotInSlot();

  /* Track docked state; we no longer use a flash counter here. */
  s_last_docked = docked;

  /* Debounce */
  if (sw != s_last_switch) {
    s_last_switch = sw;
    s_switch_stable_ms = now;
  }
  if (now - s_switch_stable_ms < CALLOUT_DEBOUNCE_MS) {
    /* Still debouncing: if callout active, keep running light show */
    if (s_callout) {
      if (now - s_last_step_ms >= STEP_MS) {
        s_last_step_ms = now;
        s_phase = (s_phase + 1) % 5;
      }
      switch (s_phase) {
        case 0: setCalloutOutputs(ARR_ON, ARR_OFF, INT_OFF); break;
        case 1: setCalloutOutputs(ARR_OFF, ARR_ON, INT_OFF); break;
        case 2: setCalloutOutputs(ARR_OFF, ARR_OFF, INT_ON); break;
        case 3: setCalloutOutputs(ARR_ON, ARR_ON, INT_ON); break;
        default: setCalloutOutputs(ARR_OFF, ARR_OFF, INT_OFF); break;
      }
    }
    return;
  }

  /* Stable switch state: s_last_switch = current (true = pressed = pin LOW, false = released) */
  bool pressed = s_last_switch;

  /* Stop callout when WALL-E docks */
  if (s_callout && docked) {
    s_callout = false;
  }

  /* Push button: on press edge (was released, now pressed), toggle callout */
  if (pressed) {
    if (!s_was_pressed) {
      s_was_pressed = true;
      s_callout = !s_callout;  /* first press = start, second press = stop */
    }
  } else {
    s_was_pressed = false;  /* released, so next press will be a new edge */
  }

  if (!s_callout) {
    /* When not in callout mode:
     * - If docked: interior OFF.
     * - If reversing / just left dock or idle: interior ON solid.
     *   Arrows remain OFF here; alignment logic handles arrow patterns.
     */
    if (docked) {
      setCalloutOutputs(ARR_OFF, ARR_OFF, INT_OFF);
    } else {
      setCalloutOutputs(ARR_OFF, ARR_OFF, INT_ON);
    }
    return;
  }

  /* Callout light show: chase left → right → internal → flash all → repeat */
  if (now - s_last_step_ms >= STEP_MS) {
    s_last_step_ms = now;
    s_phase = (s_phase + 1) % 5;
  }

  switch (s_phase) {
    case 0:
      setCalloutOutputs(ARR_ON, ARR_OFF, INT_OFF);
      break;
    case 1:
      setCalloutOutputs(ARR_OFF, ARR_ON, INT_OFF);
      break;
    case 2:
      setCalloutOutputs(ARR_OFF, ARR_OFF, INT_ON);
      break;
    case 3:
      setCalloutOutputs(ARR_ON, ARR_ON, INT_ON);
      break;
    default:
      setCalloutOutputs(ARR_OFF, ARR_OFF, INT_OFF);
      break;
  }
}
