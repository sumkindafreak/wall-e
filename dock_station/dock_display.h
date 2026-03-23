/*******************************************************************************
 * dock_display.h
 * SSD1306 128x64 OLED status display
 ******************************************************************************/

#ifndef DOCK_DISPLAY_H
#define DOCK_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

void dockDisplayBegin(void);
void dockDisplayUpdate(void);

#endif /* DOCK_DISPLAY_H */
