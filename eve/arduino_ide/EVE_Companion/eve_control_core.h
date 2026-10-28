/**
 * EVE control core — link supervision, command validation and mode arbitration.
 *
 * Plain C++ (ArduinoJson only, no Arduino calls): every time value comes in as nowMs and every
 * output goes out through EveControlHooks, so the same code runs on the ESP32-S3 and in the PC
 * tests under tests/eve_host. state_machine.cpp is the thin Arduino glue around it.
 *
 * WHAT "LINKED" MEANS
 *   A WALL_E_ACK with a session id completed the UART handshake. That proves the two boards are
 *   talking over UART. It does NOT prove physical hand contact; only the optional presence pin
 *   (EVE_PRESENT_PIN) can say that, and it is reported separately as "present".
 *
 * LINK HEALTH
 *   Only CRC- and version-valid frames reach this module (EveFrameDecoder). Of those, only frames
 *   travelling WALL-E -> EVE count as signs of life, and a frame carrying a different session id
 *   is rejected. Noise, malformed frames and EVE's own frames echoed back cannot keep the link up.
 *   - Peers that announce heartbeats in their ACK ("hb":N, this WALL-E firmware): supervised in
 *     EVERY linked mode, link lost after EVE_LINK_TIMEOUT_MS without a valid frame.
 *   - Older peers (no "hb"): the original rule — EVE_LINK_LOST_MS in ESCORT / INTERACT / DOCKED.
 *
 * MODE ARBITRATION (highest first)
 *   ERROR (blocking fault) > WAIT_ATTACH (not linked) > DOCKED (requested) > LOW_POWER (verified
 *   critical battery) > requested mode (IDLE / ATTACHED / ESCORT / INTERACT / SLEEP).
 *   A latched STOP is separate from the mode: EVE keeps reporting her mode, but no motion, sound or
 *   routine runs until an explicit CLEAR_STOP. Reconnecting or RESET_STATE never clears it.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "eve_safety.h"

typedef enum : uint8_t {
  EVE_MODE_BOOT = 0,
  EVE_MODE_WAIT_ATTACH,
  EVE_MODE_HANDSHAKE, /* kept for name compatibility; the handshake completes inside one frame */
  EVE_MODE_IDLE,
  EVE_MODE_ATTACHED,
  EVE_MODE_ESCORT,
  EVE_MODE_INTERACT,
  EVE_MODE_LOW_POWER,
  EVE_MODE_DOCKED,
  EVE_MODE_ERROR,
  EVE_MODE_SLEEP, /* standby */
  EVE_MODE_COUNT
} EveMode;

/** Trustworthy battery level after filtering + hysteresis (battery_monitor). */
typedef enum : uint8_t {
  EVE_BATLVL_UNKNOWN = 0, /* no sensor, or the reading is invalid */
  EVE_BATLVL_OK,
  EVE_BATLVL_WARN,
  EVE_BATLVL_CRITICAL,
} EveBatLevel;

/** Battery numbers copied into the heartbeat. */
typedef struct {
  bool hw;          /* sensor detected */
  bool valid;       /* latest reading plausible */
  float v;          /* pack volts */
  float a;          /* amps, sensor sign */
  int pct;          /* 0-100, only meaningful when valid */
  uint8_t st;       /* EveBatStatus value (legacy field) */
  int8_t charging;  /* -1 unknown, 0 no, 1 yes */
  bool full;        /* measured full (charging + taper) */
} EveBatteryReport;

typedef struct {
  /** Send one framed JSON message to the peer. */
  bool (*send)(uint8_t type, const char* json);
  /** Optional status sources for the heartbeat. */
  void (*battery)(EveBatteryReport* out);
  uint32_t (*freeHeap)(void);
  uint8_t (*outputs)(void);    /* EVE_OUT_* bits */
  int8_t (*present)(void);     /* presence pin: -1 not wired, 0 not present, 1 present */
  uint32_t (*rxErrors)(void);  /* CRC / version / length errors seen by the decoder */
  /** Servo access (channel = EveServoChannel). servoMove returns false if the driver refused. */
  bool (*servoAvailable)(uint8_t ch);
  void (*servoLimits)(uint8_t ch, int16_t* minDeg, int16_t* maxDeg);
  bool (*servoMove)(uint8_t ch, int16_t deg, uint16_t degPerSec);
  /**
   * Halt servo motion. stop=true: a STOP / fault — apply the configured stop policy
   * (EVE_SERVO_STOP_POLICY). stop=false: link loss — freeze at the last commanded position.
   */
  void (*haltMotion)(bool stop);
  /** Remote sound request (expression + audio). Returns true when audio actually started. */
  bool (*remoteSound)(uint8_t track);
  void (*stopSound)(void);
  void (*eyesExpression)(uint8_t exprId, uint32_t holdMs);
  void (*eyesLegacyMode)(uint8_t mode);
  /** Operator glow choice; -1 = automatic (follow the mode). */
  void (*glowOperator)(int8_t pattern);
  /** Shared-bus JSON command (desktop companion config). */
  bool (*busCommand)(const char* json);
  /** Notifications for the Arduino glue (eyes, glow, desktop companion...). */
  void (*onModeChanged)(EveMode from, EveMode to);
  void (*onLink)(bool up, bool timedOut);
  void (*log)(const char* line);
} EveControlHooks;

typedef struct {
  uint32_t framesIn;       /* valid frames received */
  uint32_t accepted;       /* commands accepted */
  uint32_t rejected;       /* commands refused (any reason) */
  uint32_t echoIgnored;    /* EVE->WALL-E frame types received (loopback / crossed wiring) */
  uint32_t sessionRejected;
  uint32_t linkUps;
  uint32_t linkLosses;
  char lastReject[24];     /* reason of the last refusal */
} EveControlStats;

void eveControlInit(const EveControlHooks* hooks, uint32_t nowMs);
void eveControlTick(uint32_t nowMs);
/** One CRC-valid frame from the UART decoder. */
void eveControlOnFrame(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq, uint32_t nowMs);

/** Battery input (call when the evaluated level / charge state changes, or every poll). */
void eveControlSetBattery(EveBatLevel level, int8_t charging, bool full, uint32_t nowMs);

// ---- Local operator commands (serial console / docked web UI) ----------------------------------
void eveControlLocalStop(EveStopSource src, uint32_t nowMs);
/** Explicit local clear. Returns false when nothing was latched. */
bool eveControlLocalClearStop(EveStopSource src, uint32_t nowMs);
/** Restart the handshake. Never clears a latched stop. */
void eveControlLocalReset(uint32_t nowMs);
/** Operator routine (scan / play / protect / greet / wake). Returns false with a reason. */
bool eveControlLocalRoutine(uint8_t routineId, uint32_t nowMs, const char** whyNot);
/** Enter (true) or leave (false) standby locally. */
bool eveControlLocalStandby(bool on, uint32_t nowMs, const char** whyNot);
/** Bench dock simulation (EVE_ENABLE_DOCK_MIMIC_TEST). */
void eveControlSetDockMimic(bool enabled, bool fakeCharging, uint32_t nowMs);

// ---- Status ------------------------------------------------------------------------------------
EveMode eveControlMode(void);
EveMode eveControlRequestedMode(void);
const char* eveControlModeName(EveMode m);
bool eveControlLinked(void);
uint32_t eveControlSession(void);
const char* eveControlPeer(void);
/** True when the peer announced heartbeats (supervised in every linked mode). */
bool eveControlPeerHeartbeats(void);
/** ms since the last valid WALL-E -> EVE frame of the current link (0 when not linked). */
uint32_t eveControlLinkAgeMs(uint32_t nowMs);
bool eveControlDockMimic(void);
bool eveControlDockMimicCharging(void);
/** WALL-E sent ATTACH_CONFIRMED during this link (an operator/WALL-E assertion, not a sensor). */
bool eveControlAttachAsserted(void);
EveBatLevel eveControlBatteryLevel(void);
const EveControlStats* eveControlStats(void);
/** True when companion JSON (shared behaviour, target relay...) may be sent. */
bool eveControlAllowsCompanionUart(void);
/** Build the heartbeat / status JSON into out (also used by the docked web UI). */
size_t eveControlStatusJson(char* out, size_t cap, uint32_t nowMs);
