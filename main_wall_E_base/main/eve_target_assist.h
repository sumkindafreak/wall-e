#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  ASSIST_NONE = 0,
  ASSIST_BIAS_LEFT,
  ASSIST_BIAS_RIGHT,
  ASSIST_ALIGN_CENTER,
  ASSIST_HOLD_TARGET,
  ASSIST_SUPPRESSED_BY_SAFETY,
  ASSIST_SUPPRESSED_BY_MANUAL,
} EveAssistState;

#define EVE_ASSIST_MASK_SAFETY 0x01u
#define EVE_ASSIST_MASK_DOCK 0x02u
#define EVE_ASSIST_MASK_MANUAL 0x04u

void eveTargetAssistInit(void);
void eveTargetAssistTick(uint32_t nowMs);
void eveTargetAssistSetSuppressMask(uint32_t mask);
void eveTargetAssistIngestJson(const char* jsonUtf8, uint32_t rxMillis);

EveAssistState eveTargetAssistGetState(void);
float eveTargetAssistGetTurnBias(void);
/** Small differential to add before motor ramp (-255..255 scale base command) */
void eveTargetAssistGetMotorDelta(int16_t* dLeft, int16_t* dRight);

/** Compact JSON for WebUI / living telemetry (zone, bias, stale). */
String eveTargetAssistGetStatusJSON(void);
