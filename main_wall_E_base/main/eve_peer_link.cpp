/**
 * WALL-E side of the EVE hand-UART link — see eve_peer_link.h.
 */
#include "eve_peer_link.h"

#include <ArduinoJson.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../../firmware_common/include/eve_link_protocol.h"

namespace {

/* Mirrors the "caps" bits EVE sends in her HELLO (eve_control_core.cpp). */
const uint16_t CAP_HB_SUPERVISE = 0x0008;
/** Link window for older EVE firmware that does not announce caps (the original 12 s rule). */
const uint32_t kLegacyLinkWindowMs = 12000;
/** ACK sent but no READY within this time -> wait for the next HELLO. */
const uint32_t kHandshakeTimeoutMs = 2000;

EvePeerHooks H = {};
EvePeerState g_state = EVE_PEER_WAITING;
uint32_t g_session = 0;
uint32_t g_ackMs = 0;
uint32_t g_lastRxMs = 0;
uint32_t g_lastHbTxMs = 0;
uint16_t g_caps = 0;
uint8_t g_txSeq = 0;
uint32_t g_sessionCounter = 0;
uint8_t g_lastType = 0;
char g_lastPayload[320] = "";
EvePeerEveStatus g_eve;
EvePeerResult g_result;
EvePeerStats S;

StaticJsonDocument<1536> g_in;
StaticJsonDocument<384> g_cmd;
char g_cmdText[EVE_MAX_PAYLOAD + 1];

void peerLog(const char* fmt, ...) {
  if (!H.log) return;
  char b[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  H.log(b);
}

void copyStr(char* dst, size_t cap, const char* src) {
  if (!dst || !cap) return;
  strncpy(dst, src ? src : "", cap - 1);
  dst[cap - 1] = '\0';
}

/** Types WALL-E transmits: if one arrives here, TX is looped back to RX. */
bool isWallEToEve(uint8_t t) { return t == MSG_WALL_E_ACK || (t >= 0x10 && t <= 0x3F); }

/** Types EVE transmits. */
bool isEveToWallE(uint8_t t) {
  return t == MSG_EVE_HELLO || (t >= MSG_EVE_READY && t <= MSG_EVE_CMD_RESULT) || (t >= MSG_BUS_HELLO && t <= MSG_BUS_NACK);
}

uint32_t newSession(uint32_t now) {
  uint32_t s = H.random32 ? H.random32() : (now * 2654435761u) ^ (++g_sessionCounter * 40503u) ^ 0x5EED1234u;
  if (s == 0) s = 1;
  return s;
}

bool sendRaw(uint8_t type, const char* json) {
  if (!H.send) return false;
  return H.send(type, ++g_txSeq, json ? json : "");
}

/** Serialise g_cmd (adding the session when linked) and send it. */
bool sendCmd(uint8_t type) {
  if (g_session != 0 && g_state == EVE_PEER_LINKED) g_cmd["session"] = g_session;
  const size_t n = serializeJson(g_cmd, g_cmdText, sizeof(g_cmdText));
  if (n == 0 || n >= sizeof(g_cmdText)) return false;
  const bool ok = sendRaw(type, g_cmdText);
  peerLog("[EVE bridge] -> %s %s%s", eveLinkMsgName(type), g_cmdText, ok ? "" : " (send failed)");
  return ok;
}

void resetEveStatus() {
  memset(&g_eve, 0, sizeof(g_eve));
  g_eve.present = -1;
  g_eve.batChg = -1;
  copyStr(g_eve.st, sizeof(g_eve.st), "unknown");
}

void dropLink(const char* why, bool counted) {
  if (g_state == EVE_PEER_LINKED && counted) S.losses++;
  if (g_state != EVE_PEER_WAITING) peerLog("[EVE bridge] link down (%s)", why);
  g_state = EVE_PEER_WAITING;
  g_session = 0;
  resetEveStatus();
}

void parseHeartbeat(uint32_t now) {
  g_eve.valid = true;
  g_eve.rxMs = now;
  copyStr(g_eve.st, sizeof(g_eve.st), g_in["st"] | "legacy");
  copyStr(g_eve.req, sizeof(g_eve.req), g_in["req"] | "");
  g_eve.stop = g_in["stop"] | false;
  copyStr(g_eve.stopSrc, sizeof(g_eve.stopSrc), g_in["stop_src"] | "none");
  g_eve.faults = g_in["faults"] | 0;
  g_eve.pwrInhibit = g_in["pwr_inh"] | false;
  copyStr(g_eve.routine, sizeof(g_eve.routine), g_in["routine"] | "none");
  g_eve.outs = g_in["outs"] | 0;
  g_eve.present = g_in["present"] | -1;
  g_eve.attachAsserted = g_in["att_cmd"] | false;
  g_eve.batHw = g_in["bat_hw"] | false;
  g_eve.batOk = g_in["bat_ok"] | false;
  g_eve.batV = g_in["bat_v"] | 0.0f;
  g_eve.batA = g_in["bat_a"] | 0.0f;
  g_eve.batPct = g_in["bat_pct"] | 0;
  g_eve.batChg = g_in["bat_chg"] | -1;
  g_eve.batFull = g_in["bat_full"] | false;
  g_eve.uptimeMs = g_in["uptime_ms"] | 0u;
  g_eve.heap = g_in["heap"] | 0u;
  g_eve.rxBad = g_in["rx_bad"] | 0u;
  g_eve.cmdOk = g_in["cmd_ok"] | 0u;
  g_eve.cmdRej = g_in["cmd_rej"] | 0u;
  copyStr(g_eve.lastRej, sizeof(g_eve.lastRej), g_in["last_rej"] | "");
}

}  // namespace

// =============================================================================
void evePeerInit(const EvePeerHooks* hooks, uint32_t nowMs) {
  if (hooks) H = *hooks;
  memset(&S, 0, sizeof(S));
  memset(&g_result, 0, sizeof(g_result));
  resetEveStatus();
  g_state = EVE_PEER_WAITING;
  g_session = 0;
  g_caps = 0;
  g_lastRxMs = 0;
  g_lastHbTxMs = nowMs;
  g_lastType = 0;
  g_lastPayload[0] = '\0';
}

void evePeerTick(uint32_t now) {
  switch (g_state) {
    case EVE_PEER_HANDSHAKE:
      if ((uint32_t)(now - g_ackMs) > kHandshakeTimeoutMs) {
        peerLog("[EVE bridge] no EVE_READY after ACK - waiting for the next HELLO");
        dropLink("handshake timeout", false);
      }
      break;
    case EVE_PEER_LINKED: {
      const bool supervised = (g_caps & CAP_HB_SUPERVISE) != 0;
      const uint32_t limit = supervised ? EVE_LINK_TIMEOUT_MS : kLegacyLinkWindowMs;
      if ((uint32_t)(now - g_lastRxMs) > limit) {
        dropLink("no valid frame from EVE", true);
        break;
      }
      if ((uint32_t)(now - g_lastHbTxMs) >= EVE_LINK_HEARTBEAT_PERIOD_MS) {
        g_lastHbTxMs = now;
        char hb[40];
        snprintf(hb, sizeof(hb), "{\"session\":%lu}", (unsigned long)g_session);
        sendRaw(MSG_WALL_E_HEARTBEAT, hb);
      }
      break;
    }
    default:
      break;
  }
}

void evePeerOnFrame(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq, uint32_t now) {
  (void)seq;
  S.framesIn++;
  if (len > EVE_MAX_PAYLOAD || (len > 0 && !payload)) return;
  if (isWallEToEve(type)) {
    S.echoIgnored++;
    return; /* our own frames looped back: never a sign of EVE */
  }
  if (!isEveToWallE(type)) return;

  g_lastType = type;
  const size_t copyLen = len < sizeof(g_lastPayload) - 1 ? len : sizeof(g_lastPayload) - 1;
  if (copyLen) memcpy(g_lastPayload, payload, copyLen);
  g_lastPayload[copyLen] = '\0';

  g_in.clear();
  bool hasJson = false;
  if (len > 0) hasJson = !deserializeJson(g_in, (const char*)payload, len) && g_in.is<JsonObject>();

  // ---- Handshake -----------------------------------------------------------------------------
  if (type == MSG_EVE_HELLO) {
    g_caps = hasJson ? (uint16_t)(g_in["caps"] | 0) : 0;
    if (!(g_state == EVE_PEER_HANDSHAKE && (uint32_t)(now - g_ackMs) < kHandshakeTimeoutMs && g_session != 0)) {
      if (g_state == EVE_PEER_LINKED) {
        peerLog("[EVE bridge] EVE restarted the handshake (she lost the link or rebooted)");
        S.losses++;
      }
      g_session = newSession(now);
    }
    g_state = EVE_PEER_HANDSHAKE;
    g_ackMs = now;
    resetEveStatus();
    char ack[96];
    snprintf(ack, sizeof(ack), "{\"session\":%lu,\"peer\":\"wall_e\",\"hb\":%lu}", (unsigned long)g_session,
             (unsigned long)EVE_LINK_HEARTBEAT_PERIOD_MS);
    sendRaw(MSG_WALL_E_ACK, ack);
    peerLog("[EVE bridge] EVE_HELLO (caps=0x%04x) -> WALL_E_ACK session=%lu", (unsigned)g_caps, (unsigned long)g_session);
    return;
  }

  /* Frames naming a session must name ours. */
  bool hasSession = false;
  uint32_t frameSession = 0;
  if (hasJson) {
    JsonVariantConst sv = g_in["session"];
    if (sv.is<uint32_t>()) {
      hasSession = true;
      frameSession = sv.as<uint32_t>();
    }
  }

  if (type == MSG_EVE_READY) {
    if (g_state == EVE_PEER_WAITING || !hasSession || frameSession != g_session) {
      S.sessionRejected++;
      peerLog("[EVE bridge] EVE_READY for unknown session ignored");
      return;
    }
    if (g_state != EVE_PEER_LINKED) {
      g_state = EVE_PEER_LINKED;
      S.links++;
      g_lastHbTxMs = now - EVE_LINK_HEARTBEAT_PERIOD_MS; /* first WALL-E heartbeat right away */
      peerLog("[EVE bridge] linked with EVE, session=%lu (UART communication, not proof of hand contact)",
           (unsigned long)g_session);
    }
    g_lastRxMs = now;
    return;
  }

  if (g_state == EVE_PEER_WAITING && type == MSG_EVE_HEARTBEAT && hasSession && frameSession != 0) {
    /* WALL-E restarted while EVE kept her link: continue her session instead of forcing a
     * re-handshake. Nothing is resumed — the stop latch and modes live on EVE. */
    g_session = frameSession;
    g_state = EVE_PEER_LINKED;
    S.links++;
    g_lastHbTxMs = now - EVE_LINK_HEARTBEAT_PERIOD_MS;
    peerLog("[EVE bridge] adopted EVE's running session %lu", (unsigned long)g_session);
  }

  if (g_state != EVE_PEER_LINKED) return; /* nothing else counts before the handshake */
  if (hasSession && frameSession != g_session) {
    S.sessionRejected++;
    return;
  }
  g_lastRxMs = now;

  switch (type) {
    case MSG_EVE_HEARTBEAT:
      if (hasJson) parseHeartbeat(now);
      break;
    case MSG_EVE_LOW_BATTERY:
      S.lowBatteryEvents++;
      peerLog("[EVE bridge] EVE reports critical battery: %s", g_lastPayload);
      break;
    case MSG_EVE_ERROR:
      S.errorEvents++;
      copyStr(S.lastError, sizeof(S.lastError), hasJson ? (g_in["msg"] | "error") : "error");
      peerLog("[EVE bridge] EVE error: %s", S.lastError);
      break;
    case MSG_EVE_SLEEP:
      peerLog("[EVE bridge] EVE entered standby");
      break;
    case MSG_EVE_CMD_RESULT:
      if (hasJson) {
        g_result.valid = true;
        g_result.seq = g_in["seq"] | 0;
        g_result.type = g_in["type"] | 0;
        g_result.ok = g_in["ok"] | false;
        copyStr(g_result.err, sizeof(g_result.err), g_in["err"] | "");
        g_result.rxMs = now;
        if (g_result.ok) {
          S.resultsOk++;
        } else {
          S.resultsRejected++;
          peerLog("[EVE bridge] EVE refused %s: %s", eveLinkMsgName(g_result.type), g_result.err);
        }
      }
      break;
    case MSG_EVE_TARGET_AWARENESS:
      if (H.targetAwareness && len > 0) H.targetAwareness(g_lastPayload, now);
      break;
    default:
      break; /* companion / bus frames: signs of life only */
  }
}

EvePeerState evePeerState(void) { return g_state; }
bool evePeerLinked(void) { return g_state == EVE_PEER_LINKED; }
uint32_t evePeerSession(void) { return g_session; }
uint16_t evePeerEveCaps(void) { return g_caps; }
uint32_t evePeerLastRxAgeMs(uint32_t nowMs) { return g_lastRxMs ? (uint32_t)(nowMs - g_lastRxMs) : 0; }
const EvePeerEveStatus* evePeerEveStatus(void) { return &g_eve; }
const EvePeerResult* evePeerLastResult(void) { return &g_result; }
const EvePeerStats* evePeerStats(void) { return &S; }

const char* evePeerStateName(EvePeerState s) {
  switch (s) {
    case EVE_PEER_HANDSHAKE: return "handshake";
    case EVE_PEER_LINKED: return "linked";
    default: return "waiting";
  }
}

// ---- Commands ------------------------------------------------------------------------------------
bool evePeerSendStop(const char* src) {
  g_cmd.clear();
  g_cmd["src"] = src ? src : "wall_e";
  return sendCmd(MSG_STOP);
}

bool evePeerSendClearStop(void) {
  if (g_state != EVE_PEER_LINKED) return false; /* EVE requires the current session */
  g_cmd.clear();
  return sendCmd(MSG_CLEAR_STOP);
}

bool evePeerSendMode(uint8_t msgType) {
  if (g_state != EVE_PEER_LINKED) return false;
  switch (msgType) {
    case MSG_MODE_ESCORT:
    case MSG_MODE_DOCK:
    case MSG_MODE_IDLE:
    case MSG_MODE_INTERACT:
    case MSG_MODE_STANDBY:
    case MSG_ATTACH_CONFIRMED:
      break;
    default:
      return false;
  }
  g_cmd.clear();
  return sendCmd(msgType);
}

bool evePeerSendRoutine(const char* name) {
  if (g_state != EVE_PEER_LINKED || !name || !name[0]) return false;
  g_cmd.clear();
  g_cmd["r"] = name;
  return sendCmd(MSG_RUN_ROUTINE);
}

bool evePeerSendEyes(const char* name, uint16_t holdMs) {
  if (g_state != EVE_PEER_LINKED || !name || !name[0]) return false;
  g_cmd.clear();
  g_cmd["e"] = name;
  if (holdMs) g_cmd["ms"] = holdMs;
  return sendCmd(MSG_SET_EYES);
}

bool evePeerSendGlow(const char* name) {
  if (g_state != EVE_PEER_LINKED || !name || !name[0]) return false;
  g_cmd.clear();
  g_cmd["p"] = name;
  return sendCmd(MSG_SET_GLOW);
}

bool evePeerSendMove(int headDeg, int armDeg) {
  if (g_state != EVE_PEER_LINKED) return false;
  /* EVE clamps to her own calibrated limits and reports "clamped" in her CMD_RESULT. */
  if (headDeg < 0) headDeg = 0;
  if (headDeg > 180) headDeg = 180;
  if (armDeg < 0) armDeg = 0;
  if (armDeg > 180) armDeg = 180;
  g_cmd.clear();
  g_cmd["l"] = headDeg;
  g_cmd["r"] = armDeg;
  return sendCmd(MSG_MOVE_SERVO);
}

bool evePeerSendSound(uint8_t track) {
  if (g_state != EVE_PEER_LINKED) return false;
  g_cmd.clear();
  g_cmd["track"] = track;
  return sendCmd(MSG_PLAY_SOUND);
}

bool evePeerSendBusCommand(const char* json) {
  if (g_state != EVE_PEER_LINKED || !json) return false;
  const bool ok = sendRaw(MSG_BUS_COMMAND, json);
  peerLog("[EVE bridge] -> BUS_COMMAND %s", json);
  return ok;
}

bool evePeerSendResetState(void) {
  if (g_state != EVE_PEER_LINKED) return false;
  g_cmd.clear();
  return sendCmd(MSG_RESET_STATE);
}

// ---- Status JSON -----------------------------------------------------------------------------
size_t evePeerStatusJson(char* out, size_t cap, uint32_t now, uint32_t crcErr, uint32_t versionErr, uint32_t lengthErr) {
  if (!out || !cap) return 0;
  static StaticJsonDocument<3072> d;
  d.clear();
  const bool linked = g_state == EVE_PEER_LINKED;
  d["ok"] = true;
  d["enabled"] = true;
  /* link_ok keeps its old name; it now means "handshake complete and EVE still heard from". */
  d["link_ok"] = linked;
  d["link"] = evePeerStateName(g_state);
  d["link_kind"] = "uart";
  d["last_rx_age_ms"] = g_lastRxMs ? (uint32_t)(now - g_lastRxMs) : 0;
  d["frames_ok"] = S.framesIn;
  d["crc_errors"] = crcErr;
  d["version_errors"] = versionErr;
  d["length_errors"] = lengthErr;
  d["echo_ignored"] = S.echoIgnored;
  d["session_rejected"] = S.sessionRejected;
  d["links"] = S.links;
  d["losses"] = S.losses;
  d["last_type"] = eveLinkMsgName(g_lastType);
  char hex[8];
  snprintf(hex, sizeof(hex), "0x%02x", (unsigned)g_lastType);
  d["last_type_hex"] = hex;
  d["session"] = g_session;
  d["eve_caps"] = g_caps;
  d["supervision"] = (g_caps & CAP_HB_SUPERVISE) ? "heartbeat_2s" : "legacy_12s";
  if (g_lastPayload[0]) {
    d["payload"] = (const char*)g_lastPayload;
  } else {
    d["payload"] = nullptr;
  }
  /* Physical contact is only known when EVE has a presence input. */
  const char* contact = "unknown";
  if (linked && g_eve.valid && g_eve.present == 1) contact = "present";
  if (linked && g_eve.valid && g_eve.present == 0) contact = "absent";
  d["contact"] = contact;

  JsonObject e = d.createNestedObject("eve");
  e["status_valid"] = linked && g_eve.valid;
  if (linked && g_eve.valid) {
    e["age_ms"] = (uint32_t)(now - g_eve.rxMs);
    e["state"] = (const char*)g_eve.st;
    e["requested"] = (const char*)g_eve.req;
    e["stop"] = g_eve.stop;
    e["stop_src"] = (const char*)g_eve.stopSrc;
    e["faults"] = g_eve.faults;
    e["power_inhibit"] = g_eve.pwrInhibit;
    e["routine"] = (const char*)g_eve.routine;
    e["outputs"] = g_eve.outs;
    e["present"] = g_eve.present;
    e["attach_asserted"] = g_eve.attachAsserted;
    e["bat_hw"] = g_eve.batHw;
    e["bat_ok"] = g_eve.batOk;
    e["bat_v"] = g_eve.batV;
    e["bat_a"] = g_eve.batA;
    e["bat_pct"] = g_eve.batPct;
    e["bat_charging"] = g_eve.batChg;
    e["bat_full"] = g_eve.batFull;
    e["uptime_ms"] = g_eve.uptimeMs;
    e["heap"] = g_eve.heap;
    e["rx_bad"] = g_eve.rxBad;
    e["cmd_ok"] = g_eve.cmdOk;
    e["cmd_rejected"] = g_eve.cmdRej;
    e["last_reject"] = (const char*)g_eve.lastRej;
  }
  JsonObject r = d.createNestedObject("last_result");
  r["valid"] = g_result.valid;
  if (g_result.valid) {
    r["type"] = eveLinkMsgName(g_result.type);
    r["seq"] = g_result.seq;
    r["ok"] = g_result.ok;
    r["err"] = (const char*)g_result.err;
    r["age_ms"] = (uint32_t)(now - g_result.rxMs);
  }
  d["low_battery_events"] = S.lowBatteryEvents;
  d["error_events"] = S.errorEvents;
  d["last_error"] = (const char*)S.lastError;
  const size_t n = serializeJson(d, out, cap);
  if (n >= cap) out[cap - 1] = '\0';
  return n;
}
