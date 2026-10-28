/**
 * EVE expression routines — short, timed, cancellable sequences of eye expression, glow, sound
 * cue and (optionally) servo pose. Non-blocking: call eveRoutinesTick() from loop().
 *
 * Plain C++ so it can be unit-tested on a PC. All outputs go through hooks, which the Arduino glue
 * (state_machine.cpp) routes to the real eyes / NeoPixel / audio / servo modules. Those modules
 * enforce the safety latch themselves too, so a routine can never move a servo while stopped.
 *
 * "scan" is an ANIMATION: EVE looks left and right. It does not detect or recognise anything.
 */
#pragma once

#include <stdint.h>

typedef enum : uint8_t {
  EVE_ROUTINE_NONE = 0,
  EVE_ROUTINE_WAKE,       /* link to WALL-E established */
  EVE_ROUTINE_LOST,       /* link to WALL-E lost (expressions only, no motion) */
  EVE_ROUTINE_ESCORT,     /* decisive, on-task */
  EVE_ROUTINE_GREET,      /* softer second response / interaction */
  EVE_ROUTINE_STANDBY,    /* standby beacon */
  EVE_ROUTINE_LOW_POWER,  /* verified critical battery */
  EVE_ROUTINE_LOW_WARN,   /* verified low (warning) battery — one-off reaction */
  EVE_ROUTINE_FULL,       /* measured full charge */
  EVE_ROUTINE_SCAN,       /* operator: theatrical scan animation */
  EVE_ROUTINE_PLAY,       /* operator: playful burst */
  EVE_ROUTINE_PROTECT,    /* operator: protective attention pose */
  EVE_ROUTINE_COUNT
} EveRoutineId;

typedef struct {
  /** Eye expression shot (EveExpressionId value) held for holdMs. */
  void (*expression)(uint8_t exprId, uint32_t holdMs);
  /** Temporary glow overlay for ms (0 = until the routine ends). */
  void (*glow)(uint8_t pattern, uint32_t ms);
  /** Sound cue track number (never 0). */
  void (*sound)(uint8_t track);
  /** Ease a servo channel to deg at degPerSec. Returns false if refused (stopped, disabled...). */
  bool (*servo)(uint8_t channel, int16_t deg, uint16_t degPerSec);
  /** May a routine start / continue? (not stopped, no blocking fault) */
  bool (*routinesAllowed)(void);
  /** Optional log sink. */
  void (*log)(const char* line);
} EveRoutineHooks;

void eveRoutinesInit(const EveRoutineHooks* hooks);
/**
 * Start a routine. Returns false when routines are not allowed, the same routine is already
 * running, or a higher-priority routine is running.
 */
bool eveRoutineStart(EveRoutineId id, uint32_t nowMs);
/** Cancel the active routine (and anything that would follow). Servos stay where commanded. */
void eveRoutineCancel(void);
void eveRoutinesTick(uint32_t nowMs);

EveRoutineId eveRoutineActive(void);
const char* eveRoutineName(EveRoutineId id);
/** Name -> id for operator commands ("scan", "play", "protect", "greet", "wake"). NONE if unknown. */
EveRoutineId eveRoutineFromName(const char* name);
/** True for routines an operator may trigger with MSG_RUN_ROUTINE. */
bool eveRoutineOperatorAllowed(EveRoutineId id);
/** True if the routine is pure animation with no sensing behind it (scan). */
bool eveRoutineIsTheatrical(EveRoutineId id);
/** Number of routines started since boot (diagnostics / tests). */
uint32_t eveRoutineStartCount(EveRoutineId id);
