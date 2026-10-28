/**
 * IDs shared by the state machine, routines, web UI and output drivers (no Arduino dependency).
 */
#pragma once

#include <stdint.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Glow patterns (onboard RGB heartbeat LED + optional external NeoPixel strip)
// Numbers 0, 1, 3, 4, 5 keep the meaning they already had in neopixel_control.cpp.
// -----------------------------------------------------------------------------
typedef enum : uint8_t {
  EVE_GLOW_IDLE = 0,           /* soft blue breathing (legacy default) */
  EVE_GLOW_LINKED = 1,         /* blue breathing, a little brighter (legacy "1") */
  EVE_GLOW_WAITING = 2,        /* slow, dim blue pulse while waiting for WALL-E */
  EVE_GLOW_CRITICAL = 3,       /* red pulse (legacy "3") */
  EVE_GLOW_STANDBY_BEACON = 4, /* slow green breathing beacon (legacy "4") */
  EVE_GLOW_INTERACT = 5,       /* soft violet (legacy "5") */
  EVE_GLOW_ESCORT = 6,         /* steady cool white-blue: decisive / on task */
  EVE_GLOW_LOW_POWER = 7,      /* slow amber pulse */
  EVE_GLOW_CHARGING = 8,       /* green-cyan breathing (only when charging is actually measured) */
  EVE_GLOW_FULL = 9,           /* white flash, then soft white */
  EVE_GLOW_FAULT = 10,         /* red double-blink */
  EVE_GLOW_STOPPED = 11,       /* steady dim red: latched stop */
  EVE_GLOW_SCAN = 12,          /* blue sweep — theatrical scan animation */
  EVE_GLOW_PLAY = 13,          /* gentle blue sparkle */
  EVE_GLOW_PROTECT = 14,       /* steady bright blue */
  EVE_GLOW_OFF = 15,
  EVE_GLOW_COUNT = 16
} EveGlowPattern;

static inline const char* eveGlowName(uint8_t p) {
  static const char* const k[EVE_GLOW_COUNT] = {"idle",   "linked",   "waiting", "critical", "standby", "interact",
                                                 "escort", "low_power", "charging", "full",     "fault",   "stopped",
                                                 "scan",   "play",      "protect",  "off"};
  return p < EVE_GLOW_COUNT ? k[p] : "unknown";
}

/** Name -> pattern. Returns -1 when unknown. */
static inline int eveGlowFromName(const char* name) {
  if (!name) return -1;
  for (uint8_t i = 0; i < EVE_GLOW_COUNT; i++) {
    if (strcmp(name, eveGlowName(i)) == 0) return i;
  }
  return -1;
}

// -----------------------------------------------------------------------------
// Eye expression names for MSG_SET_EYES {"e":"happy"}.
// Same order as EveExpressionId in eve_expression_state.h (checked by static_assert there).
// -----------------------------------------------------------------------------
#define EVE_EXPR_NAME_COUNT 22
static inline const char* eveExprNameLower(uint8_t id) {
  static const char* const k[EVE_EXPR_NAME_COUNT] = {
      "neutral",  "soft",    "sleep",    "wake",      "look_left", "look_right", "look_up",   "look_down",
      "track",    "happy",   "curious",  "sad",       "angry",     "sleepy",     "thinking",  "surprised",
      "concerned", "confused", "affection", "alert",  "low_battery", "searching"};
  return id < EVE_EXPR_NAME_COUNT ? k[id] : "unknown";
}

/** Name -> expression id (exact lowercase match). Returns -1 when unknown. */
static inline int eveExprFromName(const char* name) {
  if (!name) return -1;
  for (uint8_t i = 0; i < EVE_EXPR_NAME_COUNT; i++) {
    if (strcmp(name, eveExprNameLower(i)) == 0) return i;
  }
  return -1;
}

// -----------------------------------------------------------------------------
// Output availability bits reported in EVE's heartbeat ("outs") and on the WALL-E page.
// A bit is set only when the driver is enabled in config.h AND its pins are configured.
// -----------------------------------------------------------------------------
enum : uint8_t {
  EVE_OUT_EYES = 0x01,
  EVE_OUT_SERVOS = 0x02,
  EVE_OUT_GLOW_STRIP = 0x04,
  EVE_OUT_AUDIO = 0x08,
  EVE_OUT_TOF = 0x10,
  EVE_OUT_PRESENCE = 0x20,
  EVE_OUT_ONBOARD_RGB = 0x40,
  EVE_OUT_BATTERY = 0x80,
};

// -----------------------------------------------------------------------------
// Servo channels — the mapping already used by servo_control / web UI / CYD:
//   EVE_SERVO_L_PIN = head pan, EVE_SERVO_R_PIN = right arm.
// -----------------------------------------------------------------------------
typedef enum : uint8_t {
  EVE_SERVO_HEAD_PAN = 0,
  EVE_SERVO_RIGHT_ARM = 1,
  EVE_SERVO_COUNT = 2
} EveServoChannel;
