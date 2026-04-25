#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  SHARED_STATE_IDLE = 0,
  SHARED_STATE_CURIOUS,
  SHARED_STATE_ALERT,
  SHARED_STATE_HAPPY,
  SHARED_STATE_ESCORT,
  SHARED_STATE_CHARGING,
  SHARED_STATE_COUNT
} SharedState;

void walleSharedBehaviourInit(void);
void walleSharedBehaviourTick(uint32_t nowMs);

/** UART MSG_EVE_COMPANION (0x09) JSON payload from EVE. */
void walleSharedBehaviourOnEveJson(const char* jsonUtf8);

/** Send JSON `{"m":"token"}` to EVE (MSG_WALLE_COMPANION); no-op if bridge disabled / down. */
bool walleSendCompanionToEve(const char* token);

static inline void updateSharedBehaviour(void) {
  walleSharedBehaviourTick(millis());
}

void processIncomingState(const char* token);
void handleIncomingMessage(const char* msg);
void sendStateUpdate(const char* state);
static inline void syncEmotions(void) {
  walleSharedBehaviourTick(millis());
}
