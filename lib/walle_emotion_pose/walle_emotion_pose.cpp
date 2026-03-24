/**
 * WALL-E shared emotion pose — no servo output
 */
#include "walle_emotion_pose.h"

#include <Arduino.h>

static WalleEmotionState s_emotion = WALLE_EMOTION_NEUTRAL;
static uint32_t s_lastHumanSeenMs = 0;
static int8_t s_manualOverride = -1;

static const uint32_t HUMAN_IDLE_SAD_MS = 300000UL;

static const char* kNames[] = {
    "NEUTRAL", "CURIOUS", "HAPPY", "SAD", "SCARED", "TIRED",
};

void walleEmotionPoseInit(void) {
  s_emotion = WALLE_EMOTION_NEUTRAL;
  s_lastHumanSeenMs = 0;
  s_manualOverride = -1;
}

WalleEmotionState walleEmotionPoseGetState(void) {
  return s_emotion;
}

const char* walleEmotionPoseGetName(void) {
  return walleEmotionToString(s_emotion);
}

const char* walleEmotionToString(WalleEmotionState s) {
  uint8_t i = (uint8_t)s;
  if (i > (uint8_t)WALLE_EMOTION_TIRED) return "NEUTRAL";
  return kNames[i];
}

WalleEmotionPose walleEmotionPoseGetPose(WalleEmotionState state) {
  switch (state) {
    case WALLE_EMOTION_CURIOUS:
      return {300, 200, 0, 200};
    case WALLE_EMOTION_HAPPY:
      return {0, 150, 200, 0};
    case WALLE_EMOTION_SAD:
      return {-400, 0, -300, -500};
    case WALLE_EMOTION_SCARED:
      return {500, 600, 400, 500};
    case WALLE_EMOTION_TIRED:
      return {-300, -100, -400, -600};
    default:
    case WALLE_EMOTION_NEUTRAL:
      return {0, 0, 0, 0};
  }
}

void walleEmotionPoseSetManualOverride(int8_t emotionOrNeg1) {
  s_manualOverride = emotionOrNeg1;
}

void walleEmotionPoseUpdateFromInputs(const WalleEmotionInputs* in) {
  if (!in) return;

  if (s_manualOverride >= 0 && s_manualOverride <= (int8_t)WALLE_EMOTION_TIRED) {
    s_emotion = (WalleEmotionState)(uint8_t)s_manualOverride;
    return;
  }

  uint32_t now = millis();

  if (in->loudSound) {
    s_emotion = WALLE_EMOTION_SCARED;
    return;
  }
  if (!in->brainLinkOk || !in->visionOnline) {
    s_emotion = WALLE_EMOTION_SAD;
    return;
  }
  if (in->batteryPercent >= 0.0f && in->batteryPercent < 20.0f) {
    s_emotion = WALLE_EMOTION_TIRED;
    return;
  }
  if (in->isDocked) {
    s_emotion = WALLE_EMOTION_HAPPY;
    return;
  }
  if (in->humanDetected) {
    s_lastHumanSeenMs = now;
    s_emotion = WALLE_EMOTION_CURIOUS;
    return;
  }

  uint32_t msSince = (s_lastHumanSeenMs == 0) ? UINT32_MAX : (now - s_lastHumanSeenMs);
  if (msSince != UINT32_MAX && msSince >= HUMAN_IDLE_SAD_MS) {
    s_emotion = WALLE_EMOTION_SAD;
    return;
  }
  s_emotion = WALLE_EMOTION_NEUTRAL;
}

void walleEmotionPoseApplyToServosStub(void) {
  (void)walleEmotionPoseGetPose(s_emotion);
}
