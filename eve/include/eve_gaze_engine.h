/**
 * EVE face — eased gaze (left/right/up/down, track, return centre).
 * No LVGL types; drives eveExpressionSetTargetGaze when active.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "eve_target_tracker.h"

typedef enum {
  EVE_GAZE_CENTER = 0,
  EVE_GAZE_LEFT,
  EVE_GAZE_RIGHT,
  EVE_GAZE_UP,
  EVE_GAZE_DOWN,
} EveGazeDirection;

void eveGazeInit(void);
void eveGazeTick(uint32_t nowMs, float dtSec);

/** Script a cardinal look; holdMs keeps target before easing home (0 = default). */
void eveGazeLook(EveGazeDirection dir, uint32_t holdMs);

/** Smooth track in normalized screen space 0..1. */
void eveGazeTrackNormalized(float nx, float ny);

/** Ease back to centre over ~durationMs. */
void eveGazeReturnCenter(uint32_t durationMs);

/** Map ToF zone to a gaze target. */
void eveGazeTrackZone(EveTargetModel zone);

/** True while scripted gaze is not yet settled at centre. */
bool eveGazeIsBusy(void);

/** When true, head-tracking may skip overwriting LVGL gaze. */
bool eveGazeScriptOwnsTarget(void);

/** Release scripted gaze so head pan / tracking can drive eyes. */
void eveGazeYield(void);
