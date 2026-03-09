/*******************************************************************************
 * dock_neopixel.h
 * Non-blocking NeoPixel state-driven LED patterns for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_NEOPIXEL_H
#define DOCK_NEOPIXEL_H

#include <stdint.h>

/* State values match DockState in dock_protocol.h */
enum NeoPixelState {
  NP_STATE_NOT_DOCKED,
  NP_STATE_DOCKED_IDLE,
  NP_STATE_CHARGING,
  NP_STATE_CHARGED,
  NP_STATE_FAULT
};

/* Call once at startup */
void dockNeoPixelBegin(void);

/* Call every loop. Fully non-blocking, millis-based. */
void dockNeoPixelUpdate(NeoPixelState state, bool mouth_blocked_warn);

/* Optional: set global brightness 0-255 */
void dockNeoPixelSetBrightness(uint8_t b);

#endif /* DOCK_NEOPIXEL_H */
