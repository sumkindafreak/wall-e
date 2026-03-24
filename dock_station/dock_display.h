/*******************************************************************************
 * dock_display.h
 * Dock status display API — uses the same graphite + amber *theme* as the CYD
 * master UI (RGB565 palette / layout cues). Panel driver and resolution unchanged.
 ******************************************************************************/

#ifndef DOCK_DISPLAY_H
#define DOCK_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

void dockDisplayBegin(void);
void dockDisplayUpdate(void);

#endif /* DOCK_DISPLAY_H */
