// ============================================================
//  CYD master — thin wrapper over shared walle_emotion_pose
// ============================================================
#pragma once

#include <stdint.h>
#include "walle_emotion_pose.h"
#include "protocol.h"

typedef WalleEmotionState EmotionState;
typedef WalleEmotionPose EmotionPose;
typedef WalleEmotionInputs EmotionInputSnapshot;

#define EMO_NEUTRAL WALLE_EMOTION_NEUTRAL
#define EMO_CURIOUS WALLE_EMOTION_CURIOUS
#define EMO_HAPPY WALLE_EMOTION_HAPPY
#define EMO_SAD WALLE_EMOTION_SAD
#define EMO_SCARED WALLE_EMOTION_SCARED
#define EMO_TIRED WALLE_EMOTION_TIRED

void emotionInit(void);

EmotionState emotionGetState(void);
const char* emotionGetName(void);

EmotionPose getEmotionPose(EmotionState state);

void emotionUpdateFromInputs(const EmotionInputSnapshot& in);

void emotionRefreshFromTelemetry(const TelemetryPacket* tp, bool telemValid);

void emotionApplyPoseToMotionEngine(void);
