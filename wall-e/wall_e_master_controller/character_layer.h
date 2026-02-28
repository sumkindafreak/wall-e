// ============================================================
//  WALL-E Master Controller — Character Layer
//  Small 24x16 eye graphic: blink, mood width, E-STOP flash
// ============================================================

#ifndef CHARACTER_LAYER_H
#define CHARACTER_LAYER_H

#include <stdint.h>
#include <stdbool.h>

// Eye region (top-right corner)
#define EYELET_X  296
#define EYELET_Y  8
#define EYELET_W  24
#define EYELET_H  16

void charLayerInit(void);
void charLayerUpdate(unsigned long now);   // Call each loop — handles blink timer
void charLayerDraw(uint8_t moodState, bool estop);  // moodState: 0-4, estop: flash wide

#endif // CHARACTER_LAYER_H
