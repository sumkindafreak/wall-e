// ============================================================
//  CYD master — telemetry glue + shared pose engine
// ============================================================

#include "emotion_engine.h"
#include <Arduino.h>

void emotionInit(void) {
  walleEmotionPoseInit();
}

EmotionState emotionGetState(void) {
  return walleEmotionPoseGetState();
}

const char* emotionGetName(void) {
  return walleEmotionPoseGetName();
}

EmotionPose getEmotionPose(EmotionState state) {
  return walleEmotionPoseGetPose((WalleEmotionState)state);
}

void emotionUpdateFromInputs(const EmotionInputSnapshot& in) {
  walleEmotionPoseUpdateFromInputs(&in);
}

void emotionRefreshFromTelemetry(const TelemetryPacket* tp, bool telemValid) {
  WalleEmotionInputs s = {};
  s.brainLinkOk = telemValid ? 1u : 0u;
  s.visionOnline = telemValid ? 1u : 0u;
  s.loudSound = 0u;
  s.batteryPercent = -1.0f;

  if (telemValid && tp) {
    float v = tp->batteryVoltage;
    int pct = (int)((v - 3.0f) / 1.2f * 100.0f);
    pct = constrain(pct, 0, 100);
    s.batteryPercent = (float)pct;
    /* TODO: set when Base telemetry exposes dock/charge flags */
    s.isDocked = 0u;
    s.humanDetected = (tp->moodState == (uint8_t)MOOD_CURIOUS) ? 1u : 0u;
  } else {
    s.isDocked = 0u;
    s.humanDetected = 0u;
  }

  walleEmotionPoseUpdateFromInputs(&s);
}

void emotionApplyPoseToMotionEngine(void) {
  walleEmotionPoseApplyToServosStub();
}
