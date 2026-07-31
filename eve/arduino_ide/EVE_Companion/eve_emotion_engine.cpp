#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_emotion_engine.h"
#include "eve_expression_state.h"
#include "eve_gaze_engine.h"
#include "eve_idle_engine.h"
#include "eve_eye_blink.h"
#include <string.h>

static EveEmotionState s_state = EVE_EMOTION_BOOT;
static EveEmotionState s_prevLogged = EVE_EMOTION_BOOT;
static uint32_t s_bootUntil = 0;
static uint32_t s_personSince = 0;
static uint32_t s_personLeftAt = 0;
static uint32_t s_lastZoneSeen = 0;
static EveTargetModel s_lastZone = EVE_TARGET_MODEL_NONE;
static bool s_voice = false;
static uint32_t s_voiceUntil = 0;
static uint32_t s_thinkingUntil = 0;
static bool s_sleeping = false;
static bool s_personActive = false;

static EveExpressionId exprForState(EveEmotionState st) {
  switch (st) {
    case EVE_EMOTION_BOOT:
      return EVE_EXPR_WAKE;
    case EVE_EMOTION_IDLE:
      return EVE_EXPR_NEUTRAL_IDLE;
    case EVE_EMOTION_CURIOUS:
      return EVE_EXPR_CURIOUS;
    case EVE_EMOTION_FOLLOW:
      return EVE_EXPR_TRACK_TARGET;
    case EVE_EMOTION_INTERACT:
      return EVE_EXPR_SOFT_IDLE;
    case EVE_EMOTION_HAPPY:
      return EVE_EXPR_HAPPY;
    case EVE_EMOTION_THINKING:
      return EVE_EXPR_THINKING;
    case EVE_EMOTION_SLEEP:
      return EVE_EXPR_SLEEP;
    default:
      return EVE_EXPR_NEUTRAL_IDLE;
  }
}

static void setState(EveEmotionState st, uint32_t nowMs) {
  if (s_state == st) {
    return;
  }
  s_state = st;
  eveIdleOnEmotionChange(st);
  eveExpressionSetOrchestrator(exprForState(st));
  if (st == EVE_EMOTION_SLEEP) {
    eveIdleSetEnabled(false);
    eveEyeBlinkSetSleepClosed(true);
    eveGazeReturnCenter(1200);
  } else if (st == EVE_EMOTION_BOOT) {
    eveIdleSetEnabled(false);
    eveEyeBlinkWakeOpen();
  } else {
    eveIdleSetEnabled(true);
    if (st == EVE_EMOTION_IDLE) {
      eveGazeReturnCenter(900);
    }
  }
  (void)nowMs;
}

const char* eveEmotionStateName(EveEmotionState st) {
  switch (st) {
    case EVE_EMOTION_BOOT:
      return "BOOT";
    case EVE_EMOTION_IDLE:
      return "IDLE";
    case EVE_EMOTION_CURIOUS:
      return "CURIOUS";
    case EVE_EMOTION_FOLLOW:
      return "FOLLOW";
    case EVE_EMOTION_INTERACT:
      return "INTERACT";
    case EVE_EMOTION_HAPPY:
      return "HAPPY";
    case EVE_EMOTION_THINKING:
      return "THINKING";
    case EVE_EMOTION_SLEEP:
      return "SLEEP";
    default:
      return "?";
  }
}

void eveEmotionInit(void) {
  s_state = EVE_EMOTION_BOOT;
  s_prevLogged = EVE_EMOTION_BOOT;
  s_bootUntil = millis() + 1400u;
  s_personSince = 0;
  s_personLeftAt = 0;
  s_lastZoneSeen = 0;
  s_lastZone = EVE_TARGET_MODEL_NONE;
  s_voice = false;
  s_voiceUntil = 0;
  s_thinkingUntil = 0;
  s_sleeping = false;
  eveGazeInit();
  eveIdleInit();
  eveExpressionSetOrchestrator(EVE_EXPR_WAKE);
  eveExpressionSetVoiceActive(false);
}

void eveEmotionNotifyBootComplete(void) {
  s_bootUntil = millis() + 1200u;
}

void eveEmotionNotifySleep(bool sleeping) {
  s_sleeping = sleeping;
  if (sleeping) {
    setState(EVE_EMOTION_SLEEP, millis());
  } else if (s_state == EVE_EMOTION_SLEEP) {
    setState(EVE_EMOTION_IDLE, millis());
    eveEyeBlinkSetSleepClosed(false);
    eveEyeBlinkWakeOpen();
  }
}

void eveEmotionNotifyVoice(bool playing, uint8_t track) {
  (void)track;
  s_voice = playing;
  eveExpressionSetVoiceActive(playing);
  if (playing) {
    s_voiceUntil = millis() + 8000u;
    eveEyeBlinkTriggerWiden(0.22f);
    if (s_state == EVE_EMOTION_IDLE || s_state == EVE_EMOTION_FOLLOW || s_state == EVE_EMOTION_CURIOUS) {
      setState(EVE_EMOTION_INTERACT, millis());
    }
    eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 1800);
    if ((rand() % 4) == 0) {
      eveEyeBlinkTriggerRandom();
    }
  } else {
    s_voiceUntil = 0;
  }
}

void eveEmotionRequestThinking(uint32_t holdMs) {
  s_thinkingUntil = millis() + (holdMs ? holdMs : 2200u);
  setState(EVE_EMOTION_THINKING, millis());
  eveGazeLook(EVE_GAZE_UP, holdMs ? holdMs : 1800u);
}

static bool personPresent(const EveTargetSnapshot* snap) {
  if (!snap) {
    return false;
  }
  if (snap->zone == EVE_TARGET_MODEL_NONE || snap->zone == EVE_TARGET_MODEL_UNCERTAIN) {
    return false;
  }
  return snap->confidencePct >= 20;
}

static bool personClose(const EveTargetSnapshot* snap) {
  if (!personPresent(snap)) {
    return false;
  }
  if (snap->distanceMm <= 0) {
    return snap->stableStrong;
  }
  return snap->distanceMm <= (int32_t)(EVE_TOF_NEAR_MM * 0.65f);
}

void eveEmotionOnTofSnapshot(const EveTargetSnapshot* snap, uint32_t nowMs) {
  if (s_sleeping || s_state == EVE_EMOTION_BOOT) {
    return;
  }

  if (personPresent(snap)) {
    s_personActive = true;
    s_personLeftAt = 0;
    s_lastZone = snap->zone;
    s_lastZoneSeen = nowMs;
    if (s_personSince == 0) {
      s_personSince = nowMs;
      if (s_state == EVE_EMOTION_IDLE || s_state == EVE_EMOTION_HAPPY) {
        setState(EVE_EMOTION_CURIOUS, nowMs);
        eveGazeTrackZone(snap->zone);
        if (personClose(snap)) {
          eveExpressionRequest(EVE_EXPR_SURPRISED, 600);
          eveEyeBlinkTriggerWiden(0.35f);
        }
      }
    } else {
      uint32_t dwell = nowMs - s_personSince;
      eveGazeTrackZone(snap->zone);
      if (personClose(snap) && (s_state == EVE_EMOTION_FOLLOW || s_state == EVE_EMOTION_IDLE)) {
        setState(EVE_EMOTION_CURIOUS, nowMs);
      } else if (s_voice && s_personActive) {
        setState(EVE_EMOTION_INTERACT, nowMs);
      } else if (dwell > 3200u && s_state != EVE_EMOTION_HAPPY && s_state != EVE_EMOTION_THINKING &&
                 s_state != EVE_EMOTION_INTERACT) {
        setState(EVE_EMOTION_HAPPY, nowMs);
      } else if (dwell > 900u && s_state == EVE_EMOTION_CURIOUS) {
        setState(EVE_EMOTION_FOLLOW, nowMs);
      }
    }
  } else {
    if (s_personSince != 0) {
      if (s_personLeftAt == 0) {
        s_personLeftAt = nowMs;
        eveGazeTrackZone(s_lastZone);
        eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 1600);
        Serial.println(F("[EVE_EMOTION] Person left — watching departure"));
      } else if (nowMs - s_personLeftAt > 2200u) {
        s_personSince = 0;
        s_personLeftAt = 0;
        s_personActive = false;
        if (s_state != EVE_EMOTION_SLEEP && !s_voice) {
          setState(EVE_EMOTION_IDLE, nowMs);
          eveGazeReturnCenter(900);
        }
      }
    }
  }
}

void eveEmotionTick(uint32_t nowMs) {
  if (s_state == EVE_EMOTION_BOOT && nowMs >= s_bootUntil) {
    setState(EVE_EMOTION_IDLE, nowMs);
    eveEyeBlinkWakeOpen();
  }

  if (s_thinkingUntil != 0 && nowMs >= s_thinkingUntil) {
    s_thinkingUntil = 0;
    if (s_state == EVE_EMOTION_THINKING) {
      setState(s_personActive ? (s_voice ? EVE_EMOTION_INTERACT : EVE_EMOTION_FOLLOW) : EVE_EMOTION_IDLE, nowMs);
    }
  }

  if (s_voice && s_voiceUntil != 0 && nowMs >= s_voiceUntil) {
    s_voice = false;
    eveExpressionSetVoiceActive(false);
  }

  if (s_voice && (rand() % 900) == 0) {
    eveEyeBlinkTriggerRandom();
  }

  if (s_state != s_prevLogged) {
    Serial.print(F("[EVE_EMOTION] State -> "));
    Serial.println(eveEmotionStateName(s_state));
    s_prevLogged = s_state;
  }

  eveExpressionSetOrchestrator(exprForState(s_state));
}

EveEmotionState eveEmotionGetState(void) {
  return s_state;
}

#else /* !EVE_ENABLE_EYES */

#include "eve_emotion_engine.h"

void eveEmotionInit(void) {}
void eveEmotionTick(uint32_t) {}
EveEmotionState eveEmotionGetState(void) {
  return EVE_EMOTION_IDLE;
}
const char* eveEmotionStateName(EveEmotionState) {
  return "-";
}
void eveEmotionNotifyBootComplete(void) {}
void eveEmotionNotifySleep(bool) {}
void eveEmotionNotifyVoice(bool, uint8_t) {}
void eveEmotionOnTofSnapshot(const EveTargetSnapshot*, uint32_t) {}
void eveEmotionRequestThinking(uint32_t) {}

#endif /* EVE_ENABLE_EYES */
