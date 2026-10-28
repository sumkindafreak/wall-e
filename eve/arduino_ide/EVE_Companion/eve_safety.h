/**
 * EVE safety supervisor — latched STOP + fault register.
 *
 * Plain C++ (no Arduino calls) so it can be unit-tested on a PC.
 *
 * Rules
 *  - A STOP can come from WALL-E (MSG_STOP), the USB serial console or EVE's docked web page.
 *    It is accepted in EVERY state, even before the WALL-E handshake.
 *  - While stopped: no servo motion, no new sound, no routines. Motion is NOT resumed when the
 *    stop is cleared — something must ask for it again.
 *  - Only an explicit clear (MSG_CLEAR_STOP, serial "clearstop", web "clear stop") removes it.
 *    Reconnecting, MSG_RESET_STATE and mode commands never clear it.
 *  - Faults marked "blocking" put EVE in ERROR and block motion until the cause is gone.
 */
#pragma once

#include <stdint.h>

typedef enum : uint8_t {
  EVE_STOP_SRC_NONE = 0,
  EVE_STOP_SRC_WALLE = 1,       /* MSG_STOP over the hand UART */
  EVE_STOP_SRC_SERIAL = 2,      /* USB serial console */
  EVE_STOP_SRC_WEBUI = 3,       /* EVE's docked web page */
} EveStopSource;

/** Fault bits. Blocking faults are listed in EVE_FAULT_BLOCKING_MASK. */
typedef enum : uint16_t {
  EVE_FAULT_NONE = 0,
  EVE_FAULT_PIN_CONFLICT = 1u << 0,   /* two drivers configured on one GPIO (blocking) */
  EVE_FAULT_SERVO_DRIVER = 1u << 1,   /* servo PWM could not be attached (blocking) */
  EVE_FAULT_BATTERY_SENSOR = 1u << 2, /* battery sensor configured but not answering */
  EVE_FAULT_AUDIO = 1u << 3,          /* audio enabled but SD/I2S not ready */
  EVE_FAULT_EYES = 1u << 4,           /* eye display enabled but failed to start */
} EveFaultBits;

static const uint16_t EVE_FAULT_BLOCKING_MASK = EVE_FAULT_PIN_CONFLICT | EVE_FAULT_SERVO_DRIVER;

typedef struct {
  /** Stop every output NOW (cancel routines, halt servos, stop audio). Called on each stop latch. */
  void (*haltOutputs)(void);
  /** Optional log sink (one line, no newline). */
  void (*log)(const char* line);
} EveSafetyHooks;

void eveSafetyInit(const EveSafetyHooks* hooks);

/** Latch the stop. Returns true if it was not already latched. Always re-runs haltOutputs. */
bool eveSafetyLatchStop(EveStopSource src);
/** Clear a latched stop. Returns false if nothing was latched. */
bool eveSafetyClearStop(EveStopSource src);
bool eveSafetyStopLatched(void);
EveStopSource eveSafetyStopSource(void);
const char* eveSafetyStopSourceName(EveStopSource src);
/** Number of times a stop has been latched since boot (diagnostics). */
uint32_t eveSafetyStopCount(void);

void eveSafetySetFault(uint16_t bits, bool active);
uint16_t eveSafetyFaults(void);
bool eveSafetyHasBlockingFault(void);

/** Battery is verified critical: block motion until it recovers (does not latch). */
void eveSafetySetPowerInhibit(bool inhibit);
bool eveSafetyPowerInhibit(void);

/** True when servos may move: not stopped, no blocking fault, no power inhibit. */
bool eveSafetyMotionAllowed(void);
/** True when new sounds may start: not stopped and no blocking fault. */
bool eveSafetySoundAllowed(void);
/** True when expression routines may start: not stopped and no blocking fault. */
bool eveSafetyRoutinesAllowed(void);
