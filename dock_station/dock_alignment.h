/*******************************************************************************
 * dock_alignment.h
 * Dock alignment sensors + arrow LED indicators
 *
 * Approach Mode staging:
 *   FAR (5m): ESP-NOW homing, arrows off or slow "ready" pulse
 *   1m:      Arrows activate (guiding)
 *   20cm:    Precision alignment (faster blink)
 *   Beam:    Docking lock (both solid)
 *
 * Hardware: many builds use a **left + right IR transmitter** pair on the dock for alignment,
 * with **receiver** modules (or a pair of detectors) wired to the ESP32. This code only reads
 * those receiver pins — it does not drive the IR LEDs (they are usually always powered).
 *
 * steer left → right arrow flashes; steer right → left arrow flashes;
 * centered → both blink; docked → both off.
 ******************************************************************************/

#ifndef DOCK_ALIGNMENT_H
#define DOCK_ALIGNMENT_H

#include <stdint.h>

void dockAlignmentBegin(void);
/* IR arrow guidance: only runs in STATE_NOT_DOCKED; other states force arrows off (saves MOSFET heat). */
void dockAlignmentUpdate(bool docked);

/* WALL-E sends DOCK_CMD_DOCKING_ARM before dock may drive arrows; cleared on DISARM or when leaving NOT_DOCKED. */
void dockAlignmentSetDockingArmed(bool armed);

/* WALL-E sends stage via ESP-NOW (overrides sensor fallback until timeout) */
void dockAlignmentSetStage(uint8_t stage);

#endif /* DOCK_ALIGNMENT_H */
