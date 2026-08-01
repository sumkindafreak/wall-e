#include "eve_shared_behaviour.h"

#include "config.h"
#include "eve_feedback.h"
#include "eve_protocol.h"
#include "eyes_control.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "state_machine.h"
#include "uart_link.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static EveSharedState s_local = EVE_SHARED_STATE_IDLE;
static EveSharedState s_partner = EVE_SHARED_STATE_IDLE;
static uint32_t s_lastOutMs = 0;
static uint32_t s_lastInMs = 0;
static uint32_t s_lastInteractMs = 0;

/** Partner cue: head bias frames (spatial may override next frame unless we extend hold). */
static uint32_t s_partnerHeadHoldUntil = 0;
static int16_t s_partnerHeadDeg = 90;

typedef enum : uint8_t {
  EX_IDLE = 0,
  EX_PAUSE,
  EX_HEAD,
  EX_ARM_FOLLOW,
  EX_HOLD,
  EX_RELAX
} ExprPhase;

static ExprPhase s_ex = EX_IDLE;
static uint32_t s_exNextMs = 0;
static uint8_t s_exPri = 0;

static bool tokenCharsOk(const char* s) {
  if (!s || !s[0]) {
    return false;
  }
  for (size_t i = 0; s[i]; i++) {
    const unsigned char c = (unsigned char)s[i];
    if (i > 48) {
      return false;
    }
    if (!(isalnum(c) || c == '_')) {
      return false;
    }
  }
  return true;
}

static bool packJsonM(const char* token, char* out, size_t outSz) {
  if (!tokenCharsOk(token) || !out || outSz < 12) {
    return false;
  }
  snprintf(out, outSz, "{\"m\":\"%s\"}", token);
  return true;
}

static EveSharedState tokenToLocalState(const char* m) {
  if (!strcmp(m, "STATE_CURIOUS")) {
    return EVE_SHARED_STATE_CURIOUS;
  }
  if (!strcmp(m, "STATE_ALERT")) {
    return EVE_SHARED_STATE_ALERT;
  }
  if (!strcmp(m, "STATE_HAPPY")) {
    return EVE_SHARED_STATE_HAPPY;
  }
  if (!strcmp(m, "STATE_ESCORT")) {
    return EVE_SHARED_STATE_ESCORT;
  }
  if (!strcmp(m, "STATE_CHARGING")) {
    return EVE_SHARED_STATE_CHARGING;
  }
  return EVE_SHARED_STATE_IDLE;
}

static void applyPartnerToken(const char* m, uint32_t nowMs) {
  s_partner = tokenToLocalState(m);
  s_lastInMs = nowMs;
  s_lastInteractMs = nowMs;

  if (!strcmp(m, "WALLE_TURN_LEFT")) {
    s_partnerHeadDeg = 68;
    s_partnerHeadHoldUntil = nowMs + 900u;
    s_exPri = 3;
    s_ex = EX_HEAD;
    s_exNextMs = nowMs + 120u + (uint32_t)(rand() % 220u);
    return;
  }
  if (!strcmp(m, "WALLE_TURN_RIGHT")) {
    s_partnerHeadDeg = 118;
    s_partnerHeadHoldUntil = nowMs + 900u;
    s_exPri = 3;
    s_ex = EX_HEAD;
    s_exNextMs = nowMs + 120u + (uint32_t)(rand() % 220u);
    return;
  }
  if (!strcmp(m, "WALLE_AHEAD")) {
    s_partnerHeadDeg = 90;
    s_partnerHeadHoldUntil = nowMs + 700u;
    s_exPri = 2;
    s_ex = EX_HEAD;
    s_exNextMs = nowMs + 80u + (uint32_t)(rand() % 160u);
    return;
  }

  if (!strcmp(m, "STATE_ALERT")) {
    neopixelSetPattern(2);
    eyesSetMode(2);
    s_partnerHeadDeg = 84;
    s_partnerHeadHoldUntil = nowMs + 520u;
    s_exPri = 4;
    s_ex = EX_PAUSE;
    s_exNextMs = nowMs + 90u + (uint32_t)(rand() % 140u);
    return;
  }
  if (!strcmp(m, "STATE_HAPPY")) {
    neopixelSetPattern(4);
    eyesSetMode(1);
    s_exPri = 2;
    s_ex = EX_PAUSE;
    s_exNextMs = nowMs + 200u + (uint32_t)(rand() % 400u);
    return;
  }
  if (!strcmp(m, "STATE_CURIOUS")) {
    neopixelSetPattern(1);
    eyesSetMode(1);
    s_exPri = 2;
    s_ex = EX_PAUSE;
    s_exNextMs = nowMs + 260u + (uint32_t)(rand() % 500u);
    return;
  }
  if (!strcmp(m, "STATE_ESCORT")) {
    neopixelSetPattern(5);
    s_exPri = 3;
    s_ex = EX_PAUSE;
    s_exNextMs = nowMs + 150u + (uint32_t)(rand() % 300u);
    return;
  }
  if (!strcmp(m, "STATE_CHARGING")) {
    neopixelSetPattern(3);
    s_exPri = 1;
    s_ex = EX_RELAX;
    s_exNextMs = nowMs + 1u;
    return;
  }
  if (!strcmp(m, "STATE_IDLE")) {
    neopixelSetPattern(0);
    eyesSetMode(0);
    s_exPri = 1;
    s_ex = EX_RELAX;
    s_exNextMs = nowMs + 1u;
  }
}

void eveSharedBehaviourInit(void) {
  s_local = EVE_SHARED_STATE_IDLE;
  s_partner = EVE_SHARED_STATE_IDLE;
  s_lastOutMs = 0;
  s_lastInMs = 0;
  s_lastInteractMs = 0;
  s_partnerHeadHoldUntil = 0;
  s_partnerHeadDeg = 90;
  s_ex = EX_IDLE;
  s_exNextMs = 0;
  s_exPri = 0;
}

void sendStateUpdate(const char* state) {
  if (stateMachineGetSessionId() == 0 || !stateMachineAllowsCompanionUart()) {
    return;
  }
  if ((uint32_t)(millis() - s_lastOutMs) < 400u) {
    return;
  }
  char buf[80];
  if (!packJsonM(state, buf, sizeof(buf))) {
    return;
  }
  if (uartLinkSendJson(MSG_EVE_COMPANION, buf)) {
    s_lastOutMs = millis();
  }
}

void handleIncomingMessage(const char* msg) {
  if (!msg || !msg[0]) {
    return;
  }
  applyPartnerToken(msg, millis());
}

void processIncomingState(const char* token) {
  handleIncomingMessage(token);
}

void eveSharedBehaviourOnWallEJson(const char* jsonUtf8) {
  if (!jsonUtf8 || !jsonUtf8[0]) {
    return;
  }
  StaticJsonDocument<160> doc;
  if (deserializeJson(doc, jsonUtf8)) {
    return;
  }
  const char* m = doc["m"] | "";
  if (!m[0]) {
    return;
  }
  handleIncomingMessage(m);
}

void eveSharedBehaviourOnTofSpatialAck(EveTargetModel zone, uint32_t nowMs) {
  if (stateMachineGetSessionId() == 0 || !stateMachineAllowsCompanionUart()) {
    return;
  }
  if ((uint32_t)(nowMs - s_lastInteractMs) < 3000u) {
    return;
  }
  if ((rand() % 5) != 0) {
    return;
  }

  const char* tok = nullptr;
  if (zone == EVE_TARGET_MODEL_LEFT) {
    tok = "EVE_LOOK_LEFT";
  } else if (zone == EVE_TARGET_MODEL_RIGHT) {
    tok = "EVE_LOOK_RIGHT";
  } else if (zone == EVE_TARGET_MODEL_CENTER || zone == EVE_TARGET_MODEL_MULTI) {
    tok = "EVE_LOOK_CENTER";
  }
  if (!tok) {
    return;
  }

  char buf[80];
  if (!packJsonM(tok, buf, sizeof(buf))) {
    return;
  }
  if (uartLinkSendJson(MSG_EVE_COMPANION, buf)) {
    s_lastOutMs = nowMs;
    s_lastInteractMs = nowMs;
    showToast(tok);
  }
}

void eveSharedBehaviourNotifyLinkedIdle(void) {
  s_local = EVE_SHARED_STATE_IDLE;
  sendStateUpdate("STATE_IDLE");
}

void eveSharedBehaviourNotifyLocalEscort(void) {
  s_local = EVE_SHARED_STATE_ESCORT;
  sendStateUpdate("STATE_ESCORT");
}

void eveSharedBehaviourNotifyLocalIdle(void) {
  s_local = EVE_SHARED_STATE_IDLE;
  sendStateUpdate("STATE_IDLE");
}

void eveSharedBehaviourNotifyLocalDock(void) {
  s_local = EVE_SHARED_STATE_CHARGING;
  sendStateUpdate("STATE_CHARGING");
}

EveSharedState eveSharedBehaviourGetLocalState(void) {
  return s_local;
}

EveSharedState eveSharedBehaviourGetPartnerState(void) {
  return s_partner;
}

void eveSharedBehaviourTick(uint32_t nowMs) {
  if (nowMs < s_exNextMs) {
    return;
  }

  if (s_partnerHeadHoldUntil != 0u && nowMs < s_partnerHeadHoldUntil) {
    servoSetHeadPanTarget(s_partnerHeadDeg);
  } else if (s_partnerHeadHoldUntil != 0u && nowMs >= s_partnerHeadHoldUntil) {
    s_partnerHeadHoldUntil = 0;
  }

  switch (s_ex) {
    case EX_IDLE:
      break;
    case EX_PAUSE:
      s_ex = EX_HEAD;
      s_exNextMs = nowMs + 40u;
      break;
    case EX_HEAD:
      if (s_exPri >= 3) {
        servoSetHeadPanTarget(s_partnerHeadDeg);
      }
      s_ex = EX_ARM_FOLLOW;
      s_exNextMs = nowMs + 140u + (uint32_t)(rand() % 120u);
      break;
    case EX_ARM_FOLLOW: {
      int16_t arm = 90;
      if (s_partner == EVE_SHARED_STATE_ALERT || s_exPri >= 4) {
        arm = 38;
      } else if (s_partner == EVE_SHARED_STATE_HAPPY) {
        arm = 118;
      } else if (s_partner == EVE_SHARED_STATE_CURIOUS) {
        arm = 102;
      } else {
        arm = 95 + (int16_t)(rand() % 7);
      }
      servoSetRightArmTarget(arm);
      s_ex = EX_HOLD;
      s_exNextMs = nowMs + 420u + (uint32_t)(rand() % 380u);
      break;
    }
    case EX_HOLD:
      s_ex = EX_RELAX;
      s_exNextMs = nowMs + 180u + (uint32_t)(rand() % 200u);
      break;
    case EX_RELAX:
      if (s_exPri <= 1) {
        servoSetRightArmTarget(88 + (int16_t)(rand() % 5));
      } else {
        servoSetRightArmTarget(90);
      }
      s_ex = EX_IDLE;
      s_exPri = 0;
      s_exNextMs = nowMs + 1u;
      break;
    default:
      s_ex = EX_IDLE;
      break;
  }

#if EVE_ENABLE_SERVOS
  if (s_ex == EX_IDLE && s_partnerHeadHoldUntil == 0 && (rand() % 200) == 0 &&
      (uint32_t)(nowMs - s_lastInteractMs) > 5000u) {
    if ((uint32_t)(nowMs - s_lastInteractMs) > 8000u) {
      servoSetRightArmTarget((int16_t)(88 + (rand() % 9)));
    }
  }
#else
  (void)nowMs;
#endif
}
