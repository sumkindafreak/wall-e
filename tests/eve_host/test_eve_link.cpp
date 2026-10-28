/**
 * Host tests: EVE control core + safety + routines <-> WALL-E peer link over a simulated UART.
 *
 * Everything here is SIMULATED: the wire is two byte queues, the clock is a variable, and the
 * "servos", "audio" and "eyes" are counters. Passing these tests says the logic behaves; it says
 * nothing about wiring, servo direction, battery readings or timing on the real robot.
 */
#include <ArduinoJson.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "eve_battery_logic.h"
#include "eve_control_core.h"
#include "eve_output_ids.h"
#include "eve_peer_link.h"
#include "eve_pin_check.h"
#include "eve_protocol.h"
#include "eve_routines.h"
#include "eve_safety.h"

// =============================================================================
// Tiny test framework
// =============================================================================
static int g_checks = 0;
static int g_failures = 0;
static const char* g_test = "";
static bool g_verbose = false;

#define CHECK(cond)                                                                        \
  do {                                                                                     \
    g_checks++;                                                                            \
    if (!(cond)) {                                                                         \
      g_failures++;                                                                        \
      std::printf("  FAIL [%s] %s:%d: %s\n", g_test, __FILE__, __LINE__, #cond);           \
    }                                                                                      \
  } while (0)

#define CHECK_EQ_STR(a, b)                                                                 \
  do {                                                                                     \
    g_checks++;                                                                            \
    const std::string _a = (a), _b = (b);                                                  \
    if (_a != _b) {                                                                        \
      g_failures++;                                                                        \
      std::printf("  FAIL [%s] %s:%d: \"%s\" != \"%s\"\n", g_test, __FILE__, __LINE__,     \
                  _a.c_str(), _b.c_str());                                                 \
    }                                                                                      \
  } while (0)

// =============================================================================
// Simulated world
// =============================================================================
struct Sent {
  uint8_t type;
  std::string json;
  uint32_t t;
};

struct World {
  uint32_t now = 1000;
  std::deque<uint8_t> toEve;    /* bytes arriving at EVE's RX */
  std::deque<uint8_t> toWallE;  /* bytes arriving at WALL-E's RX */
  bool cutToEve = false;        /* WALL-E -> EVE wire broken */
  bool cutToWallE = false;      /* EVE -> WALL-E wire broken */
  bool walleRunning = true;     /* WALL-E's firmware is ticking */
  EveFrameDecoder eveDec, walleDec;
  uint8_t eveSeq = 0;
  std::vector<Sent> eveSent, walleSent;
  // Fake outputs
  int servoMoves = 0;
  int lastHead = -1, lastArm = -1;
  int halts = 0, haltsStop = 0, soundStops = 0, sounds = 0, routineSounds = 0;
  int lastExpr = -1, lastGlowOperator = -2;
  std::vector<std::string> modes;
  std::vector<std::string> logs;
  EveBatteryReport bat{};
};
static World* W = nullptr;

static void pushFrame(std::deque<uint8_t>& q, uint8_t type, uint8_t seq, const char* json) {
  uint8_t buf[EVE_MAX_PAYLOAD + EVE_FRAME_OVERHEAD];
  const size_t n = eveLinkEncode(type, seq, (const uint8_t*)json, (uint16_t)strlen(json), buf, sizeof(buf));
  for (size_t i = 0; i < n; i++) q.push_back(buf[i]);
}

// ---- EVE side hooks ------------------------------------------------------------------------------
static bool eveSend(uint8_t type, const char* json) {
  W->eveSent.push_back({type, json, W->now});
  if (!W->cutToWallE) pushFrame(W->toWallE, type, ++W->eveSeq, json);
  return true;
}
static void eveBattery(EveBatteryReport* out) { *out = W->bat; }
static uint32_t eveHeap() { return 123456; }
static uint8_t eveOuts() { return EVE_OUT_SERVOS | EVE_OUT_ONBOARD_RGB | EVE_OUT_BATTERY; }
static int8_t evePresent() { return -1; }
static bool servoAvail(uint8_t) { return true; }
static void servoLimits(uint8_t ch, int16_t* mn, int16_t* mx) {
  if (ch == EVE_SERVO_HEAD_PAN) { *mn = 45; *mx = 135; } else { *mn = 0; *mx = 180; }
}
/* Mimics servo_control: refuses whenever eve_safety says motion is not allowed. */
static bool servoMove(uint8_t ch, int16_t deg, uint16_t) {
  if (!eveSafetyMotionAllowed()) return false;
  W->servoMoves++;
  if (ch == EVE_SERVO_HEAD_PAN) W->lastHead = deg; else W->lastArm = deg;
  return true;
}
static void haltMotion(bool stop) { W->halts++; if (stop) W->haltsStop++; }
static bool remoteSound(uint8_t) {
  if (!eveSafetySoundAllowed()) return false;
  W->sounds++;
  return true;
}
static void stopSound() { W->soundStops++; }
static void eyesExpr(uint8_t id, uint32_t) { W->lastExpr = id; }
static void eyesLegacy(uint8_t) {}
static void glowOp(int8_t p) { W->lastGlowOperator = p; }
static bool busCmd(const char*) { return true; }
static void modeChanged(EveMode, EveMode to) { W->modes.push_back(eveControlModeName(to)); }
static void onLink(bool, bool) {}
static void logLine(const char* l) {
  W->logs.push_back(l);
  if (g_verbose) std::printf("    | %s\n", l);
}
// Routine hooks
static void rExpr(uint8_t id, uint32_t) { W->lastExpr = id; }
static void rGlow(uint8_t, uint32_t) {}
static void rSound(uint8_t) { if (eveSafetySoundAllowed()) W->routineSounds++; }
static bool rServo(uint8_t ch, int16_t deg, uint16_t dps) { return servoMove(ch, deg, dps); }
// Safety hook: what a stop does (same order as state_machine.cpp)
static void haltOutputs() {
  eveRoutineCancel();
  haltMotion(true);
  stopSound();
}

// ---- WALL-E side hooks ---------------------------------------------------------------------------
static bool walleSend(uint8_t type, uint8_t seq, const char* json) {
  W->walleSent.push_back({type, json, W->now});
  if (!W->cutToEve) pushFrame(W->toEve, type, seq, json);
  return true;
}
static uint32_t g_rand = 0x1234567u;
static uint32_t walleRandom() {
  g_rand = g_rand * 1664525u + 1013904223u;
  return g_rand;
}

static void initEve() {
  /* A real reboot clears RAM, including the stop latch. */
  eveSafetyClearStop(EVE_STOP_SRC_SERIAL);
  eveSafetySetFault(0xFFFF, false);
  eveSafetySetPowerInhibit(false);
  static const EveSafetyHooks sh = {haltOutputs, logLine};
  eveSafetyInit(&sh);
  static const EveRoutineHooks rh = {rExpr, rGlow, rSound, rServo, eveSafetyRoutinesAllowed, logLine};
  eveRoutinesInit(&rh);
  EveControlHooks h{};
  h.send = eveSend;
  h.battery = eveBattery;
  h.freeHeap = eveHeap;
  h.outputs = eveOuts;
  h.present = evePresent;
  h.servoAvailable = servoAvail;
  h.servoLimits = servoLimits;
  h.servoMove = servoMove;
  h.haltMotion = haltMotion;
  h.remoteSound = remoteSound;
  h.stopSound = stopSound;
  h.eyesExpression = eyesExpr;
  h.eyesLegacyMode = eyesLegacy;
  h.glowOperator = glowOp;
  h.busCommand = busCmd;
  h.onModeChanged = modeChanged;
  h.onLink = onLink;
  h.log = logLine;
  eveControlInit(&h, W->now);
  W->eveDec.reset();
}

static void initWallE() {
  static const EvePeerHooks ph = {walleSend, logLine, nullptr, walleRandom};
  evePeerInit(&ph, W->now);
  W->walleDec.reset();
}

static void newWorld(World& w) {
  W = &w;
  w.bat.charging = -1;
  initEve();
  initWallE();
}

static void pump() {
  while (!W->toEve.empty()) {
    const uint8_t c = W->toEve.front();
    W->toEve.pop_front();
    if (W->eveDec.feed(c)) eveControlOnFrame(W->eveDec.type(), W->eveDec.payload(), W->eveDec.len(), W->eveDec.seq(), W->now);
  }
  while (!W->toWallE.empty()) {
    const uint8_t c = W->toWallE.front();
    W->toWallE.pop_front();
    if (!W->walleRunning) continue;
    if (W->walleDec.feed(c)) evePeerOnFrame(W->walleDec.type(), W->walleDec.payload(), W->walleDec.len(), W->walleDec.seq(), W->now);
  }
}

/** Advance the simulated clock in 10 ms steps, ticking both boards. */
static void run(uint32_t ms, const std::function<void()>& each = nullptr) {
  for (uint32_t t = 0; t < ms; t += 10) {
    W->now += 10;
    pump();
    eveControlTick(W->now);
    if (W->walleRunning) evePeerTick(W->now);
    if (each) each();
    pump();
  }
}

/** Inject a raw command frame at EVE (as if from WALL-E) and return EVE's CMD_RESULT JSON. */
static uint8_t g_injSeq = 200;
static std::string inject(uint8_t type, const char* json) {
  const size_t before = W->eveSent.size();
  const uint8_t seq = ++g_injSeq;
  pushFrame(W->toEve, type, seq, json);
  pump();
  for (size_t i = before; i < W->eveSent.size(); i++) {
    if (W->eveSent[i].type == MSG_EVE_CMD_RESULT) return W->eveSent[i].json;
  }
  return "";
}

static std::string field(const std::string& json, const char* key) {
  StaticJsonDocument<1024> d;
  if (deserializeJson(d, json)) return "<bad json>";
  JsonVariantConst v = d[key];
  if (v.isNull()) return "<null>";
  if (v.is<bool>()) return v.as<bool>() ? "true" : "false";
  if (v.is<long>()) return std::to_string(v.as<long>());
  if (v.is<const char*>()) return v.as<const char*>();
  std::string s;
  serializeJson(v, s);
  return s;
}

static void linkUp() {
  run(1500);
  CHECK(eveControlLinked());
  CHECK(evePeerLinked());
}

static int countSent(const std::vector<Sent>& v, uint8_t type, uint32_t since = 0) {
  int n = 0;
  for (auto& s : v) if (s.type == type && s.t >= since) n++;
  return n;
}

static std::string sessionJson(uint32_t extraField = 0, const char* extra = nullptr) {
  char b[96];
  if (extra) snprintf(b, sizeof(b), "{\"session\":%lu,%s}", (unsigned long)(eveControlSession() + extraField), extra);
  else snprintf(b, sizeof(b), "{\"session\":%lu}", (unsigned long)(eveControlSession() + extraField));
  return b;
}

// =============================================================================
// Tests
// =============================================================================
static void test_codec() {
  g_test = "codec";
  EveFrameDecoder d;
  uint8_t buf[600];
  const char* js = "{\"a\":1}";
  size_t n = eveLinkEncode(0x22, 7, (const uint8_t*)js, (uint16_t)strlen(js), buf, sizeof(buf));
  CHECK(n == EVE_FRAME_OVERHEAD + strlen(js));
  int got = 0;
  for (size_t i = 0; i < n; i++) if (d.feed(buf[i])) {
    got++;
    CHECK(d.type() == 0x22);
    CHECK(d.seq() == 7);
    CHECK(d.len() == strlen(js));
    CHECK(std::string(d.text()) == js);
  }
  CHECK(got == 1);

  // Corrupted payload -> CRC error, no frame
  buf[8] ^= 0x40;
  got = 0;
  for (size_t i = 0; i < n; i++) got += d.feed(buf[i]);
  CHECK(got == 0);
  CHECK(d.stats().crcErr == 1);

  // Wrong version with a correct CRC -> version error
  n = eveLinkEncode(0x22, 8, (const uint8_t*)js, (uint16_t)strlen(js), buf, sizeof(buf));
  buf[2] = 0x02;
  const uint16_t crc = eveLinkCrc16(buf + 2, 5 + strlen(js));
  buf[7 + strlen(js)] = (uint8_t)(crc & 0xFF);
  buf[8 + strlen(js)] = (uint8_t)(crc >> 8);
  got = 0;
  for (size_t i = 0; i < n; i++) got += d.feed(buf[i]);
  CHECK(got == 0);
  CHECK(d.stats().versionErr == 1);

  // Oversized length field -> length error
  const uint8_t big[] = {EVE_SOF0, EVE_SOF1, 0x01, 0x22, 0x01, 0x03, 0x00};
  for (uint8_t b : big) d.feed(b);
  CHECK(d.stats().lengthErr == 1);

  // Too-large payload refused by the encoder
  std::string huge(EVE_MAX_PAYLOAD + 1, 'x');
  CHECK(eveLinkEncode(0x22, 1, (const uint8_t*)huge.data(), (uint16_t)huge.size(), buf, sizeof(buf)) == 0);

  // Resync after noise (including SOF bytes): later valid frames still decode
  std::mt19937 rng(42);
  for (int i = 0; i < 3000; i++) d.feed((uint8_t)(i % 97 == 0 ? EVE_SOF0 : rng()));
  got = 0;
  for (int k = 0; k < 60; k++) {
    n = eveLinkEncode(0x04, (uint8_t)k, (const uint8_t*)js, (uint16_t)strlen(js), buf, sizeof(buf));
    for (size_t i = 0; i < n; i++) got += d.feed(buf[i]);
  }
  CHECK(got >= 30);
}

static void test_handshake_and_heartbeats() {
  g_test = "handshake";
  World w;
  newWorld(w);
  CHECK(eveControlMode() == EVE_MODE_WAIT_ATTACH);
  const uint32_t wake0 = eveRoutineStartCount(EVE_ROUTINE_WAKE);
  linkUp();
  CHECK(eveControlSession() != 0);
  CHECK(eveControlSession() == evePeerSession());
  CHECK(eveControlMode() == EVE_MODE_IDLE);
  CHECK(eveControlPeerHeartbeats());
  CHECK_EQ_STR(eveControlPeer(), "wall_e");
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0 + 1);
  CHECK(evePeerEveStatus()->valid);
  CHECK_EQ_STR(evePeerEveStatus()->st, "IDLE");
  CHECK(evePeerEveCaps() & 0x0008);

  // Ten quiet seconds: heartbeats both ways keep it up; the wake response does not repeat.
  run(10000);
  CHECK(eveControlLinked());
  CHECK(evePeerLinked());
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0 + 1);
  CHECK(eveControlStats()->linkLosses == 0);
  CHECK(countSent(w.walleSent, MSG_WALL_E_HEARTBEAT) > 30);
  CHECK(countSent(w.eveSent, MSG_EVE_HEARTBEAT) > 30);

  // Heartbeat payload stays within the frame limit with long values
  w.bat = {true, true, -12345.67f, -9999.99f, 100, 2, 1, true};
  inject(MSG_CLEAR_STOP, "{}"); /* produces lastReject = "session_required" (longest-ish reason) */
  run(300);
  size_t maxLen = 0;
  for (auto& s : w.eveSent) if (s.type == MSG_EVE_HEARTBEAT && s.json.size() > maxLen) maxLen = s.json.size();
  CHECK(maxLen > 0 && maxLen <= EVE_MAX_PAYLOAD);
  std::printf("  info: largest heartbeat %zu bytes (limit %u)\n", maxLen, (unsigned)EVE_MAX_PAYLOAD);
  char web[1200];
  const size_t webLen = eveControlStatusJson(web, sizeof(web), w.now);
  CHECK(webLen > 0 && webLen < 900); /* eve_status_manager buffer is 900 */
  bool overflow = false;
  for (auto& l : w.logs) if (l.find("overflowed") != std::string::npos) overflow = true;
  CHECK(!overflow);
}

static void test_link_loss_and_reconnect() {
  g_test = "link-loss";
  World w;
  newWorld(w);
  linkUp();
  const uint32_t lost0 = eveRoutineStartCount(EVE_ROUTINE_LOST);
  const uint32_t wake0 = eveRoutineStartCount(EVE_ROUTINE_WAKE);
  w.cutToEve = w.cutToWallE = true;
  run(1700); /* last heartbeat was <= 250 ms before the cut: still inside the 2 s window */
  CHECK(eveControlLinked());
  run(600);
  CHECK(!eveControlLinked());
  CHECK(eveControlMode() == EVE_MODE_WAIT_ATTACH);
  CHECK(!evePeerLinked());
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOST) == lost0 + 1);
  CHECK(w.halts >= 1); /* motion frozen on link loss */
  run(5000); /* HELLO retries do not replay the lost reaction */
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOST) == lost0 + 1);
  CHECK(countSent(w.eveSent, MSG_EVE_HELLO, w.now - 5000) >= 9);

  w.cutToEve = w.cutToWallE = false;
  run(1500);
  CHECK(eveControlLinked());
  CHECK(evePeerLinked());
  CHECK(eveControlMode() == EVE_MODE_IDLE);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0 + 1);
  CHECK(eveControlStats()->linkLosses == 1);
}

static void test_noise_and_echo_do_not_keep_link() {
  g_test = "noise";
  World w;
  newWorld(w);
  linkUp();
  // WALL-E goes silent; only noise reaches EVE.
  w.walleRunning = false;
  w.cutToEve = true;
  std::mt19937 rng(7);
  uint32_t dropAt = 0;
  run(4000, [&]() {
    for (int i = 0; i < 20; i++) w.toEve.push_back((uint8_t)(i == 3 ? EVE_SOF0 : (i == 4 ? EVE_SOF1 : rng())));
    if (!dropAt && !eveControlLinked()) dropAt = w.now;
  });
  CHECK(!eveControlLinked());
  CHECK(dropAt != 0);
  const EveFrameDecoder::Stats& st = w.eveDec.stats();
  CHECK(st.crcErr + st.versionErr + st.lengthErr > 0);

  // Echo: EVE's own frames looped back to her RX never count as WALL-E.
  World w2;
  newWorld(w2);
  linkUp();
  w2.walleRunning = false;
  w2.cutToEve = true;
  size_t echoed = w2.eveSent.size();
  run(3000, [&]() {
    while (echoed < w2.eveSent.size()) {
      pushFrame(w2.toEve, w2.eveSent[echoed].type, 1, w2.eveSent[echoed].json.c_str());
      echoed++;
    }
  });
  CHECK(!eveControlLinked());
  CHECK(eveControlStats()->echoIgnored > 0);
}

static void test_stale_session_rejected() {
  g_test = "session";
  World w;
  newWorld(w);
  linkUp();
  w.walleRunning = false;
  w.cutToEve = true;
  run(3000, [&]() {
    if (w.now % 250 == 0 && eveControlLinked()) {
      pushFrame(w.toEve, MSG_WALL_E_HEARTBEAT, 9, sessionJson(1).c_str()); /* wrong session */
    }
  });
  CHECK(!eveControlLinked());
  CHECK(eveControlStats()->sessionRejected > 0);
}

static void test_malformed_commands() {
  g_test = "malformed";
  World w;
  newWorld(w);
  // Before the handshake every command except STOP is refused.
  std::string r = inject(MSG_MOVE_SERVO, "{\"l\":90}");
  CHECK_EQ_STR(field(r, "ok"), "false");
  CHECK_EQ_STR(field(r, "err"), "not_linked");

  linkUp();
  run(5000); /* let the wake routine finish */
  const int moves0 = w.servoMoves;
  r = inject(MSG_MOVE_SERVO, "{bad json");
  CHECK_EQ_STR(field(r, "err"), "bad_json");
  r = inject(MSG_MOVE_SERVO, "[1,2]");
  CHECK_EQ_STR(field(r, "err"), "bad_json");
  r = inject(MSG_MOVE_SERVO, "{\"l\":\"ninety\"}");
  CHECK_EQ_STR(field(r, "err"), "bad_value");
  r = inject(MSG_MOVE_SERVO, "{\"l\":90.5}");
  CHECK_EQ_STR(field(r, "err"), "bad_value");
  r = inject(MSG_MOVE_SERVO, "{\"l\":999}");
  CHECK_EQ_STR(field(r, "err"), "range");
  r = inject(MSG_MOVE_SERVO, "{}");
  CHECK_EQ_STR(field(r, "err"), "missing");
  r = inject(MSG_MOVE_SERVO, "{\"l\":100,\"dps\":0}");
  CHECK_EQ_STR(field(r, "err"), "range");
  r = inject(MSG_MOVE_SERVO, sessionJson(1, "\"l\":90").c_str());
  CHECK_EQ_STR(field(r, "err"), "session");
  CHECK(w.servoMoves == moves0); /* nothing above moved anything */

  r = inject(MSG_MOVE_SERVO, "{\"l\":200,\"r\":-5}");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK_EQ_STR(field(r, "clamped"), "true");
  CHECK(w.lastHead == 135);
  CHECK(w.lastArm == 0);
  CHECK(w.servoMoves == moves0 + 2);
  r = inject(MSG_MOVE_SERVO, "{\"l\":100}");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK(w.lastArm == 0); /* a missing channel is left alone (it used to snap to 90) */

  r = inject(MSG_SET_EYES, "{\"e\":\"nonsense\"}");
  CHECK_EQ_STR(field(r, "err"), "bad_value");
  r = inject(MSG_SET_EYES, "{\"e\":\"HAPPY\",\"ms\":1500}");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK(w.lastExpr == 9);
  r = inject(MSG_SET_EYES, "{\"e\":\"happy\",\"ms\":-4}");
  CHECK_EQ_STR(field(r, "err"), "range");
  r = inject(MSG_SET_EYES, "{\"mode\":7}");
  CHECK_EQ_STR(field(r, "err"), "range");

  r = inject(MSG_SET_GLOW, "{\"pattern\":99}");
  CHECK_EQ_STR(field(r, "err"), "range");
  r = inject(MSG_SET_GLOW, "{\"p\":\"sparkly\"}");
  CHECK_EQ_STR(field(r, "err"), "bad_value");
  r = inject(MSG_SET_GLOW, "{\"p\":\"escort\"}");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK(w.lastGlowOperator == EVE_GLOW_ESCORT);
  r = inject(MSG_SET_GLOW, "{\"p\":\"auto\"}");
  CHECK(w.lastGlowOperator == -1);

  r = inject(MSG_PLAY_SOUND, "{}");
  CHECK_EQ_STR(field(r, "err"), "missing");
  r = inject(MSG_PLAY_SOUND, "{\"track\":300}");
  CHECK_EQ_STR(field(r, "err"), "range");
  r = inject(MSG_PLAY_SOUND, "{\"track\":3}");
  CHECK_EQ_STR(field(r, "ok"), "true");

  r = inject(MSG_RUN_ROUTINE, "{\"r\":\"lost\"}");
  CHECK_EQ_STR(field(r, "err"), "bad_value"); /* not operator-triggerable */
  r = inject(MSG_RUN_ROUTINE, "{\"r\":\"dance\"}");
  CHECK_EQ_STR(field(r, "err"), "bad_value");
  r = inject(MSG_RUN_ROUTINE, "{\"r\":\"scan\"}");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK_EQ_STR(field(r, "animation_only"), "true");

  r = inject(0x3E, "{}");
  CHECK_EQ_STR(field(r, "err"), "unknown_type");
  CHECK(eveControlLinked()); /* rejected commands never break the link */
}

static void test_stop_latch() {
  g_test = "stop";
  World w;
  newWorld(w);
  // STOP before any handshake, empty payload: accepted.
  std::string r = inject(MSG_STOP, "");
  CHECK_EQ_STR(field(r, "ok"), "true");
  CHECK(eveSafetyStopLatched());
  CHECK(w.haltsStop == 1 && w.soundStops >= 1);

  const uint32_t wake0 = eveRoutineStartCount(EVE_ROUTINE_WAKE);
  linkUp(); /* reconnecting/linking never clears the stop */
  CHECK(eveSafetyStopLatched());
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0); /* no gesture while stopped */
  CHECK(evePeerEveStatus()->stop);

  const int moves0 = w.servoMoves;
  CHECK_EQ_STR(field(inject(MSG_MODE_ESCORT, "{}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_MODE_IDLE, "{}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_ATTACH_CONFIRMED, "{}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"play\"}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_MOVE_SERVO, "{\"l\":90}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_SET_EYES, "{\"e\":\"happy\"}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_PLAY_SOUND, "{\"track\":2}"), "err"), "stopped");
  CHECK_EQ_STR(field(inject(MSG_MODE_STANDBY, "{}"), "ok"), "true"); /* calmer modes allowed */
  CHECK(eveControlMode() == EVE_MODE_SLEEP);
  CHECK_EQ_STR(field(inject(MSG_MODE_DOCK, "{}"), "ok"), "true");
  CHECK(eveControlMode() == EVE_MODE_DOCKED);
  CHECK(w.servoMoves == moves0);

  // A repeated STOP re-asserts the halt.
  const int halts0 = w.haltsStop;
  inject(MSG_STOP, "{\"src\":\"cyd\"}");
  CHECK(w.haltsStop == halts0 + 1);

  // RESET_STATE restarts the handshake but keeps the latch.
  inject(MSG_RESET_STATE, "{}");
  CHECK(!eveControlLinked());
  run(1500);
  CHECK(eveControlLinked());
  CHECK(eveSafetyStopLatched());

  // CLEAR_STOP needs the current session.
  CHECK_EQ_STR(field(inject(MSG_CLEAR_STOP, "{}"), "err"), "session_required");
  CHECK_EQ_STR(field(inject(MSG_CLEAR_STOP, sessionJson(5).c_str()), "err"), "session");
  CHECK(eveSafetyStopLatched());
  CHECK(evePeerSendClearStop());
  run(100);
  CHECK(!eveSafetyStopLatched());
  CHECK(evePeerLastResult()->valid && evePeerLastResult()->ok);
  run(3000);
  CHECK(w.servoMoves == moves0); /* nothing resumes by itself after the clear */

  // Local (serial / web) stop and clear.
  eveControlLocalStop(EVE_STOP_SRC_SERIAL, w.now);
  CHECK(eveSafetyStopLatched());
  CHECK(eveSafetyStopSource() == EVE_STOP_SRC_SERIAL);
  CHECK(eveControlLocalClearStop(EVE_STOP_SRC_SERIAL, w.now));
  CHECK(!eveControlLocalClearStop(EVE_STOP_SRC_SERIAL, w.now));

  // WALL-E's STOP goes out even without a link.
  World w2;
  newWorld(w2);
  CHECK(evePeerSendStop("cyd_estop"));
  run(50);
  CHECK(eveSafetyStopLatched());
}

static void test_cancelled_gesture() {
  g_test = "cancel";
  World w;
  newWorld(w);
  linkUp();
  run(5000);
  const int m0 = w.servoMoves;
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"play\"}"), "ok"), "true");
  CHECK(eveRoutineActive() == EVE_ROUTINE_PLAY);
  CHECK(w.servoMoves == m0 + 2); /* first pose: head + arm */
  run(600);
  CHECK(evePeerSendStop("webui"));
  run(20);
  CHECK(eveRoutineActive() == EVE_ROUTINE_NONE);
  const int m1 = w.servoMoves;
  run(5000);
  CHECK(w.servoMoves == m1); /* the remaining steps never ran */
  CHECK(w.haltsStop >= 1);
  CHECK(w.soundStops >= 1);

  // A manual move replaces a running gesture.
  CHECK(evePeerSendClearStop());
  run(50);
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"protect\"}"), "ok"), "true");
  CHECK(eveRoutineActive() == EVE_ROUTINE_PROTECT);
  inject(MSG_MOVE_SERVO, "{\"l\":95,\"r\":95}");
  CHECK(eveRoutineActive() == EVE_ROUTINE_NONE);
}

static void test_transitions() {
  g_test = "transitions";
  World w;
  newWorld(w);
  linkUp();
  run(5000);

  // Interaction
  const uint32_t greet0 = eveRoutineStartCount(EVE_ROUTINE_GREET);
  CHECK(evePeerSendMode(MSG_MODE_INTERACT));
  run(50);
  CHECK(eveControlMode() == EVE_MODE_INTERACT);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_GREET) == greet0 + 1);
  CHECK(evePeerSendMode(MSG_MODE_INTERACT));
  run(50);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_GREET) == greet0 + 1); /* once per event */
  run(5000);

  // Escort
  const uint32_t esc0 = eveRoutineStartCount(EVE_ROUTINE_ESCORT);
  CHECK(evePeerSendMode(MSG_MODE_ESCORT));
  run(50);
  CHECK(eveControlMode() == EVE_MODE_ESCORT);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_ESCORT) == esc0 + 1);
  run(3000);

  // Standby and wake
  const int sleeps0 = countSent(w.eveSent, MSG_EVE_SLEEP);
  CHECK(evePeerSendMode(MSG_MODE_STANDBY));
  run(50);
  CHECK(eveControlMode() == EVE_MODE_SLEEP);
  CHECK(countSent(w.eveSent, MSG_EVE_SLEEP) == sleeps0 + 1);
  CHECK(eveControlLinked()); /* heartbeats continue in standby */
  run(6000);
  CHECK(eveControlLinked());
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"scan\"}"), "err"), "standby");
  const uint32_t wake0 = eveRoutineStartCount(EVE_ROUTINE_WAKE);
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"wake\"}"), "ok"), "true");
  CHECK(eveControlMode() == EVE_MODE_IDLE);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0 + 1);
  run(5000);

  // Battery warning: once per descent, glitches do not repeat it
  const uint32_t warn0 = eveRoutineStartCount(EVE_ROUTINE_LOW_WARN);
  eveControlSetBattery(EVE_BATLVL_WARN, -1, false, w.now);
  eveControlSetBattery(EVE_BATLVL_UNKNOWN, -1, false, w.now);
  eveControlSetBattery(EVE_BATLVL_WARN, -1, false, w.now);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOW_WARN) == warn0 + 1);
  run(3000);
  eveControlSetBattery(EVE_BATLVL_OK, -1, false, w.now);
  eveControlSetBattery(EVE_BATLVL_WARN, -1, false, w.now);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOW_WARN) == warn0 + 2);
  run(4000);

  // Critical battery -> LOW_POWER, motion refused, WALL-E told
  const uint32_t lp0 = eveRoutineStartCount(EVE_ROUTINE_LOW_POWER);
  eveControlSetBattery(EVE_BATLVL_CRITICAL, -1, false, w.now);
  run(50);
  CHECK(eveControlMode() == EVE_MODE_LOW_POWER);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOW_POWER) == lp0 + 1);
  CHECK(evePeerStats()->lowBatteryEvents >= 1);
  CHECK_EQ_STR(field(inject(MSG_MOVE_SERVO, "{\"l\":90}"), "err"), "low_power");
  CHECK_EQ_STR(field(inject(MSG_MODE_ESCORT, "{}"), "err"), "low_power");
  CHECK_EQ_STR(field(inject(MSG_RUN_ROUTINE, "{\"r\":\"play\"}"), "err"), "low_power");
  const uint32_t lowEvents = evePeerStats()->lowBatteryEvents;
  run(21000);
  CHECK(evePeerStats()->lowBatteryEvents >= lowEvents + 1); /* repeated report while critical */
  // Docking is allowed and wins the mode; motion stays inhibited.
  CHECK_EQ_STR(field(inject(MSG_MODE_DOCK, "{}"), "ok"), "true");
  CHECK(eveControlMode() == EVE_MODE_DOCKED);
  CHECK_EQ_STR(field(inject(MSG_MOVE_SERVO, "{\"l\":90}"), "err"), "low_power");
  CHECK_EQ_STR(field(inject(MSG_MODE_IDLE, "{}"), "ok"), "true");
  CHECK(eveControlMode() == EVE_MODE_LOW_POWER);
  // An invalid reading is not a recovery.
  eveControlSetBattery(EVE_BATLVL_UNKNOWN, -1, false, w.now);
  run(50);
  CHECK(eveControlMode() == EVE_MODE_LOW_POWER);
  eveControlSetBattery(EVE_BATLVL_OK, -1, false, w.now);
  run(50);
  CHECK(eveControlMode() == EVE_MODE_IDLE);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_LOW_POWER) == lp0 + 1);

  // Measured full charge -> one reaction
  const uint32_t full0 = eveRoutineStartCount(EVE_ROUTINE_FULL);
  eveControlSetBattery(EVE_BATLVL_OK, 1, true, w.now);
  eveControlSetBattery(EVE_BATLVL_OK, 1, true, w.now);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_FULL) == full0 + 1);
  run(3000);

  // Blocking fault -> ERROR (still linked, still reporting), then back
  eveSafetySetFault(EVE_FAULT_SERVO_DRIVER, true);
  run(50);
  CHECK(eveControlMode() == EVE_MODE_ERROR);
  CHECK_EQ_STR(field(inject(MSG_MOVE_SERVO, "{\"l\":90}"), "err"), "fault");
  run(1000);
  CHECK(eveControlLinked());
  CHECK(evePeerEveStatus()->faults & EVE_FAULT_SERVO_DRIVER);
  eveSafetySetFault(EVE_FAULT_SERVO_DRIVER, false);
  run(50);
  CHECK(eveControlMode() == EVE_MODE_IDLE);

  // Link loss while escorting
  CHECK(evePeerSendMode(MSG_MODE_ESCORT));
  run(100);
  w.cutToEve = w.cutToWallE = true;
  run(2500);
  CHECK(eveControlMode() == EVE_MODE_WAIT_ATTACH);
  w.cutToEve = w.cutToWallE = false;
  run(1500);
  CHECK(eveControlMode() == EVE_MODE_IDLE); /* reconnect does not resume escort */
}

static void test_legacy_peer_and_mimic() {
  g_test = "legacy";
  World w;
  newWorld(w);
  w.walleRunning = false; /* play an older WALL-E / dock by hand */
  run(600);
  pushFrame(w.toEve, MSG_WALL_E_ACK, 1, "{\"session\":4242,\"peer\":\"eve_dock_c3\"}");
  run(20);
  CHECK(eveControlLinked());
  CHECK(!eveControlPeerHeartbeats());
  CHECK_EQ_STR(eveControlPeer(), "eve_dock_c3");
  run(10000); /* legacy rule: IDLE is not supervised */
  CHECK(eveControlLinked());
  pushFrame(w.toEve, MSG_MODE_ESCORT, 2, "{}");
  run(7900);
  CHECK(eveControlLinked());
  run(300);
  CHECK(!eveControlLinked()); /* 8 s rule in ESCORT */

  // Bad ACKs are ignored
  pushFrame(w.toEve, MSG_WALL_E_ACK, 3, "{\"session\":0}");
  pushFrame(w.toEve, MSG_WALL_E_ACK, 4, "");
  pushFrame(w.toEve, MSG_WALL_E_ACK, 5, "{\"session\":\"x\"}");
  run(20);
  CHECK(!eveControlLinked());

  // Dock mimic (bench)
  eveControlSetDockMimic(true, true, w.now);
  run(20);
  CHECK(eveControlMode() == EVE_MODE_DOCKED);
  CHECK(eveControlDockMimicCharging());
  run(15000);
  CHECK(eveControlMode() == EVE_MODE_DOCKED); /* not supervised */
  eveControlSetDockMimic(false, false, w.now);
  run(20);
  CHECK(eveControlMode() == EVE_MODE_WAIT_ATTACH);
}

static void test_restarts() {
  g_test = "restarts";
  World w;
  newWorld(w);
  linkUp();
  const uint32_t s0 = eveControlSession();
  const uint32_t wake0 = eveRoutineStartCount(EVE_ROUTINE_WAKE);
  // WALL-E reboots: it adopts EVE's running session, EVE never drops.
  initWallE();
  run(1000);
  CHECK(evePeerLinked());
  CHECK(evePeerSession() == s0);
  CHECK(eveControlLinked());
  CHECK(eveControlStats()->linkLosses == 0);
  CHECK(eveRoutineStartCount(EVE_ROUTINE_WAKE) == wake0);
  // EVE reboots: she says HELLO, WALL-E hands out a fresh session.
  initEve();
  run(1500);
  CHECK(eveControlLinked());
  CHECK(evePeerLinked());
  CHECK(eveControlSession() == evePeerSession());
  CHECK(eveControlSession() != s0);
}

static void test_battery_logic() {
  g_test = "battery";
  EveBatteryConfig c = {6.0f, 8.4f, 5.0f, 8.8f, 25, 15, 4, 0, 0.05f, 0.10f, 1.0f, 3};
  EveBatteryState s;
  eveBatteryLogicInit(&s);
  CHECK(s.level == EVE_BATLVL_UNKNOWN);
  eveBatteryLogicUpdate(&s, &c, 8.0f, true, -0.4f, true);
  CHECK(s.valid && s.level == EVE_BATLVL_OK);
  CHECK(s.charging == -1); /* direction not configured -> unknown, never guessed */
  // Implausible readings are invalid, not "empty": the level holds for 2 samples, then UNKNOWN.
  eveBatteryLogicUpdate(&s, &c, 0.1f, true, 0.f, true);
  CHECK(!s.valid && s.level == EVE_BATLVL_OK);
  eveBatteryLogicUpdate(&s, &c, 12.0f, true, 0.f, true);
  CHECK(!s.valid && s.level == EVE_BATLVL_OK);
  eveBatteryLogicUpdate(&s, &c, 7.0f, false, 0.f, false);
  CHECK(!s.valid && s.level == EVE_BATLVL_UNKNOWN);
  // Hysteresis: 25 % warn, 15 % crit, recover above +4 %
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.20f, true, 0.f, true); /* 20 % */
  CHECK(s.level == EVE_BATLVL_WARN);
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.27f, true, 0.f, true); /* 27 %: inside band */
  CHECK(s.level == EVE_BATLVL_WARN);
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.31f, true, 0.f, true); /* 31 % */
  CHECK(s.level == EVE_BATLVL_OK);
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.10f, true, 0.f, true);
  CHECK(s.level == EVE_BATLVL_CRITICAL);
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.17f, true, 0.f, true); /* 17 %: still critical */
  CHECK(s.level == EVE_BATLVL_CRITICAL);
  eveBatteryLogicUpdate(&s, &c, 6.0f + 2.4f * 0.21f, true, 0.f, true);
  CHECK(s.level == EVE_BATLVL_WARN);
  // With a configured charge direction (-1 = charging reads negative)
  c.chargeSign = -1;
  eveBatteryLogicInit(&s);
  eveBatteryLogicUpdate(&s, &c, 8.0f, true, -0.8f, true);
  CHECK(s.charging == 1 && !s.full);
  eveBatteryLogicUpdate(&s, &c, 8.38f, true, -0.07f, true);
  CHECK(s.charging == 1 && s.full);
  eveBatteryLogicUpdate(&s, &c, 8.36f, true, 0.0f, true);
  CHECK(s.charging == 0 && s.full); /* charger finished: full stays */
  eveBatteryLogicUpdate(&s, &c, 8.2f, true, 0.6f, true);
  CHECK(s.charging == 0 && !s.full); /* discharging again */
  eveBatteryLogicUpdate(&s, &c, 8.0f, true, -0.8f, false);
  CHECK(s.charging == -1); /* no current reading -> unknown */
  // EMA smooths a single sag
  c.alpha = 0.35f;
  eveBatteryLogicInit(&s);
  eveBatteryLogicUpdate(&s, &c, 8.0f, true, 0.f, true);
  eveBatteryLogicUpdate(&s, &c, 6.2f, true, 0.f, true);
  CHECK(s.vFilt > 7.2f && s.level == EVE_BATLVL_OK);
}

static void test_pin_check() {
  g_test = "pins";
  char msg[128];
  EvePinUse ok[] = {{"uart tx", 17, 0}, {"uart rx", 18, 0}, {"ina sda", 8, EVE_PINGRP_I2C_SDA},
                    {"tof sda", 8, EVE_PINGRP_I2C_SDA}, {"rgb", 48, 0}, {"unused", -1, 0}};
  CHECK(evePinCheck(ok, 6, msg, sizeof(msg)) == 0);
  EvePinUse clash[] = {{"uart tx", 17, 0}, {"head servo", 17, 0}};
  CHECK(evePinCheck(clash, 2, msg, sizeof(msg)) == 1);
  CHECK(std::string(msg).find("GPIO17") != std::string::npos);
  EvePinUse mixed[] = {{"ina sda", 8, EVE_PINGRP_I2C_SDA}, {"strip", 8, 0}};
  CHECK(evePinCheck(mixed, 2, msg, sizeof(msg)) == 1);
  EvePinUse reserved[] = {{"strip", 35, 0}, {"servo", 27, 0}, {"ghost", 23, 0}, {"too big", 49, 0}};
  CHECK(evePinCheck(reserved, 4, msg, sizeof(msg)) == 4);
}

static void test_expression_names() {
  g_test = "names";
  CHECK(eveExprFromName("happy") == 9);
  CHECK(eveExprFromName("searching") == 21);
  CHECK(eveExprFromName("nope") == -1);
  CHECK(eveGlowFromName("standby") == EVE_GLOW_STANDBY_BEACON);
  CHECK(eveRoutineOperatorAllowed(eveRoutineFromName("scan")));
  CHECK(!eveRoutineOperatorAllowed(eveRoutineFromName("low_power")));
  CHECK(eveRoutineIsTheatrical(EVE_ROUTINE_SCAN));
}

int main(int argc, char** argv) {
  const char* only = nullptr;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-v")) g_verbose = true;
    else only = argv[i]; /* run only tests whose name contains this text */
  }
  struct T { const char* name; void (*fn)(); } tests[] = {
      {"codec", test_codec},
      {"handshake + heartbeats", test_handshake_and_heartbeats},
      {"link loss + reconnect", test_link_loss_and_reconnect},
      {"noise / echo cannot keep the link", test_noise_and_echo_do_not_keep_link},
      {"stale session rejected", test_stale_session_rejected},
      {"malformed commands", test_malformed_commands},
      {"stop latch", test_stop_latch},
      {"cancelled gestures", test_cancelled_gesture},
      {"state transitions", test_transitions},
      {"legacy peer + dock mimic", test_legacy_peer_and_mimic},
      {"board restarts", test_restarts},
      {"battery evaluation", test_battery_logic},
      {"pin check", test_pin_check},
      {"name tables", test_expression_names},
  };
  for (auto& t : tests) {
    if (only && !strstr(t.name, only)) continue;
    const int before = g_failures;
    t.fn();
    std::printf("%-40s %s\n", t.name, g_failures == before ? "ok" : "FAILED");
  }
  std::printf("\n%d checks, %d failed (simulated: no hardware involved)\n", g_checks, g_failures);
  return g_failures ? 1 : 0;
}
