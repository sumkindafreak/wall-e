/*******************************************************************************
 * dock_state.cpp
 * State machine and MOSFET control
 ******************************************************************************/

#include "dock_state.h"
#include "dock_config.h"
#include "dock_sensors.h"
#include <Arduino.h>

static DockState g_state = STATE_BOOT;
static bool g_charge_enabled = false;
static uint32_t g_dock_detected_at = 0;      /* When beam first broke */
static uint32_t g_charge_enabled_at = 0;     /* When we turned MOSFET on */
static uint32_t g_charged_low_current_since = 0;  /* When current dropped below CHARGED threshold */
static bool g_had_charge_enable_debounce = false;

DockState dockStateGet(void) {
  return g_state;
}

bool dockChargeEnabled(void) {
  return g_charge_enabled;
}

int dockStateToNeoPixelState(DockState s) {
  switch (s) {
    case STATE_BOOT:
    case STATE_NOT_DOCKED: return 0;  /* NP_STATE_NOT_DOCKED */
    case STATE_DOCKED_IDLE: return 1;
    case STATE_CHARGING: return 2;
    case STATE_CHARGED: return 3;
    case STATE_FAULT: return 4;
    default: return 0;
  }
}

static void setState(DockState s) {
  if (g_state != s) {
    g_state = s;
  }
}

static void setFault(void) {
  g_charge_enabled = false;
  digitalWrite(PIN_MOSFET_GATE, LOW);
  setState(STATE_FAULT);
}

bool dockStateUpdate(void) {
  DockState prev = g_state;
  bool beam = dockBeamPresent();
  bool blocked = dockMouthBlocked();
  float i = dockCurrentAmps();

  /* Abs value for overcurrent check */
  float i_abs = (i < 0) ? -i : i;

  /* --- Safety: overcurrent always faults --- */
  if (i_abs > CURRENT_OVERCURRENT_A) {
    setFault();
    return (g_state != prev);
  }

  /* --- STATE_BOOT: transition to NOT_DOCKED after first sensor read --- */
  if (g_state == STATE_BOOT) {
    setState(beam ? STATE_DOCKED_IDLE : STATE_NOT_DOCKED);
    if (beam) {
      g_dock_detected_at = millis();
    }
    return (g_state != prev);
  }

  /* --- STATE_NOT_DOCKED --- */
  if (g_state == STATE_NOT_DOCKED) {
    g_charge_enabled = false;
    digitalWrite(PIN_MOSFET_GATE, LOW);
    if (beam) {
      g_dock_detected_at = millis();
      g_had_charge_enable_debounce = false;
      setState(STATE_DOCKED_IDLE);
    }
    return (g_state != prev);
  }

  /* --- STATE_DOCKED_IDLE --- */
  if (g_state == STATE_DOCKED_IDLE) {
    g_charge_enabled = false;
    digitalWrite(PIN_MOSFET_GATE, LOW);

    if (!beam) {
      setState(STATE_NOT_DOCKED);
      return (g_state != prev);
    }

    /* Debounce: wait DOCK_DEBOUNCE_MS before enabling charge */
    uint32_t now = millis();
    if (!g_had_charge_enable_debounce) {
      if ((now - g_dock_detected_at) >= DOCK_DEBOUNCE_MS) {
        g_had_charge_enable_debounce = true;
      }
    }

    /* Enable charging only if mouth clear (or obstacles disabled) and debounce done */
    if (g_had_charge_enable_debounce && !blocked) {
      g_charge_enabled = true;
      digitalWrite(PIN_MOSFET_GATE, HIGH);
      g_charge_enabled_at = now;
      g_charged_low_current_since = 0;
      setState(STATE_CHARGING);
    }
    return (g_state != prev);
  }

  /* --- STATE_CHARGING --- */
  if (g_state == STATE_CHARGING) {
    if (!beam) {
      g_charge_enabled = false;
      digitalWrite(PIN_MOSFET_GATE, LOW);
      setState(STATE_NOT_DOCKED);
      return (g_state != prev);
    }

    if (blocked) {
      /* Mouth blocked: disable charging, back to DOCKED_IDLE */
      g_charge_enabled = false;
      digitalWrite(PIN_MOSFET_GATE, LOW);
      setState(STATE_DOCKED_IDLE);
      g_had_charge_enable_debounce = true;  /* Don't re-enable until clear */
      return (g_state != prev);
    }

    uint32_t now = millis();

    /* Current below CHARGED threshold? Start timer */
    if (i_abs < CURRENT_CHARGED_A) {
      if (g_charged_low_current_since == 0) {
        g_charged_low_current_since = now;
      } else if ((now - g_charged_low_current_since) >= CHARGED_STABLE_MS) {
        /* Stable low current for 90s -> CHARGED, turn off MOSFET */
        g_charge_enabled = false;
        digitalWrite(PIN_MOSFET_GATE, LOW);
        setState(STATE_CHARGED);
        return (g_state != prev);
      }
    } else {
      g_charged_low_current_since = 0;
    }

    return (g_state != prev);
  }

  /* --- STATE_CHARGED --- */
  if (g_state == STATE_CHARGED) {
    g_charge_enabled = false;
    digitalWrite(PIN_MOSFET_GATE, LOW);

    if (!beam) {
      setState(STATE_NOT_DOCKED);
    }
    return (g_state != prev);
  }

  /* --- STATE_FAULT --- */
  if (g_state == STATE_FAULT) {
    g_charge_enabled = false;
    digitalWrite(PIN_MOSFET_GATE, LOW);
    /* Stay in FAULT until dockStateResetFault() called */
  }

  return (g_state != prev);
}

void dockStateForceOff(void) {
  g_charge_enabled = false;
  digitalWrite(PIN_MOSFET_GATE, LOW);
  setState(STATE_FAULT);
}

void dockStateResetFault(void) {
  if (g_state != STATE_FAULT) return;
  g_charge_enabled = false;
  digitalWrite(PIN_MOSFET_GATE, LOW);
  if (dockBeamPresent()) {
    g_dock_detected_at = millis();
    g_had_charge_enable_debounce = false;
    setState(STATE_DOCKED_IDLE);
  } else {
    setState(STATE_NOT_DOCKED);
  }
}
