#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "eve_target_tracker.h"

/** Cross-character shared mood / mode (UART + local mirror). */
typedef enum {
  EVE_SHARED_STATE_IDLE = 0,
  EVE_SHARED_STATE_CURIOUS,
  EVE_SHARED_STATE_ALERT,
  EVE_SHARED_STATE_HAPPY,
  EVE_SHARED_STATE_ESCORT,
  EVE_SHARED_STATE_CHARGING,
  EVE_SHARED_STATE_COUNT
} EveSharedState;

/** Alias for documentation / parity with shared “character state” naming. */
typedef EveSharedState SharedState;

void eveSharedBehaviourInit(void);
void eveSharedBehaviourTick(uint32_t nowMs);

/** Modular loop hooks (non-blocking). */
static inline void updateSharedBehaviour(void) {
  eveSharedBehaviourTick(millis());
}

void processIncomingState(const char* token);
static inline void syncEmotions(void) {
  eveSharedBehaviourTick(millis());
}

/** Send token to WALL-E over UART (JSON `{"m":"..."}`); no-op if not linked. */
void sendStateUpdate(const char* state);
void handleIncomingMessage(const char* msg);

/** Raw JSON from MSG_WALLE_COMPANION frame (extracts "m"). */
void eveSharedBehaviourOnWallEJson(const char* jsonUtf8);

/** ToF acknowledgement path: occasional cue to partner (random + cooldown). */
void eveSharedBehaviourOnTofSpatialAck(EveTargetModel zone, uint32_t nowMs);

void eveSharedBehaviourNotifyLinkedIdle(void);
void eveSharedBehaviourNotifyLocalEscort(void);
void eveSharedBehaviourNotifyLocalIdle(void);
void eveSharedBehaviourNotifyLocalDock(void);

EveSharedState eveSharedBehaviourGetLocalState(void);
EveSharedState eveSharedBehaviourGetPartnerState(void);
