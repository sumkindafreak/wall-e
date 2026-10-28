/**
 * EVE state machine — Arduino glue around eve_control_core (link, commands, modes),
 * eve_safety (latched stop, faults) and eve_routines (expressions).
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "eve_safety.h"

/** Call first in setup(): safety + routine hooks and the GPIO configuration check. */
void stateMachineEarlyInit(void);
/** Call once in setup() after the drivers. Calling it again later acts as a reset (stop stays latched). */
void stateMachineInit(void);
void stateMachineTick(void);
void stateMachineOnUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);

uint32_t stateMachineGetSessionId(void);
/** True only while EVE is currently in docked state. */
bool stateMachineIsDocked(void);
const char* stateMachineGetStateName(void);
void stateMachineSetDockMimic(bool enabled, bool fakeCharging);
bool stateMachineIsDockMimic(void);
/** True when UART companion JSON may be sent (handshake done, not waiting / sleep / error). */
bool stateMachineAllowsCompanionUart(void);
/** Label advertised by the UART peer during ACK, e.g. "wall_e" or "eve_dock_c3". */
const char* stateMachineGetPeerLabel(void);

// ---- Local operator commands (serial console, docked web UI) ----------------------------------
void stateMachineLocalStop(EveStopSource src);
/** Returns false when no stop was latched. */
bool stateMachineLocalClearStop(EveStopSource src);
/** Restart the WALL-E handshake. Never clears a latched stop. */
void stateMachineLocalReset(void);
/** Operator routine by name (scan, play, protect, greet, wake). whyNot gets the refusal reason. */
bool stateMachineLocalRoutine(const char* name, const char** whyNot);
bool stateMachineLocalStandby(bool on, const char** whyNot);
/** Full status JSON (mode, stop, faults, link, battery, outputs). */
size_t stateMachineStatusJson(char* out, size_t cap);
/** Print a one-screen diagnostic summary to Serial. */
void stateMachinePrintDiagnostics(void);
