/**
 * WALL-E side of the EVE hand-UART link — plain C++ (ArduinoJson only), unit-tested on a PC
 * together with EVE's eve_control_core (tests/eve_host).
 *
 * Handshake:   EVE_HELLO  ->  WALL_E_ACK {"session":S,"peer":"wall_e","hb":250}
 *              EVE_READY {"session":S}  ->  linked
 * While linked: WALL-E sends WALL_E_HEARTBEAT {"session":S} every 250 ms so EVE can supervise
 *              the link in every mode; EVE's heartbeats (with her status) keep WALL-E's side alive.
 * Link health: only CRC/version-valid frames travelling EVE -> WALL-E with the current session
 *              count. Noise, malformed frames and WALL-E's own frames echoed back never do.
 *
 * "Linked" means the two boards are communicating over UART. It is not proof that EVE is
 * physically in WALL-E's hand; only EVE's presence input ("present") can report that.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  /** Frame and send one message; seq is chosen here so EVE's CMD_RESULT can be matched. */
  bool (*send)(uint8_t type, uint8_t seq, const char* json);
  void (*log)(const char* line);
  /** MSG_EVE_TARGET_AWARENESS payloads (eve_target_assist). Optional. */
  void (*targetAwareness)(const char* json, uint32_t nowMs);
  /** Source of fresh session ids (esp_random on the robot). Optional. */
  uint32_t (*random32)(void);
} EvePeerHooks;

typedef enum : uint8_t {
  EVE_PEER_WAITING = 0, /* no EVE handshake */
  EVE_PEER_HANDSHAKE,   /* ACK sent, waiting for READY */
  EVE_PEER_LINKED,
} EvePeerState;

/** EVE's status as last reported in her heartbeat. */
typedef struct {
  bool valid;              /* at least one heartbeat this link */
  uint32_t rxMs;           /* when it arrived */
  char st[14];             /* EVE mode, e.g. "IDLE" */
  char req[14];
  bool stop;
  char stopSrc[10];
  uint16_t faults;
  bool pwrInhibit;
  char routine[12];
  uint8_t outs;
  int8_t present;          /* -1 no presence input, 0 no, 1 yes */
  bool attachAsserted;
  bool batHw;
  bool batOk;
  float batV;
  float batA;
  int batPct;
  int8_t batChg;           /* -1 unknown */
  bool batFull;
  uint32_t uptimeMs;
  uint32_t heap;
  uint32_t rxBad;
  uint32_t cmdOk;
  uint32_t cmdRej;
  char lastRej[24];
} EvePeerEveStatus;

typedef struct {
  bool valid;
  uint8_t seq;
  uint8_t type;
  bool ok;
  char err[24];
  uint32_t rxMs;
} EvePeerResult;

typedef struct {
  uint32_t framesIn;
  uint32_t echoIgnored;
  uint32_t sessionRejected;
  uint32_t links;
  uint32_t losses;
  uint32_t lowBatteryEvents;
  uint32_t errorEvents;
  uint32_t resultsOk;
  uint32_t resultsRejected;
  char lastError[32];
} EvePeerStats;

void evePeerInit(const EvePeerHooks* hooks, uint32_t nowMs);
void evePeerTick(uint32_t nowMs);
void evePeerOnFrame(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq, uint32_t nowMs);

EvePeerState evePeerState(void);
bool evePeerLinked(void);
uint32_t evePeerSession(void);
/** EVE announced the newer protocol features (caps) in her HELLO. */
uint16_t evePeerEveCaps(void);
uint32_t evePeerLastRxAgeMs(uint32_t nowMs);
const EvePeerEveStatus* evePeerEveStatus(void);
const EvePeerResult* evePeerLastResult(void);
const EvePeerStats* evePeerStats(void);
const char* evePeerStateName(EvePeerState s);

// ---- Commands to EVE. All return false when nothing was sent. ----------------------------------
/** STOP is sent even without a link (EVE accepts it before the handshake). */
bool evePeerSendStop(const char* src);
bool evePeerSendClearStop(void);
/** msgType = MSG_MODE_ESCORT / _DOCK / _IDLE / _INTERACT / _STANDBY or MSG_ATTACH_CONFIRMED. */
bool evePeerSendMode(uint8_t msgType);
bool evePeerSendRoutine(const char* name);
bool evePeerSendEyes(const char* name, uint16_t holdMs);
bool evePeerSendGlow(const char* name);
bool evePeerSendMove(int headDeg, int armDeg);
bool evePeerSendSound(uint8_t track);
bool evePeerSendBusCommand(const char* json);
bool evePeerSendResetState(void);

/** Status JSON for /api/eve/status (legacy keys kept). Returns bytes written. */
size_t evePeerStatusJson(char* out, size_t cap, uint32_t nowMs, uint32_t crcErr, uint32_t versionErr,
                         uint32_t lengthErr);
