/*******************************************************************************
 * VL6180X time-of-flight (e.g. TOF050C breakout) — short-range dock presence.
 * I2C @ 0x29. Typical range ~20–100 mm (good for "robot nose in slot").
 ******************************************************************************/
#ifndef DOCK_VL6180_H
#define DOCK_VL6180_H

#include <stdbool.h>

void dockVl6180Begin(void);
void dockVl6180Update(void);

/** Last range in mm, or 0 if invalid / no reading. */
uint8_t dockVl6180RangeMm(void);

/** True when a solid target is within dock distance band (see dock_config). */
bool dockVl6180Docked(void);

/** False if begin() failed — caller should fall back to sonar / mouth sensors. */
bool dockVl6180Ready(void);

#endif
