/*******************************************************************************
 * dock_state.cpp
 * State machine and MOSFET control
 ******************************************************************************/

#include "dock_config.h"
#include "dock_state.h"
#include "dock_hw.h"
#include "dock_sensors.h"
#include <Arduino.h>

#ifndef OBSTACLES_REQUIRED_FOR_CHARGE
#define OBSTACLES_REQUIRED_FOR_CHARGE  3
#endif
#ifndef OBSTACLES_CLEAR_LEAVE_MS
#define OBSTACLES_CLEAR_LEAVE_MS  500
#endif
#ifndef BLOCKED_BEFORE_CHARGE_MS
#define BLOCKED_BEFORE_CHARGE_MS  2500
#endif
#ifndef CLEAR_STOP_CHARGE_DEBOUNCE_MS
#define CLEAR_STOP_CHARGE_DEBOUNCE_MS  600
#endif

static DockState g_state = STATE_BOOT;
static DockFaultCode g_fault_code = FAULT_NONE;
static bool g_charge_enabled = false;
static bool g_charge_requested = false;      /* Set by REQUEST_CHARGE from WALL-E */
static uint32_t g_dock_detected_at = 0;      /* When beam first broke */
static uint32_t g_charge_enabled_at = 0;     /* When we turned MOSFET on */
static uint32_t g_charged_low_current_since = 0;  /* When current dropped below CHARGED threshold */
static uint32_t g_charged_entered_at = 0;    /* When we entered STATE_CHARGED (for idle timeout) */
static bool g_had_charge_enable_debounce = false;
static bool g_idle_mode = false;
static uint32_t g_blocked_since = 0;            /* Blocked this long in DOCKED_IDLE → start charging */
static uint32_t g_clear_since = 0;              /* Clear this long in CHARGING → stop charging */
static uint32_t g_obstacles_clear_since = 0;   /* No obstacles this long → robot left (NOT_DOCKED) */
static uint32_t g_beam_broken_since = 0;   /* Beam broken debounce: NOT_DOCKED -> DOCKED_IDLE */
static uint32_t g_beam_clear_since = 0;    /* Beam clear debounce: docked -> NOT_DOCKED */

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

static void setFault(DockFaultCode code) {
  g_charge_enabled = false;
  g_fault_code = code;
  dockChargeGateWrite(false);
  setState(STATE_FAULT);
}

DockFaultCode dockStateGetFaultCode(void) {
  return g_state == STATE_FAULT ? g_fault_code : FAULT_NONE;
}

bool dockIsIdleMode(void) {
  return g_idle_mode;
}

bool dockStateUpdate(void) {
  DockState prev = g_state;
  bool beam = dockDockDetected();  /* beam or sonar fallback when USE_SONAR */
  /* Charging only when at least OBSTACLES_REQUIRED_FOR_CHARGE (e.g. 3) obstacles blocked */
  bool blocked = dockAtLeastNObstaclesBlocked(OBSTACLES_REQUIRED_FOR_CHARGE);
  float i = dockCurrentAmps();
  bool current_sense_ok = dockCurrentSenseAvailable();

  /* Abs value for overcurrent check */
  float i_abs = (i < 0) ? -i : i;

  /* --- Safety: overcurrent faults only when charge gate is ON --- */
#if !OVERCURRENT_FAULT_DISABLED
  if (current_sense_ok && g_charge_enabled && i_abs > CURRENT_OVERCURRENT_A) {
    setFault(FAULT_OVERCURRENT);
    return (g_state != prev);
  }
#endif

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
    g_charge_requested = false;
    g_idle_mode = false;
    g_beam_clear_since = 0;
    dockChargeGateWrite(false);
    if (beam) {
      uint32_t now = millis();
      if (g_beam_broken_since == 0)
        g_beam_broken_since = now;
      else if ((now - g_beam_broken_since) >= (uint32_t)BEAM_BROKEN_DEBOUNCE_MS) {
        g_dock_detected_at = now;
        g_had_charge_enable_debounce = false;
        g_beam_broken_since = 0;
        setState(STATE_DOCKED_IDLE);
      }
    } else {
      g_beam_broken_since = 0;
    }
    return (g_state != prev);
  }

  /* --- STATE_DOCKED_IDLE --- */
  if (g_state == STATE_DOCKED_IDLE) {
    uint32_t now = millis();
    g_beam_broken_since = 0;
    /* Robot left: beam clear OR no obstacles (all clear) */
    if (!beam) {
      g_charge_enabled = false;
      dockChargeGateWrite(false);
      if (g_beam_clear_since == 0)
        g_beam_clear_since = now;
      else if ((now - g_beam_clear_since) >= (uint32_t)BEAM_CLEAR_DEBOUNCE_MS) {
        g_beam_clear_since = 0;
        g_obstacles_clear_since = 0;
        setState(STATE_NOT_DOCKED);
        return (g_state != prev);
      }
    } else {
      g_beam_clear_since = 0;
    }
    /* Do NOT treat "obstacles all clear" as robot leaving if the beam is present.
     * For this build, the IR beam is the primary dock presence sensor; obstacles
     * only gate charging. So only beam-clear should drive NOT_DOCKED.
     */
    g_obstacles_clear_since = 0;
    if (!beam)
      return (g_state != prev);

    now = millis();

    /* Debounce: wait DOCK_DEBOUNCE_MS after dock before allowing charge logic */
    if (!g_had_charge_enable_debounce) {
      if ((now - g_dock_detected_at) >= DOCK_DEBOUNCE_MS) {
        g_had_charge_enable_debounce = true;
      }
    }

    /* Charge only when obstacles BLOCKED for BLOCKED_BEFORE_CHARGE_MS (robot in mouth). When clear, gate off. */
    if (!blocked) {
      g_blocked_since = 0;
      g_charge_enabled = false;
      dockChargeGateWrite(false);
      return (g_state != prev);
    }
    if (!g_had_charge_enable_debounce || !dockChargeGateAvailable())
      return (g_state != prev);

    if (g_blocked_since == 0)
      g_blocked_since = now;
    else if ((now - g_blocked_since) >= (uint32_t)BLOCKED_BEFORE_CHARGE_MS) {
      g_charge_enabled = true;
      dockChargeGateWrite(true);
      if (g_charge_enabled_at == 0)
        g_charge_enabled_at = now;
      g_charged_low_current_since = 0;
      g_blocked_since = 0;
      setState(STATE_CHARGING);
    }
    return (g_state != prev);
  }

  /* --- STATE_CHARGING --- */
  if (g_state == STATE_CHARGING) {
    uint32_t now = millis();
    g_beam_broken_since = 0;

    if (!beam) {
      g_charge_enabled = false;
      dockChargeGateWrite(false);
      if (g_beam_clear_since == 0)
        g_beam_clear_since = now;
      else if ((now - g_beam_clear_since) >= (uint32_t)BEAM_CLEAR_DEBOUNCE_MS) {
        g_beam_clear_since = 0;
        g_obstacles_clear_since = 0;
        setState(STATE_NOT_DOCKED);
        return (g_state != prev);
      }
    } else {
      g_beam_clear_since = 0;
    }
    /* Same policy as DOCKED_IDLE: only beam-clear means "robot left".
     * Obstacles clear just stop charging via the blocked/clear logic below.
     */
    g_obstacles_clear_since = 0;
    if (!beam)
      return (g_state != prev);

    /* When obstacles CLEAR, stop charging (gate off, back to DOCKED_IDLE). When blocked, keep charging. */
    if (blocked) {
      g_clear_since = 0;
    } else {
      g_charge_enabled = false;
      dockChargeGateWrite(false);
      /* Debounce clear so brief sensor glitches don't bounce state */
      if (g_clear_since == 0)
        g_clear_since = now;
      else if ((now - g_clear_since) >= (uint32_t)CLEAR_STOP_CHARGE_DEBOUNCE_MS) {
        g_clear_since = 0;
        setState(STATE_DOCKED_IDLE);
        g_blocked_since = 0;
        return (g_state != prev);
      }
    }

    /* Current below CHARGED threshold? Start timer */
    if (current_sense_ok && i_abs < CURRENT_CHARGED_A) {
      if (g_charged_low_current_since == 0) {
        g_charged_low_current_since = now;
      } else if ((now - g_charged_low_current_since) >= CHARGED_STABLE_MS) {
        /* Stable low current for 90s -> CHARGED, turn off MOSFET */
        g_charge_enabled = false;
        dockChargeGateWrite(false);
        g_charged_entered_at = now;
        g_idle_mode = false;
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
    uint32_t now = millis();
    g_charge_enabled = false;
    dockChargeGateWrite(false);
    g_beam_broken_since = 0;

#if (IDLE_AFTER_CHARGED_MS > 0)
    if ((now - g_charged_entered_at) >= IDLE_AFTER_CHARGED_MS) {
      g_idle_mode = true;
    }
#endif

    if (!beam) {
      if (g_beam_clear_since == 0)
        g_beam_clear_since = now;
      else if ((now - g_beam_clear_since) >= (uint32_t)BEAM_CLEAR_DEBOUNCE_MS) {
        g_beam_clear_since = 0;
        g_obstacles_clear_since = 0;
        setState(STATE_NOT_DOCKED);
        g_idle_mode = false;
        return (g_state != prev);
      }
    } else {
      g_beam_clear_since = 0;
    }
    /* In CHARGED, keep showing "docked" as long as the beam is present;
     * leaving the dock is driven purely by beam clear.
     */
    g_obstacles_clear_since = 0;
    return (g_state != prev);
  }

  /* --- STATE_FAULT --- */
  if (g_state == STATE_FAULT) {
    g_charge_enabled = false;
    dockChargeGateWrite(false);
    /* Stay in FAULT until dockStateResetFault() called */
  }

  return (g_state != prev);
}

void dockStateForceOff(void) {
  g_charge_enabled = false;
  g_fault_code = FAULT_FORCE_OFF;
  dockChargeGateWrite(false);
  setState(STATE_FAULT);
}

void dockStateRequestCharge(void) {
  if (g_state == STATE_DOCKED_IDLE && dockDockDetected()) {
    g_charge_requested = true;
  }
}

void dockStateResetFault(void) {
  if (g_state != STATE_FAULT) return;
  g_charge_enabled = false;
  g_fault_code = FAULT_NONE;
  dockChargeGateWrite(false);
  if (dockDockDetected()) {
    g_dock_detected_at = millis();
    g_had_charge_enable_debounce = false;
    setState(STATE_DOCKED_IDLE);
  } else {
    setState(STATE_NOT_DOCKED);
  }
}
