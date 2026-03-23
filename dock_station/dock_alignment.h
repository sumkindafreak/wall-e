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
 * Left/right IR sensors detect which side WALL-E is closer to.
 * steer left → right arrow flashes; steer right → left arrow flashes;
 * centered → both blink; docked → both solid.
 ******************************************************************************/

#ifndef DOCK_ALIGNMENT_H
#define DOCK_ALIGNMENT_H

#include <stdint.h>

void dockAlignmentBegin(void);
void dockAlignmentUpdate(bool docked);

/* WALL-E sends stage via ESP-NOW (overrides sensor fallback until timeout) */
void dockAlignmentSetStage(uint8_t stage);

#endif /* DOCK_ALIGNMENT_H */
