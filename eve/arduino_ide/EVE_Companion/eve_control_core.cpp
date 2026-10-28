/**
 * EVE control core — see eve_control_core.h for the rules this file implements.
 */
#include "eve_control_core.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "eve_output_ids.h"
#include "eve_protocol.h"
#include "eve_routines.h"

namespace {

// HELLO "caps" bits — lets WALL-E know which newer features this EVE firmware understands.
enum : uint16_t {
  CAP_CMD_RESULT = 0x0001,   /* replies MSG_EVE_CMD_RESULT to every command */
  CAP_CLEAR_STOP = 0x0002,   /* latched stop + MSG_CLEAR_STOP */
  CAP_ROUTINES = 0x0004,     /* MSG_RUN_ROUTINE */
  CAP_HB_SUPERVISE = 0x0008, /* supervises the link with WALL-E heartbeats */
  CAP_MODES_V2 = 0x0010,     /* MSG_MODE_INTERACT / MSG_MODE_STANDBY */
};

const uint32_t kMimicSession = 0xE0E0E0E0u;

EveControlHooks H = {};
EveControlStats S = {};

EveMode g_mode = EVE_MODE_BOOT;
EveMode g_req = EVE_MODE_IDLE;
bool g_linked = false;
uint32_t g_session = 0;
char g_peer[24] = "none";
bool g_peerHb = false;
uint32_t g_lastPeerRxMs = 0;
uint32_t g_lastHelloMs = 0;
bool g_helloDue = true;
uint32_t g_lastHbTxMs = 0;
uint32_t g_lastCmdMs = 0;
uint32_t g_bootMs = 0;
bool g_attach = false;
bool g_mimic = false;
bool g_mimicChg = false;
EveBatLevel g_bat = EVE_BATLVL_UNKNOWN;
int8_t g_batChg = -1;
bool g_batFull = false;
bool g_warnReacted = false;
bool g_lowPowerReacted = false;
uint32_t g_lastLowBatTxMs = 0;
bool g_lostByTimeout = false;
bool g_rejectLogged = false;
uint32_t g_lastRejectLogMs = 0;
uint32_t g_rejectSuppressed = 0;
bool g_infoLogged = false;
uint32_t g_lastInfoLogMs = 0;

/* Input document (parsed command) and output document (what EVE sends) are kept separate so a
 * reply can be built while the command's strings are still being read. */
StaticJsonDocument<1536> g_in;
StaticJsonDocument<1280> g_outDoc;
char g_text[EVE_MAX_PAYLOAD + 1];
char g_out[EVE_MAX_PAYLOAD + 1];

const char* const kModeNames[EVE_MODE_COUNT] = {"BOOT",     "WAIT_ATTACH", "HANDSHAKE", "IDLE",  "ATTACHED", "ESCORT",
                                                "INTERACT", "LOW_POWER",   "DOCKED",    "ERROR", "SLEEP"};

// -----------------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------------
void coreLog(const char* fmt, ...) {
  if (!H.log) return;
  char b[176];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  H.log(b);
}

/** Rate-limited info log (loopback warnings, unknown frames). */
void infoRateLog(uint32_t now, const char* fmt, ...) {
  if (g_infoLogged && (uint32_t)(now - g_lastInfoLogMs) < EVE_LOG_RATE_MS) return;
  g_infoLogged = true;
  g_lastInfoLogMs = now;
  if (!H.log) return;
  char b[176];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  H.log(b);
}

float round2(float v) { return (float)lroundf(v * 100.0f) / 100.0f; }

void copyStr(char* dst, size_t cap, const char* src) {
  if (!dst || cap == 0) return;
  strncpy(dst, src ? src : "", cap - 1);
  dst[cap - 1] = '\0';
}

/** Peer label from the ACK: keep [A-Za-z0-9_-], max 23 chars, default "wall_e". */
void sanitizePeer(const char* in, char* out, size_t cap) {
  size_t n = 0;
  if (in) {
    for (size_t i = 0; in[i] && n + 1 < cap; i++) {
      const unsigned char c = (unsigned char)in[i];
      out[n++] = (isalnum(c) || c == '_' || c == '-') ? (char)c : '_';
    }
  }
  out[n] = '\0';
  if (n == 0) copyStr(out, cap, "wall_e");
}

/** Frame types EVE itself transmits. Receiving one means loopback / crossed wiring. */
bool isEveToPeer(uint8_t t) {
  return t == MSG_EVE_HELLO || (t >= MSG_EVE_READY && t <= 0x0F) || t == MSG_BUS_EVENT;
}

/** Frame types that travel WALL-E (or the dock) -> EVE and may count as signs of life. */
bool isPeerToEve(uint8_t t) {
  return t == MSG_WALL_E_ACK || (t >= 0x10 && t <= 0x3F) || (t >= MSG_BUS_HELLO && t <= MSG_BUS_NACK && t != MSG_BUS_EVENT);
}

/** Commands that get a MSG_EVE_CMD_RESULT reply. */
bool isCoreCommand(uint8_t t) { return t >= 0x10 && t <= 0x3F; }

bool motionMode(EveMode m) {
  return m == EVE_MODE_IDLE || m == EVE_MODE_ATTACHED || m == EVE_MODE_ESCORT || m == EVE_MODE_INTERACT ||
         m == EVE_MODE_DOCKED;
}

JsonObject beginOut() {
  g_outDoc.clear();
  return g_outDoc.to<JsonObject>();
}

bool sendOut(uint8_t type) {
  if (!H.send) return false;
  if (g_outDoc.overflowed()) coreLog("[EVE][LINK] %s JSON document overflowed - fields missing", eveLinkMsgName(type));
  const size_t n = measureJson(g_outDoc);
  if (n > EVE_MAX_PAYLOAD) {
    coreLog("[EVE][LINK] %s payload too large (%u bytes) - not sent", eveLinkMsgName(type), (unsigned)n);
    return false;
  }
  serializeJson(g_outDoc, g_out, sizeof(g_out));
  return H.send(type, g_out);
}

// -----------------------------------------------------------------------------
// Status JSON (heartbeat + docked web UI)
// -----------------------------------------------------------------------------
void buildStatus(JsonObject o, uint32_t now, bool forWeb) {
  o["session"] = g_session;
  o["uptime_ms"] = (uint32_t)(now - g_bootMs);
  if (H.freeHeap) o["heap"] = H.freeHeap();
  EveBatteryReport b;
  memset(&b, 0, sizeof(b));
  b.charging = -1;
  b.st = 3; /* EVE_BAT_UNKNOWN */
  if (H.battery) H.battery(&b);
  /* Legacy fields first (WALL-E and the dock already read these). */
  o["bat_ok"] = b.valid;
  o["bat_v"] = round2(b.v);
  o["bat_a"] = round2(b.a);
  o["bat_pct"] = b.valid ? b.pct : 0;
  o["bat_st"] = b.st;
  o["bat_hw"] = b.hw;
  o["bat_chg"] = b.charging;
  o["bat_full"] = b.full;
  o["st"] = kModeNames[g_mode];
  o["req"] = kModeNames[g_req];
  o["stop"] = eveSafetyStopLatched();
  o["stop_src"] = eveSafetyStopSourceName(eveSafetyStopSource());
  o["faults"] = eveSafetyFaults();
  o["pwr_inh"] = eveSafetyPowerInhibit();
  o["routine"] = eveRoutineName(eveRoutineActive());
  o["outs"] = H.outputs ? H.outputs() : 0;
  o["present"] = H.present ? H.present() : -1;
  o["att_cmd"] = g_attach;
  o["rx_bad"] = H.rxErrors ? H.rxErrors() : 0;
  o["cmd_ok"] = S.accepted;
  o["cmd_rej"] = S.rejected;
  o["last_rej"] = S.lastReject;
  if (forWeb) {
    o["link"] = g_linked;
    o["peer"] = g_peer;
    o["peer_hb"] = g_peerHb;
    o["link_age_ms"] = g_linked ? (uint32_t)(now - g_lastPeerRxMs) : 0;
    o["mimic"] = g_mimic;
    o["echo"] = S.echoIgnored;
    o["link_ups"] = S.linkUps;
    o["link_losses"] = S.linkLosses;
  }
}

void sendHello(uint32_t now) {
  JsonObject o = beginOut();
  o["proto"] = 1;
  o["eve"] = "eve-s3";
  o["caps"] = (uint16_t)(CAP_CMD_RESULT | CAP_CLEAR_STOP | CAP_ROUTINES | CAP_HB_SUPERVISE | CAP_MODES_V2);
  o["uptime_ms"] = (uint32_t)(now - g_bootMs);
  o["outs"] = H.outputs ? H.outputs() : 0;
  o["stop"] = eveSafetyStopLatched();
  sendOut(MSG_EVE_HELLO);
#if EVE_SERIAL_VERBOSE_LOGS
  coreLog("[EVE][LINK] -> EVE_HELLO");
#endif
}

void sendReady() {
  JsonObject o = beginOut();
  o["session"] = g_session;
  o["stop"] = eveSafetyStopLatched();
  sendOut(MSG_EVE_READY);
}

void sendHeartbeat(uint32_t now) {
  buildStatus(beginOut(), now, false);
  sendOut(MSG_EVE_HEARTBEAT);
}

void sendLowBattery(uint32_t now) {
  EveBatteryReport b;
  memset(&b, 0, sizeof(b));
  if (H.battery) H.battery(&b);
  JsonObject o = beginOut();
  o["v"] = round2(b.v);
  o["a"] = round2(b.a);
  o["pct"] = b.pct;
  o["session"] = g_session;
  sendOut(MSG_EVE_LOW_BATTERY);
  g_lastLowBatTxMs = now;
  coreLog("[EVE][BAT] -> EVE_LOW_BATTERY %.2fV %d%%", (double)b.v, b.pct);
}

// -----------------------------------------------------------------------------
// Command results
// -----------------------------------------------------------------------------
JsonObject beginResult(uint8_t seq, uint8_t type, bool ok, const char* err) {
  JsonObject r = beginOut();
  r["seq"] = seq;
  r["type"] = type;
  r["ok"] = ok;
  if (err) r["err"] = err;
  return r;
}

void finishResult(JsonObject r) {
  r["st"] = kModeNames[g_mode];
  r["stop"] = eveSafetyStopLatched();
  sendOut(MSG_EVE_CMD_RESULT);
}

void reject(uint8_t seq, uint8_t type, const char* why, uint32_t now) {
  S.rejected++;
  copyStr(S.lastReject, sizeof(S.lastReject), why);
  if (!g_rejectLogged || (uint32_t)(now - g_lastRejectLogMs) >= EVE_LOG_RATE_MS) {
    if (g_rejectSuppressed) {
      coreLog("[EVE][CMD] %s rejected: %s (+%lu more since last log)", eveLinkMsgName(type), why,
           (unsigned long)g_rejectSuppressed);
    } else {
      coreLog("[EVE][CMD] %s rejected: %s", eveLinkMsgName(type), why);
    }
    g_rejectLogged = true;
    g_lastRejectLogMs = now;
    g_rejectSuppressed = 0;
  } else {
    g_rejectSuppressed++;
  }
  if (isCoreCommand(type)) finishResult(beginResult(seq, type, false, why));
}

JsonObject accept(uint8_t seq, uint8_t type) {
  S.accepted++;
  return beginResult(seq, type, true, nullptr);
}

// -----------------------------------------------------------------------------
// Mode arbitration
// -----------------------------------------------------------------------------
EveMode computeMode() {
  if (eveSafetyHasBlockingFault()) return EVE_MODE_ERROR;
  if (!g_linked) return EVE_MODE_WAIT_ATTACH;
  if (g_req == EVE_MODE_DOCKED) return EVE_MODE_DOCKED;
  if (eveSafetyPowerInhibit()) return EVE_MODE_LOW_POWER;
  return g_req;
}

void startRoutine(EveRoutineId id, uint32_t now) {
  if (!eveRoutineStart(id, now)) {
#if EVE_SERIAL_VERBOSE_LOGS
    coreLog("[EVE][ROUTINE] %s not started (stopped / busy)", eveRoutineName(id));
#endif
  }
}

void onEnter(EveMode from, EveMode to, uint32_t now) {
  /* Leaving low power ends the dim "low power" reaction straight away. */
  if (from == EVE_MODE_LOW_POWER && eveRoutineActive() == EVE_ROUTINE_LOW_POWER) eveRoutineCancel();
  switch (to) {
    case EVE_MODE_WAIT_ATTACH:
      if (g_lostByTimeout) startRoutine(EVE_ROUTINE_LOST, now);
      g_lostByTimeout = false;
      break;
    case EVE_MODE_IDLE:
    case EVE_MODE_ATTACHED:
      if (from == EVE_MODE_WAIT_ATTACH || from == EVE_MODE_BOOT || from == EVE_MODE_HANDSHAKE ||
          from == EVE_MODE_SLEEP) {
        /* Link established (or waking from standby): the warmer, once-per-event wake response.
         * A still-running "lost" reaction must not block it. */
        if (eveRoutineActive() == EVE_ROUTINE_LOST) eveRoutineCancel();
        startRoutine(EVE_ROUTINE_WAKE, now);
      }
      break;
    case EVE_MODE_ESCORT:
      startRoutine(EVE_ROUTINE_ESCORT, now);
      break;
    case EVE_MODE_INTERACT:
      startRoutine(EVE_ROUTINE_GREET, now);
      break;
    case EVE_MODE_SLEEP: {
      startRoutine(EVE_ROUTINE_STANDBY, now);
      JsonObject o = beginOut();
      o["session"] = g_session;
      sendOut(MSG_EVE_SLEEP);
      break;
    }
    case EVE_MODE_LOW_POWER:
      /* Once per critical-battery episode (undocking while still critical does not replay it). */
      if (!g_lowPowerReacted) {
        g_lowPowerReacted = true;
        startRoutine(EVE_ROUTINE_LOW_POWER, now);
      }
      break;
    case EVE_MODE_ERROR:
      eveRoutineCancel();
      if (H.haltMotion) H.haltMotion(true);
      break;
    default:
      break;
  }
}

void applyMode(uint32_t now) {
  const EveMode to = computeMode();
  if (to == g_mode) return;
  const EveMode from = g_mode;
  g_mode = to;
  coreLog("[EVE][SM] %s -> %s", kModeNames[from], kModeNames[to]);
  if (H.onModeChanged) H.onModeChanged(from, to);
  onEnter(from, to, now);
}

// -----------------------------------------------------------------------------
// Link up / down
// -----------------------------------------------------------------------------
void linkUp(uint32_t session, const char* peer, bool hb, uint32_t now) {
  g_linked = true;
  g_session = session;
  copyStr(g_peer, sizeof(g_peer), peer);
  g_peerHb = hb;
  g_lastPeerRxMs = now;
  g_req = EVE_MODE_IDLE;
  g_attach = false;
  g_lastHbTxMs = now - EVE_HEARTBEAT_MS; /* first heartbeat on the next tick */
  g_lastCmdMs = now;
  S.linkUps++;
  sendReady();
  coreLog("[EVE][LINK] UART link up: session=%lu peer=%s supervision=%s (communication only, not proof of hand contact)",
       (unsigned long)session, g_peer, hb ? "heartbeat 2s" : "legacy 8s");
  if (eveSafetyStopLatched()) coreLog("[EVE][LINK] stop is still latched - send CLEAR_STOP to release");
  if (H.onLink) H.onLink(true, false);
  applyMode(now);
}

void linkDown(bool timedOut, const char* why, uint32_t now) {
  if (!g_linked) return;
  if (timedOut) {
    JsonObject o = beginOut();
    o["code"] = 1;
    o["msg"] = "link_timeout";
    sendOut(MSG_EVE_ERROR);
    S.linkLosses++;
  }
  coreLog("[EVE][LINK] link down (%s) after session=%lu peer=%s", why, (unsigned long)g_session, g_peer);
  g_linked = false;
  g_session = 0;
  copyStr(g_peer, sizeof(g_peer), "none");
  g_peerHb = false;
  g_req = EVE_MODE_IDLE;
  g_attach = false;
  g_mimic = false;
  g_mimicChg = false;
  g_helloDue = true;
  g_lostByTimeout = timedOut;
  /* Defined link-loss response: cancel gestures, freeze servos where they were last commanded
   * (no new motion), drop any operator glow. A latched stop stays latched. */
  eveRoutineCancel();
  if (H.haltMotion) H.haltMotion(false);
  if (H.glowOperator) H.glowOperator(-1);
  if (H.onLink) H.onLink(false, timedOut);
  applyMode(now);
}

void feedLiveness(uint32_t now) { g_lastPeerRxMs = now; }

// -----------------------------------------------------------------------------
// Routine gate shared by MSG_RUN_ROUTINE and local operator commands
// -----------------------------------------------------------------------------
bool routineGate(EveRoutineId id, uint32_t now, const char** why) {
  if (eveSafetyStopLatched()) {
    *why = "stopped";
    return false;
  }
  if (eveSafetyHasBlockingFault()) {
    *why = "fault";
    return false;
  }
  if (g_mode == EVE_MODE_LOW_POWER || eveSafetyPowerInhibit()) {
    *why = "low_power";
    return false;
  }
  if (g_mode == EVE_MODE_SLEEP) {
    if (id != EVE_ROUTINE_WAKE) {
      *why = "standby";
      return false;
    }
    g_req = EVE_MODE_IDLE; /* leaving standby runs the wake routine once (onEnter) */
    applyMode(now);
    return true;
  }
  if (!eveRoutineStart(id, now)) {
    *why = "busy";
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Command handlers (g_in holds the parsed JSON object, or is empty)
// -----------------------------------------------------------------------------
void handleAck(bool hasJson, uint32_t now) {
  if (!hasJson) {
    infoRateLog(now, "[EVE][LINK] WALL_E_ACK without JSON ignored");
    S.rejected++;
    return;
  }
  JsonVariantConst sv = g_in["session"];
  if (!sv.is<uint32_t>() || sv.as<uint32_t>() == 0) {
    infoRateLog(now, "[EVE][LINK] WALL_E_ACK without a valid session ignored");
    S.rejected++;
    return;
  }
  const uint32_t session = sv.as<uint32_t>();
  char peer[24];
  sanitizePeer(g_in["peer"] | "wall_e", peer, sizeof(peer));
  JsonVariantConst hv = g_in["hb"];
  const bool hb = hv.is<int>() && hv.as<int>() > 0;

  if (g_mimic) {
    coreLog("[EVE][LINK] real ACK received - leaving dock mimic");
    linkDown(false, "mimic_replaced", now);
  }
  if (g_linked) {
    if (session == g_session) {
      /* Duplicate ACK (WALL-E may have missed READY): answer again, nothing else changes. */
      feedLiveness(now);
      g_peerHb = hb;
      copyStr(g_peer, sizeof(g_peer), peer);
      sendReady();
      return;
    }
    coreLog("[EVE][LINK] new session %lu replaces %lu (peer restarted?)", (unsigned long)session,
         (unsigned long)g_session);
    linkDown(false, "new_session", now);
  }
  linkUp(session, peer, hb, now);
}

void handleStop(uint8_t seq, bool hasJson, uint32_t now) {
  const char* src = hasJson ? (g_in["src"] | "wall_e") : "wall_e";
  char srcCopy[24];
  copyStr(srcCopy, sizeof(srcCopy), src);
  const bool newly = eveSafetyLatchStop(EVE_STOP_SRC_WALLE);
  eveRoutineCancel();
  coreLog("[EVE][CMD] STOP from %s (%s)", srcCopy, newly ? "latched" : "already latched");
  JsonObject r = accept(seq, MSG_STOP);
  r["new"] = newly;
  finishResult(r);
  applyMode(now);
}

void handleClearStop(uint8_t seq, bool hasJson, uint32_t now) {
  /* The session is mandatory: a stale or echoed CLEAR_STOP from an old link must never release. */
  if (!hasJson || !g_in["session"].is<uint32_t>()) {
    reject(seq, MSG_CLEAR_STOP, "session_required", now);
    return;
  }
  const bool was = eveSafetyClearStop(EVE_STOP_SRC_WALLE);
  applyMode(now); /* before building the reply: a mode change may send its own frame */
  JsonObject r = accept(seq, MSG_CLEAR_STOP);
  r["was_stopped"] = was;
  finishResult(r);
}

void handleMode(uint8_t seq, uint8_t type, uint32_t now) {
  EveMode want = EVE_MODE_IDLE;
  bool allowedWhileStopped = false;
  bool needsPower = false;
  switch (type) {
    case MSG_ATTACH_CONFIRMED: want = EVE_MODE_ATTACHED; break;
    case MSG_MODE_ESCORT: want = EVE_MODE_ESCORT; needsPower = true; break;
    case MSG_MODE_INTERACT: want = EVE_MODE_INTERACT; needsPower = true; break;
    case MSG_MODE_IDLE: want = EVE_MODE_IDLE; break;
    case MSG_MODE_DOCK: want = EVE_MODE_DOCKED; allowedWhileStopped = true; break;
    case MSG_MODE_STANDBY: want = EVE_MODE_SLEEP; allowedWhileStopped = true; break;
    default: reject(seq, type, "unknown_type", now); return;
  }
  if (!allowedWhileStopped && eveSafetyStopLatched()) {
    reject(seq, type, "stopped", now);
    return;
  }
  if (needsPower && eveSafetyPowerInhibit()) {
    reject(seq, type, "low_power", now);
    return;
  }
  if (type == MSG_ATTACH_CONFIRMED) g_attach = true;
  g_req = want;
  applyMode(now);
  JsonObject r = accept(seq, type);
  r["req"] = kModeNames[g_req];
  finishResult(r);
}

void handleSetEyes(uint8_t seq, bool hasJson, uint32_t now) {
  if (eveSafetyStopLatched()) return reject(seq, MSG_SET_EYES, "stopped", now);
  if (eveSafetyHasBlockingFault()) return reject(seq, MSG_SET_EYES, "fault", now);
  if (!hasJson) return reject(seq, MSG_SET_EYES, "missing", now);
  JsonVariantConst e = g_in["e"];
  if (e.is<const char*>()) {
    char name[24];
    size_t i = 0;
    for (const char* p = e.as<const char*>(); *p && i + 1 < sizeof(name); p++) name[i++] = (char)tolower((unsigned char)*p);
    name[i] = '\0';
    const int id = eveExprFromName(name);
    if (id < 0) return reject(seq, MSG_SET_EYES, "bad_value", now);
    uint32_t ms = 2000;
    JsonVariantConst mv = g_in["ms"];
    if (!mv.isNull()) {
      if (!mv.is<int>() || mv.as<int>() < 0 || mv.as<int>() > 30000) return reject(seq, MSG_SET_EYES, "range", now);
      if (mv.as<int>() > 0) ms = (uint32_t)mv.as<int>();
    }
    if (H.eyesExpression) H.eyesExpression((uint8_t)id, ms);
    JsonObject r = accept(seq, MSG_SET_EYES);
    r["e"] = eveExprNameLower((uint8_t)id);
    finishResult(r);
    return;
  }
  JsonVariantConst lm = g_in["mode"];
  if (lm.is<int>()) {
    const int m = lm.as<int>();
    if (m < 0 || m > 2) return reject(seq, MSG_SET_EYES, "range", now);
    if (H.eyesLegacyMode) H.eyesLegacyMode((uint8_t)m);
    finishResult(accept(seq, MSG_SET_EYES));
    return;
  }
  reject(seq, MSG_SET_EYES, "missing", now);
}

void handleSetGlow(uint8_t seq, bool hasJson, uint32_t now) {
  if (eveSafetyStopLatched()) return reject(seq, MSG_SET_GLOW, "stopped", now);
  if (!hasJson) return reject(seq, MSG_SET_GLOW, "missing", now);
  int pattern = -2;
  JsonVariantConst pv = g_in["p"];
  JsonVariantConst nv = g_in["pattern"];
  if (pv.is<const char*>()) {
    const char* p = pv.as<const char*>();
    pattern = (strcmp(p, "auto") == 0) ? -1 : eveGlowFromName(p);
    if (pattern < -1 || (pattern == -1 && strcmp(p, "auto") != 0)) return reject(seq, MSG_SET_GLOW, "bad_value", now);
  } else if (nv.is<int>()) {
    pattern = nv.as<int>();
    if (pattern < 0 || pattern >= EVE_GLOW_COUNT) return reject(seq, MSG_SET_GLOW, "range", now);
  } else {
    return reject(seq, MSG_SET_GLOW, "missing", now);
  }
  if (H.glowOperator) H.glowOperator((int8_t)pattern);
  JsonObject r = accept(seq, MSG_SET_GLOW);
  r["p"] = pattern < 0 ? "auto" : eveGlowName((uint8_t)pattern);
  finishResult(r);
}

void handleMoveServo(uint8_t seq, bool hasJson, uint32_t now) {
  if (eveSafetyStopLatched()) return reject(seq, MSG_MOVE_SERVO, "stopped", now);
  if (eveSafetyHasBlockingFault()) return reject(seq, MSG_MOVE_SERVO, "fault", now);
  if (eveSafetyPowerInhibit()) return reject(seq, MSG_MOVE_SERVO, "low_power", now);
  if (!motionMode(g_mode)) return reject(seq, MSG_MOVE_SERVO, "mode", now);
  if (!hasJson) return reject(seq, MSG_MOVE_SERVO, "missing", now);

  JsonVariantConst lv = g_in["l"];
  JsonVariantConst rv = g_in["r"];
  JsonVariantConst dv = g_in["dps"];
  const bool hasL = !lv.isNull();
  const bool hasR = !rv.isNull();
  if (!hasL && !hasR) return reject(seq, MSG_MOVE_SERVO, "missing", now);
  if ((hasL && !lv.is<int>()) || (hasR && !rv.is<int>())) return reject(seq, MSG_MOVE_SERVO, "bad_value", now);
  const int l = hasL ? lv.as<int>() : 0;
  const int r = hasR ? rv.as<int>() : 0;
  if (l < -360 || l > 360 || r < -360 || r > 360) return reject(seq, MSG_MOVE_SERVO, "range", now);
  uint16_t dps = EVE_SERVO_DEFAULT_DPS;
  if (!dv.isNull()) {
    if (!dv.is<int>() || dv.as<int>() <= 0) return reject(seq, MSG_MOVE_SERVO, "range", now);
    dps = (uint16_t)(dv.as<int>() > EVE_SERVO_MAX_DPS ? EVE_SERVO_MAX_DPS : dv.as<int>());
  }
  const bool headOk = H.servoAvailable && H.servoAvailable(EVE_SERVO_HEAD_PAN);
  const bool armOk = H.servoAvailable && H.servoAvailable(EVE_SERVO_RIGHT_ARM);
  if ((hasL && !headOk && (!hasR || !armOk)) || (!hasL && hasR && !armOk)) {
    return reject(seq, MSG_MOVE_SERVO, "servo_unavailable", now);
  }

  /* Manual control replaces any running gesture. */
  eveRoutineCancel();
  bool clamped = false;
  bool refused = false;
  int16_t appliedL = -1, appliedR = -1;
  if (hasL && headOk) {
    int16_t mn = 0, mx = 180;
    if (H.servoLimits) H.servoLimits(EVE_SERVO_HEAD_PAN, &mn, &mx);
    int16_t d = (int16_t)l;
    if (d < mn) { d = mn; clamped = true; }
    if (d > mx) { d = mx; clamped = true; }
    if (H.servoMove && H.servoMove(EVE_SERVO_HEAD_PAN, d, dps)) appliedL = d; else refused = true;
  }
  if (hasR && armOk) {
    int16_t mn = 0, mx = 180;
    if (H.servoLimits) H.servoLimits(EVE_SERVO_RIGHT_ARM, &mn, &mx);
    int16_t d = (int16_t)r;
    if (d < mn) { d = mn; clamped = true; }
    if (d > mx) { d = mx; clamped = true; }
    if (H.servoMove && H.servoMove(EVE_SERVO_RIGHT_ARM, d, dps)) appliedR = d; else refused = true;
  }
  if (refused && appliedL < 0 && appliedR < 0) return reject(seq, MSG_MOVE_SERVO, "refused", now);
  JsonObject res = accept(seq, MSG_MOVE_SERVO);
  if (appliedL >= 0) res["l"] = appliedL;
  if (appliedR >= 0) res["r"] = appliedR;
  res["clamped"] = clamped;
  if (hasL != (appliedL >= 0) || hasR != (appliedR >= 0)) res["partial"] = true;
  finishResult(res);
}

void handlePlaySound(uint8_t seq, bool hasJson, uint32_t now) {
  if (!hasJson) return reject(seq, MSG_PLAY_SOUND, "missing", now);
  JsonVariantConst tv = g_in["track"];
  if (!tv.is<int>()) return reject(seq, MSG_PLAY_SOUND, "missing", now);
  const int track = tv.as<int>();
  if (track < 0 || track > 255) return reject(seq, MSG_PLAY_SOUND, "range", now);
  if (track == 0) {
    /* Track 0 = stop audio (the old STOP handler used it that way). Always allowed. */
    if (H.stopSound) H.stopSound();
    JsonObject r = accept(seq, MSG_PLAY_SOUND);
    r["audio"] = false;
    finishResult(r);
    return;
  }
  if (eveSafetyStopLatched()) return reject(seq, MSG_PLAY_SOUND, "stopped", now);
  if (eveSafetyHasBlockingFault()) return reject(seq, MSG_PLAY_SOUND, "fault", now);
  const bool audio = H.remoteSound ? H.remoteSound((uint8_t)track) : false;
  JsonObject r = accept(seq, MSG_PLAY_SOUND);
  r["track"] = track;
  r["audio"] = audio; /* false = no audio hardware or the file is missing; the eye reaction still ran */
  finishResult(r);
}

void handleRunRoutine(uint8_t seq, bool hasJson, uint32_t now) {
  if (!hasJson) return reject(seq, MSG_RUN_ROUTINE, "missing", now);
  JsonVariantConst rv = g_in["r"];
  if (!rv.is<const char*>()) return reject(seq, MSG_RUN_ROUTINE, "missing", now);
  const EveRoutineId id = eveRoutineFromName(rv.as<const char*>());
  if (id == EVE_ROUTINE_NONE || !eveRoutineOperatorAllowed(id)) return reject(seq, MSG_RUN_ROUTINE, "bad_value", now);
  const char* why = nullptr;
  if (!routineGate(id, now, &why)) return reject(seq, MSG_RUN_ROUTINE, why, now);
  JsonObject r = accept(seq, MSG_RUN_ROUTINE);
  r["r"] = eveRoutineName(id);
  if (eveRoutineIsTheatrical(id)) r["animation_only"] = true;
  finishResult(r);
}

}  // namespace

// =============================================================================
// Public API
// =============================================================================
void eveControlInit(const EveControlHooks* hooks, uint32_t nowMs) {
  if (hooks) H = *hooks;
  memset(&S, 0, sizeof(S));
  copyStr(S.lastReject, sizeof(S.lastReject), "none");
  g_bootMs = nowMs;
  g_mode = EVE_MODE_BOOT;
  g_req = EVE_MODE_IDLE;
  g_linked = false;
  g_session = 0;
  copyStr(g_peer, sizeof(g_peer), "none");
  g_peerHb = false;
  g_lastPeerRxMs = 0;
  g_helloDue = true;
  g_lastHelloMs = nowMs;
  g_lastHbTxMs = nowMs;
  g_lastCmdMs = nowMs;
  g_attach = false;
  g_mimic = false;
  g_mimicChg = false;
  g_bat = EVE_BATLVL_UNKNOWN;
  g_batChg = -1;
  g_batFull = false;
  g_warnReacted = false;
  g_lowPowerReacted = false;
  g_lostByTimeout = false;
  g_rejectLogged = false;
  g_rejectSuppressed = 0;
  g_infoLogged = false;
  applyMode(nowMs); /* BOOT -> WAIT_ATTACH (or ERROR if a blocking fault is already set) */
}

void eveControlTick(uint32_t now) {
  if (!g_linked) {
    if (g_helloDue || (uint32_t)(now - g_lastHelloMs) >= EVE_HELLO_RETRY_MS) {
      g_helloDue = false;
      g_lastHelloMs = now;
      sendHello(now);
    }
  } else {
    if ((uint32_t)(now - g_lastHbTxMs) >= EVE_HEARTBEAT_MS) {
      g_lastHbTxMs = now;
      sendHeartbeat(now);
    }
    if (!g_mimic) {
      const bool supervised =
          g_peerHb || g_mode == EVE_MODE_ESCORT || g_mode == EVE_MODE_INTERACT || g_mode == EVE_MODE_DOCKED;
      const uint32_t limit = g_peerHb ? EVE_LINK_TIMEOUT_MS : EVE_LINK_LOST_MS;
      if (supervised && (uint32_t)(now - g_lastPeerRxMs) > limit) {
        linkDown(true, "no valid frame from peer", now);
      }
    }
    if (g_linked && g_bat == EVE_BATLVL_CRITICAL && (uint32_t)(now - g_lastLowBatTxMs) >= EVE_BAT_LOW_REPORT_MIN_MS) {
      sendLowBattery(now);
    }
#if EVE_IDLE_STANDBY_MS > 0
    if (g_linked && (g_req == EVE_MODE_IDLE || g_req == EVE_MODE_ATTACHED) && g_mode == g_req &&
        eveRoutineActive() == EVE_ROUTINE_NONE && (uint32_t)(now - g_lastCmdMs) >= EVE_IDLE_STANDBY_MS) {
      coreLog("[EVE][SM] no commands for %lu ms -> standby", (unsigned long)EVE_IDLE_STANDBY_MS);
      g_req = EVE_MODE_SLEEP;
    }
#endif
  }
  eveRoutinesTick(now);
  applyMode(now);
}

void eveControlOnFrame(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq, uint32_t now) {
  S.framesIn++;
  if (len > EVE_MAX_PAYLOAD || (len > 0 && payload == nullptr)) return; /* decoder already enforces */
  if (isEveToPeer(type)) {
    S.echoIgnored++;
    infoRateLog(now, "[EVE][LINK] ignored %s arriving at EVE (TX/RX looped back or crossed?)", eveLinkMsgName(type));
    return;
  }
  if (!isPeerToEve(type)) {
    infoRateLog(now, "[EVE][LINK] ignored unknown frame type 0x%02x", (unsigned)type);
    return;
  }

  if (len > 0) memcpy(g_text, payload, len);
  g_text[len] = '\0';
  g_in.clear();
  bool jsonOk = true;
  bool hasJson = false;
  if (len > 0) {
    const DeserializationError err = deserializeJson(g_in, (const char*)g_text, len);
    jsonOk = !err;
    hasJson = jsonOk && g_in.is<JsonObject>();
  }

  if (type == MSG_WALL_E_ACK) {
    handleAck(hasJson, now);
    return;
  }

  /* Session check: a frame that names a session must name the current one. */
  bool sessionOk = true;
  if (hasJson) {
    JsonVariantConst sv = g_in["session"];
    if (!sv.isNull()) sessionOk = g_linked && sv.is<uint32_t>() && sv.as<uint32_t>() == g_session;
  }

  /* STOP is accepted in every state, before the handshake, with any (or no) session. */
  if (type == MSG_STOP) {
    if (g_linked && sessionOk) feedLiveness(now);
    handleStop(seq, hasJson, now);
    return;
  }

  if (!g_linked) {
    if (isCoreCommand(type)) reject(seq, type, "not_linked", now);
    return;
  }
  if (!sessionOk) {
    S.sessionRejected++;
    if (isCoreCommand(type)) reject(seq, type, "session", now);
    return;
  }
  feedLiveness(now);

  if (!isCoreCommand(type)) {
    if (type == MSG_BUS_COMMAND) {
      g_lastCmdMs = now;
      if (H.busCommand && H.busCommand(g_text)) coreLog("[EVE][CMD] BUS_COMMAND applied");
    }
    return; /* other shared-bus frames (dock status/identify) only prove the peer is alive */
  }
  if (type == MSG_WALL_E_HEARTBEAT) return;
  if (len > 0 && !hasJson) {
    reject(seq, type, "bad_json", now);
    return;
  }
  g_lastCmdMs = now;

  switch (type) {
    case MSG_ATTACH_CONFIRMED:
    case MSG_MODE_ESCORT:
    case MSG_MODE_DOCK:
    case MSG_MODE_IDLE:
    case MSG_MODE_INTERACT:
    case MSG_MODE_STANDBY:
      handleMode(seq, type, now);
      break;
    case MSG_SET_EYES: handleSetEyes(seq, hasJson, now); break;
    case MSG_SET_GLOW: handleSetGlow(seq, hasJson, now); break;
    case MSG_MOVE_SERVO: handleMoveServo(seq, hasJson, now); break;
    case MSG_PLAY_SOUND: handlePlaySound(seq, hasJson, now); break;
    case MSG_CLEAR_STOP: handleClearStop(seq, hasJson, now); break;
    case MSG_RUN_ROUTINE: handleRunRoutine(seq, hasJson, now); break;
    case MSG_RESET_STATE: {
      JsonObject r = accept(seq, type);
      finishResult(r);
      if (eveSafetyStopLatched()) coreLog("[EVE][CMD] RESET_STATE: handshake restarts, stop stays latched");
      linkDown(false, "reset_state", now);
      break;
    }
    default:
      reject(seq, type, "unknown_type", now);
      break;
  }
}

void eveControlSetBattery(EveBatLevel level, int8_t charging, bool full, uint32_t now) {
  const EveBatLevel prev = g_bat;
  g_batChg = charging;
  if (level != prev) {
    static const char* const kLvl[] = {"unknown", "ok", "warn", "critical"};
    g_bat = level;
    coreLog("[EVE][BAT] level %s -> %s", kLvl[prev], kLvl[level]);
    if (level == EVE_BATLVL_CRITICAL) {
      eveSafetySetPowerInhibit(true);
      g_warnReacted = true; /* recovering through WARN later must not replay the warning */
      if (g_linked) sendLowBattery(now);
    } else if (level == EVE_BATLVL_OK || level == EVE_BATLVL_WARN) {
      eveSafetySetPowerInhibit(false);
      g_lowPowerReacted = false;
      if (level == EVE_BATLVL_OK) g_warnReacted = false;
      if (level == EVE_BATLVL_WARN && !g_warnReacted) {
        /* Once per descent into WARN — sensor glitches (WARN->UNKNOWN->WARN) do not repeat it. */
        g_warnReacted = true;
        startRoutine(EVE_ROUTINE_LOW_WARN, now);
      }
    }
    /* UNKNOWN: an invalid reading is not a recovery, so a power inhibit stays as it was. */
  }
  if (full && !g_batFull) startRoutine(EVE_ROUTINE_FULL, now);
  g_batFull = full;
  applyMode(now);
}

void eveControlLocalStop(EveStopSource src, uint32_t nowMs) {
  eveSafetyLatchStop(src);
  eveRoutineCancel();
  applyMode(nowMs);
}

bool eveControlLocalClearStop(EveStopSource src, uint32_t nowMs) {
  const bool was = eveSafetyClearStop(src);
  applyMode(nowMs);
  return was;
}

void eveControlLocalReset(uint32_t nowMs) {
  if (g_linked) {
    linkDown(false, "local_reset", nowMs);
  } else {
    g_helloDue = true;
  }
  if (eveSafetyStopLatched()) coreLog("[EVE][SM] reset: handshake restarts, stop stays latched");
}

bool eveControlLocalRoutine(uint8_t routineId, uint32_t nowMs, const char** whyNot) {
  const char* why = nullptr;
  const EveRoutineId id = (EveRoutineId)routineId;
  bool ok = false;
  if (!eveRoutineOperatorAllowed(id)) {
    why = "bad_value";
  } else {
    ok = routineGate(id, nowMs, &why);
  }
  if (whyNot) *whyNot = ok ? nullptr : why;
  return ok;
}

bool eveControlLocalStandby(bool on, uint32_t nowMs, const char** whyNot) {
  if (!g_linked) {
    if (whyNot) *whyNot = "not_linked";
    return false;
  }
  if (on) {
    g_req = EVE_MODE_SLEEP;
  } else if (g_req == EVE_MODE_SLEEP) {
    if (eveSafetyStopLatched()) {
      if (whyNot) *whyNot = "stopped";
      return false;
    }
    g_req = EVE_MODE_IDLE;
  }
  g_lastCmdMs = nowMs;
  applyMode(nowMs);
  if (whyNot) *whyNot = nullptr;
  return true;
}

void eveControlSetDockMimic(bool enabled, bool fakeCharging, uint32_t nowMs) {
  if (enabled) {
    if (g_linked && !g_mimic) linkDown(false, "dock_mimic", nowMs);
    g_mimic = true;
    g_mimicChg = fakeCharging;
    g_linked = true;
    g_session = kMimicSession;
    copyStr(g_peer, sizeof(g_peer), "dock_test");
    g_peerHb = false;
    g_lastPeerRxMs = nowMs;
    g_req = EVE_MODE_DOCKED;
    g_lastCmdMs = nowMs;
    coreLog("[EVE][SM] DOCK MIMIC ON (bench simulation, fake charging=%s)", fakeCharging ? "yes" : "no");
    if (H.onLink) H.onLink(true, false);
    applyMode(nowMs);
  } else if (g_mimic) {
    linkDown(false, "dock_mimic_off", nowMs);
    coreLog("[EVE][SM] DOCK MIMIC OFF");
  }
}

EveMode eveControlMode(void) { return g_mode; }
EveMode eveControlRequestedMode(void) { return g_req; }
const char* eveControlModeName(EveMode m) { return m < EVE_MODE_COUNT ? kModeNames[m] : "UNKNOWN"; }
bool eveControlLinked(void) { return g_linked; }
uint32_t eveControlSession(void) { return g_session; }
const char* eveControlPeer(void) { return g_peer; }
bool eveControlPeerHeartbeats(void) { return g_peerHb; }
uint32_t eveControlLinkAgeMs(uint32_t nowMs) { return g_linked ? (uint32_t)(nowMs - g_lastPeerRxMs) : 0; }
bool eveControlDockMimic(void) { return g_mimic; }
bool eveControlDockMimicCharging(void) { return g_mimic && g_mimicChg; }
bool eveControlAttachAsserted(void) { return g_attach; }
EveBatLevel eveControlBatteryLevel(void) { return g_bat; }
const EveControlStats* eveControlStats(void) { return &S; }

bool eveControlAllowsCompanionUart(void) {
  if (!g_linked || g_session == 0) return false;
  switch (g_mode) {
    case EVE_MODE_IDLE:
    case EVE_MODE_ATTACHED:
    case EVE_MODE_ESCORT:
    case EVE_MODE_INTERACT:
    case EVE_MODE_DOCKED:
    case EVE_MODE_LOW_POWER:
      return true;
    default:
      return false;
  }
}

size_t eveControlStatusJson(char* out, size_t cap, uint32_t nowMs) {
  if (!out || cap == 0) return 0;
  buildStatus(beginOut(), nowMs, true);
  const size_t n = serializeJson(g_outDoc, out, cap);
  if (n >= cap) out[cap - 1] = '\0';
  return n;
}
