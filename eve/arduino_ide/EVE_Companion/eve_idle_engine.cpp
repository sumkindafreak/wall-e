#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_idle_engine.h"
#include "eve_gaze_engine.h"
#include "eve_eye_blink.h"
#include "eve_expression_state.h"
#include <stdlib.h>

typedef enum {
  IDLE_ACT_NONE = 0,
  IDLE_ACT_BLINK,
  IDLE_ACT_DOUBLE,
  IDLE_ACT_SLOW,
  IDLE_ACT_WINK,
  IDLE_ACT_GAZE_NUDGE,
  IDLE_ACT_LOOK_AROUND,
  IDLE_ACT_PAUSE,
  IDLE_ACT_THINKING,
  IDLE_ACT_SLEEPY,
} IdleAction;

static bool s_enabled = true;
static uint32_t s_nextActionMs = 0;
static EveEmotionState s_emotion = EVE_EMOTION_BOOT;
static uint8_t s_lookStep = 0;
static uint32_t s_lookStepAt = 0;

static uint32_t randSpan(uint32_t lo, uint32_t hi) {
  if (hi <= lo) {
    return lo;
  }
  return lo + (uint32_t)(rand() % (int)(hi - lo + 1));
}

static IdleAction pickAction(void) {
  int r = rand() % 100;
  if (s_emotion == EVE_EMOTION_SLEEP || s_emotion == EVE_EMOTION_THINKING) {
    if (r < 35) {
      return IDLE_ACT_SLOW;
    }
    if (r < 55) {
      return IDLE_ACT_SLEEPY;
    }
    if (r < 70) {
      return IDLE_ACT_PAUSE;
    }
    return IDLE_ACT_BLINK;
  }
  if (r < 22) {
    return IDLE_ACT_BLINK;
  }
  if (r < 32) {
    return IDLE_ACT_DOUBLE;
  }
  if (r < 40) {
    return IDLE_ACT_SLOW;
  }
  if (r < 46) {
    return IDLE_ACT_WINK;
  }
  if (r < 58) {
    return IDLE_ACT_GAZE_NUDGE;
  }
  if (r < 68) {
    return IDLE_ACT_LOOK_AROUND;
  }
  if (r < 78) {
    return IDLE_ACT_PAUSE;
  }
  if (r < 88) {
    return IDLE_ACT_THINKING;
  }
  return IDLE_ACT_SLEEPY;
}

static void scheduleNext(uint32_t nowMs) {
  uint32_t gap = randSpan(2800, 9200);
  if (s_emotion == EVE_EMOTION_FOLLOW || s_emotion == EVE_EMOTION_HAPPY || s_emotion == EVE_EMOTION_INTERACT) {
    gap = randSpan(4200, 11000);
  }
  s_nextActionMs = nowMs + gap;
}

void eveIdleInit(void) {
  s_enabled = true;
  s_emotion = EVE_EMOTION_BOOT;
  s_lookStep = 0;
  s_lookStepAt = 0;
  scheduleNext(millis() + randSpan(800, 2400));
}

void eveIdleSetEnabled(bool on) {
  s_enabled = on;
  if (on) {
    scheduleNext(millis() + randSpan(500, 1800));
  }
}

void eveIdleOnEmotionChange(EveEmotionState state) {
  s_emotion = state;
  s_lookStep = 0;
  scheduleNext(millis() + randSpan(1200, 3600));
}

static void runLookAroundStep(uint32_t nowMs) {
  if (s_lookStep == 0) {
    eveGazeLook(EVE_GAZE_LEFT, 700);
    s_lookStep = 1;
    s_lookStepAt = nowMs + 900;
    return;
  }
  if (s_lookStep == 1 && nowMs >= s_lookStepAt) {
    eveGazeLook(EVE_GAZE_RIGHT, 700);
    s_lookStep = 2;
    s_lookStepAt = nowMs + 900;
    return;
  }
  if (s_lookStep == 2 && nowMs >= s_lookStepAt) {
    eveGazeReturnCenter(800);
    s_lookStep = 0;
    scheduleNext(nowMs + randSpan(2000, 5000));
  }
}

void eveIdleTick(uint32_t nowMs) {
  if (!s_enabled || s_emotion == EVE_EMOTION_BOOT || s_emotion == EVE_EMOTION_SLEEP) {
    return;
  }
  if (s_lookStep != 0) {
    runLookAroundStep(nowMs);
    return;
  }
  if (nowMs < s_nextActionMs) {
    return;
  }

  IdleAction act = pickAction();
  switch (act) {
    case IDLE_ACT_BLINK:
      eveEyeBlinkTrigger((rand() & 1) ? EVE_EYE_SIDE_RIGHT : EVE_EYE_SIDE_LEFT);
      break;
    case IDLE_ACT_DOUBLE:
      eveEyeBlinkTriggerDouble((rand() & 1) ? EVE_EYE_SIDE_RIGHT : EVE_EYE_SIDE_LEFT);
      break;
    case IDLE_ACT_SLOW:
      eveEyeBlinkTriggerSlow((rand() & 1) ? EVE_EYE_SIDE_RIGHT : EVE_EYE_SIDE_LEFT);
      break;
    case IDLE_ACT_WINK:
      eveEyeBlinkTriggerWink((rand() & 1) ? EVE_EYE_SIDE_LEFT : EVE_EYE_SIDE_RIGHT);
      break;
    case IDLE_ACT_GAZE_NUDGE: {
      float dx = ((float)(rand() % 200) / 100.f - 1.f) * 0.06f;
      float dy = ((float)(rand() % 200) / 100.f - 1.f) * 0.05f;
      eveEyeBlinkNudgeGaze(dx, dy);
      if ((rand() % 3) == 0) {
        eveGazeLook((EveGazeDirection)(1 + rand() % 4), randSpan(500, 1100));
      }
      break;
    }
    case IDLE_ACT_LOOK_AROUND:
      s_lookStep = 1;
      runLookAroundStep(nowMs);
      return;
    case IDLE_ACT_PAUSE:
      break;
    case IDLE_ACT_THINKING:
      eveExpressionRequest(EVE_EXPR_THINKING, randSpan(1400, 2600));
      eveGazeLook(EVE_GAZE_UP, randSpan(600, 1200));
      eveEyeBlinkTriggerSquint(0.35f);
      break;
    case IDLE_ACT_SLEEPY:
      eveExpressionRequest(EVE_EXPR_SLEEPY, randSpan(1200, 2200));
      eveEyeBlinkTriggerSlow((rand() & 1) ? EVE_EYE_SIDE_RIGHT : EVE_EYE_SIDE_LEFT);
      eveEyeBlinkTriggerSquint(0.25f);
      break;
    default:
      break;
  }
  scheduleNext(nowMs);
}

#else /* !EVE_ENABLE_EYES */

#include "eve_idle_engine.h"

void eveIdleInit(void) {}
void eveIdleTick(uint32_t) {}
void eveIdleSetEnabled(bool) {}
void eveIdleOnEmotionChange(EveEmotionState) {}

#endif /* EVE_ENABLE_EYES */
