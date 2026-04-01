// ============================================================
//  Joystick hub — dead-zone + smoothing helpers (physical ADS1115)
//  Existing logic remains in ads1115_input.cpp; this file adds
//  small utilities without renaming globals.
// ============================================================

#pragma once

#include <Arduino.h>

static inline float joystickHubExpo(float x, float expo) {
  float ax = fabsf(x);
  float e = expo;
  if (e < 0.0f) e = 0.0f;
  if (e > 1.0f) e = 1.0f;
  float shaped = ax * (1.0f - e) + (ax * ax * ax) * e;
  return (x < 0.0f) ? -shaped : shaped;
}
