/*******************************************************************************
 * dock_callout.h
 * Call WALL-E push button + MOSFET light show (arrows + internal LED)
 *
 * Press once = callout starts (light show). Callout stops when WALL-E docks
 * (beam broken) or user presses the button again (cancel).
 ******************************************************************************/

#ifndef DOCK_CALLOUT_H
#define DOCK_CALLOUT_H

#include <stdbool.h>

void dockCalloutBegin(void);
void dockCalloutUpdate(void);
bool dockCalloutIsActive(void);

#endif /* DOCK_CALLOUT_H */
