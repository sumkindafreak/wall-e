#include "walle_shared_behaviour.h"

#include "eve_uart_bridge.h"
#include "eve_target_assist.h"
#include "emotion_engine.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_lastRxTokMs = 0;
static uint32_t s_lastTxMs = 0;

static bool tokenOk(const char* s) {
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

void walleSharedBehaviourInit(void) {
  s_lastRxTokMs = 0;
  s_lastTxMs = 0;
}

bool walleSendCompanionToEve(const char* token) {
  if (!tokenOk(token)) {
    return false;
  }
  if ((uint32_t)(millis() - s_lastTxMs) < 350u) {
    return false;
  }
  char buf[80];
  snprintf(buf, sizeof(buf), "{\"m\":\"%s\"}", token);
  if (!eveUartBridgeSendCompanionToEve(buf)) {
    return false;
  }
  s_lastTxMs = millis();
  return true;
}

void sendStateUpdate(const char* state) {
  (void)walleSendCompanionToEve(state);
}

static void applyToken(const char* m, uint32_t nowMs) {
  if (!m || !m[0]) {
    return;
  }
  s_lastRxTokMs = nowMs;

  if (!strcmp(m, "EVE_LOOK_LEFT")) {
    eveTargetAssistCompanionNudge(-0.14f, 900u);
    return;
  }
  if (!strcmp(m, "EVE_LOOK_RIGHT")) {
    eveTargetAssistCompanionNudge(0.14f, 900u);
    return;
  }
  if (!strcmp(m, "EVE_LOOK_CENTER")) {
    eveTargetAssistCompanionNudge(0.0f, 400u);
    return;
  }
  if (!strcmp(m, "EVE_ALERT")) {
    eveTargetAssistCompanionNudge(0.0f, 200u);
    emotionTransitionTo(EMOTION_NERVOUS, 0.75f, 1200);
    return;
  }

  if (!strcmp(m, "STATE_ALERT")) {
    emotionTransitionTo(EMOTION_NERVOUS, 0.7f, 1100);
    return;
  }
  if (!strcmp(m, "STATE_HAPPY")) {
    emotionTransitionTo(EMOTION_EXCITED, 0.55f, 1000);
    return;
  }
  if (!strcmp(m, "STATE_CURIOUS")) {
    emotionTransitionTo(EMOTION_CURIOUS, 0.5f, 1000);
    return;
  }
  if (!strcmp(m, "STATE_IDLE")) {
    emotionTransitionTo(EMOTION_CALM, 0.35f, 800);
    return;
  }
  if (!strcmp(m, "STATE_ESCORT")) {
    emotionTransitionTo(EMOTION_CALM, 0.45f, 1500);
    return;
  }
  if (!strcmp(m, "STATE_CHARGING")) {
    emotionTransitionTo(EMOTION_CALM, 0.3f, 2000);
  }
}

void handleIncomingMessage(const char* msg) {
  applyToken(msg, millis());
}

void processIncomingState(const char* token) {
  handleIncomingMessage(token);
}

void walleSharedBehaviourOnEveJson(const char* jsonUtf8) {
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
  applyToken(m, millis());
}

void walleSharedBehaviourTick(uint32_t nowMs) {
  (void)nowMs;
}
