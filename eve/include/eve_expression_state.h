/**
 * EVE face — maps robot/world inputs to a target expression + layout parameters.
 * No LVGL types here (safe to include from any module).
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  EVE_EXPR_NEUTRAL_IDLE = 0,
  EVE_EXPR_SOFT_IDLE,
  EVE_EXPR_SLEEP,
  EVE_EXPR_WAKE,
  EVE_EXPR_LOOK_LEFT,
  EVE_EXPR_LOOK_RIGHT,
  EVE_EXPR_LOOK_UP,
  EVE_EXPR_LOOK_DOWN,
  EVE_EXPR_TRACK_TARGET,
  EVE_EXPR_HAPPY,
  EVE_EXPR_CURIOUS,
  EVE_EXPR_CONCERNED,
  EVE_EXPR_CONFUSED,
  EVE_EXPR_AFFECTION,
  EVE_EXPR_ALERT,
  EVE_EXPR_LOW_BATTERY,
  EVE_EXPR_SEARCHING_FOR_WALLE,
} EveExpressionId;

/** Scalar layout targets consumed by the renderer / animator (normalized where noted). */
typedef struct {
  float eyeSep;     /* inter-eye spacing scale ~0.85–1.15 */
  float eyeScaleX;
  float eyeScaleY;
  float gazeX; /* -1..1 */
  float gazeY;
  float lid;       /* 0 open, 1 closed */
  float glowOpa;   /* 0–255 */
  float scanOpa;   /* 0–255 */
  float tiltDeg;   /* whole visor tilt */
  float squint;    /* 0–1 narrows eye height */
} EveEyeTarget;

void eveExpressionInit(void);
void eveExpressionTick(uint32_t nowMs);

EveExpressionId eveExpressionGetCurrent(void);
void eveExpressionGetTarget(EveEyeTarget* out);

/** Temporary override; returns to automatic after holdMs (0 = default ~2s). */
void eveExpressionRequest(EveExpressionId id, uint32_t holdMs);

void eveExpressionNotifyWallEConnected(void);
void eveExpressionNotifyWallEDisconnected(void);
void eveExpressionNotifyRecordFailure(void);
void eveExpressionNotifySharedVoicebox(bool on);
void eveExpressionSetTargetGaze(float nx, float ny);
void eveExpressionSetSearchingWallE(bool on);
void eveExpressionSetDockedCharging(bool docked, bool charging);
void eveExpressionSetTracking(bool on);

/** Legacy eyesSetMode: 0 neutral, 1 calm/social, 2 alert/confused */
void eveExpressionSetLegacyMode(uint8_t mode);

#if EVE_FACE_DEBUG_BENCH
void eveExpressionDebugSerialPoll(void);
const char* eveExpressionName(EveExpressionId id);
#endif
