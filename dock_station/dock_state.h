/*******************************************************************************
 * dock_state.h
 * State machine for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_STATE_H
#define DOCK_STATE_H

#include "dock_protocol.h"

/* Returns current state */
DockState dockStateGet(void);

/* Fault reason (when state == STATE_FAULT). For LED fault-code display. */
typedef enum {
  FAULT_NONE = 0,
  FAULT_OVERCURRENT = 1,   /* Current > 3A */
  FAULT_FORCE_OFF = 2      /* DOCK_CMD_FORCE_OFF from WALL-E */
} DockFaultCode;
DockFaultCode dockStateGetFaultCode(void);

/* Call every loop. Reads sensors, updates state, controls MOSFET.
 * Returns true if state changed (for debug logging). */
bool dockStateUpdate(void);

/* Charge enable output: call after dockStateUpdate to apply MOSFET */
bool dockChargeEnabled(void);

/* For NeoPixel: map DockState to NeoPixelState (skip BOOT/NOT_DOCKED mapping) */
int dockStateToNeoPixelState(DockState s);

/* True after CHARGED for IDLE_AFTER_CHARGED_MS (dim display/LEDs, show Idle). Cleared when leaving CHARGED. */
bool dockIsIdleMode(void);

/* True when NOT_DOCKED, quiet bay (no presence) for BAY_IDLE_AFTER_MS — arrows stay off until WALL-E arms docking. */
bool dockIsBayIdle(void);
void dockStateClearBayIdle(void);

/* Short name for Serial (includes STANDBY when bay idle). */
const char* dockStateNameForSerial(void);

/* Remote command handlers (from WALL-E via ESP-NOW) */
void dockStateForceOff(void);     /* Force charging off, enter FAULT */
void dockStateResetFault(void);   /* Clear FAULT, return to NOT_DOCKED or DOCKED_IDLE */
void dockStateRequestCharge(void); /* WALL-E requests charge; enables MOSFET when beam broken */

#endif /* DOCK_STATE_H */
