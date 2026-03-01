// ============================================================
//  WALL-E Master Controller — Animation System
//  millis()-driven, non-blocking, region-based redraw only
// ============================================================

#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_state.h"
#include "protocol.h"

#define EYE_REGION_W  32
#define EYE_REGION_H  32
#define EYE_REGION_X  (320 - EYE_REGION_W - 4)
#define EYE_REGION_Y  4

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void animInit(void);
void animUpdate(unsigned long now);  // Call each loop — blink, pulse timers
void animDrawEye(uint8_t moodState, bool estop, bool forceRedraw);  // Region-only
float animGetPulseBrightness(void);  // For status glow: sin(millis*0.005)*0.5+0.5

#endif // ANIMATION_SYSTEM_H
