#pragma once

#include <Arduino.h>

/** Smoothstep-style ease for t in [0,1]. */
float eveEaseInOut(float t);

void eveMotionRealismInit(void);

/** Milliseconds since last servoTick call, clamped for stable integration. */
uint32_t eveMotionRealismServoDeltaMs(void);

/** Remote / WALL-E angle command: slight angle & timing variation. */
int16_t eveMotionRealismApplyRemoteHeadDeg(int16_t deg, uint32_t nowMs);

/** Autonomous head target (ToF / head tracker): no angle dither; still marks activity. */
void eveMotionRealismNotifyAutonomousHeadTarget(int16_t deg, uint32_t nowMs);

/**
 * Move current toward target using eased, millis-based stepping.
 * @param headChannel true = slightly snappier profile for head vs arm.
 */
int16_t eveMotionRealismStepToward(int16_t current, int16_t target, uint32_t dtMs, bool headChannel);

/** Right arm follows its target on a slower eased profile (overlapping motion vs head). */
int16_t eveMotionRealismStepArmToward(int16_t current, int16_t target, uint32_t dtMs);

/** Subtle idle drift on head desired angle when quiet and settled (non-blocking). */
void eveMotionRealismIdleTick(int16_t* headDesiredInOut, uint32_t nowMs, bool headSettled);
