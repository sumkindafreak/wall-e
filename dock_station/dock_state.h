/*******************************************************************************
 * dock_state.h
 * State machine for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_STATE_H
#define DOCK_STATE_H

#include "dock_protocol.h"

/* Returns current state */
DockState dockStateGet(void);

/* Call every loop. Reads sensors, updates state, controls MOSFET.
 * Returns true if state changed (for debug logging). */
bool dockStateUpdate(void);

/* Charge enable output: call after dockStateUpdate to apply MOSFET */
bool dockChargeEnabled(void);

/* For NeoPixel: map DockState to NeoPixelState (skip BOOT/NOT_DOCKED mapping) */
int dockStateToNeoPixelState(DockState s);

/* Remote command handlers (from WALL-E via ESP-NOW) */
void dockStateForceOff(void);   /* Force charging off, enter FAULT */
void dockStateResetFault(void); /* Clear FAULT, return to NOT_DOCKED or DOCKED_IDLE */

#endif /* DOCK_STATE_H */
