/*******************************************************************************
 * dock_neopixel.h
 * FastLED-based status & fault indicators for Smart Charging Crate
 *
 * Intelligent fault finding:
 *   FAULT: N red blinks = fault code N (1=overcurrent, 2=force_off)
 *   Then: segment display — beam | mouth | current bar
 ******************************************************************************/

#ifndef DOCK_NEOPIXEL_H
#define DOCK_NEOPIXEL_H

#include <stdint.h>
#include "dock_state.h"

/* State values (match DockState + CALLOUT) */
enum NeoPixelState {
  NP_STATE_NOT_DOCKED,
  NP_STATE_DOCKED_IDLE,
  NP_STATE_CHARGING,
  NP_STATE_CHARGED,
  NP_STATE_FAULT,
  NP_STATE_CALLOUT
};

/* Call once at startup */
void dockNeoPixelBegin(void);

/* Call every loop. Pass fault_code for intelligent fault display. */
void dockNeoPixelUpdate(NeoPixelState state, bool mouth_blocked_warn);
void dockNeoPixelUpdateEx(NeoPixelState state, bool mouth_blocked_warn,
  DockFaultCode fault_code, bool beam_present, bool mouth_blocked, float current_amps);

/* Optional: set global brightness 0-255 */
void dockNeoPixelSetBrightness(uint8_t b);

#endif /* DOCK_NEOPIXEL_H */
