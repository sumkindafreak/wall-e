/**
 * WALL-E shared: servo-agnostic emotional pose state (-1000..+1000 abstract axes).
 * Map to eye tilt / neck yaw / pitch / height when servo layer is ready.
 * Sync copies: lib/walle_emotion_pose/, main_wall_E_base/main/
 */
#pragma once

#include <stdint.h>

typedef enum WalleEmotionState : uint8_t {
  WALLE_EMOTION_NEUTRAL = 0,
  WALLE_EMOTION_CURIOUS,
  WALLE_EMOTION_HAPPY,
  WALLE_EMOTION_SAD,
  WALLE_EMOTION_SCARED,
  WALLE_EMOTION_TIRED,
} WalleEmotionState;

typedef struct {
  int16_t eyeTilt;
  int16_t neckRotate;
  int16_t neckLift;
  int16_t neckHeight;
} WalleEmotionPose;

/** Inputs for trigger logic (fill from telemetry / node registry). */
typedef struct {
  uint8_t brainLinkOk;   /**< 0/1 */
  uint8_t visionOnline; /**< 0/1 */
  float batteryPercent; /**< 0..100, or negative if unknown */
  uint8_t isDocked;     /**< 0/1 */
  uint8_t humanDetected;/**< 0/1 */
  uint8_t loudSound;    /**< 0/1 */
} WalleEmotionInputs;

void walleEmotionPoseInit(void);

WalleEmotionState walleEmotionPoseGetState(void);
const char* walleEmotionPoseGetName(void);
/** Alias for API docs / WebUI */
const char* walleEmotionToString(WalleEmotionState s);

WalleEmotionPose walleEmotionPoseGetPose(WalleEmotionState state);

void walleEmotionPoseUpdateFromInputs(const WalleEmotionInputs* in);

/** Manual override: pass WALLE_EMOTION_* or -1 to resume auto logic */
void walleEmotionPoseSetManualOverride(int8_t emotionOrNeg1);

/** Stub for future: map pose → motion / PCA9685 (TODO) */
void walleEmotionPoseApplyToServosStub(void);
