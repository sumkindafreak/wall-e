#include "config.h"
#include "eve_expression_state.h"
#include <string.h>

#if EVE_ENABLE_EYES

#include "battery_monitor.h"
#include "eve_attachment_manager.h"
#include <math.h>

static EveExpressionId s_autoExpr = EVE_EXPR_NEUTRAL_IDLE;
static EveExpressionId s_shotExpr = EVE_EXPR_NEUTRAL_IDLE;
static uint32_t s_shotUntil = 0;

static bool s_walleConn = false;
static bool s_search = false;
static bool s_track = false;
static bool s_sharedVb = false;
static bool s_docked = false;
static bool s_charging = false;
static uint32_t s_dockStillMs = 0;
static uint32_t s_lastDockChange = 0;
static uint32_t s_reconnectAffectMs = 0;
static uint32_t s_recordFailUntil = 0;

static float s_tgtGazeNx = 0.5f;
static float s_tgtGazeNy = 0.5f;
static uint8_t s_legacy = 0;
static EveExpressionId s_orchestrator = EVE_EXPR_NEUTRAL_IDLE;
static bool s_orchestratorActive = false;
static bool s_voiceActive = false;

static EveExpressionId pickAuto(uint32_t now) {
  if (s_recordFailUntil != 0 && now < s_recordFailUntil) {
    return EVE_EXPR_CONFUSED;
  }
  if (eveBatteryDataValid()) {
    if (eveBatteryStatus() == EVE_BAT_CRITICAL || eveBatteryPercent() <= EVE_BAT_CRIT_PCT) {
      return EVE_EXPR_LOW_BATTERY;
    }
  }
  if (s_search) {
    return EVE_EXPR_SEARCHING_FOR_WALLE;
  }
  if (s_track) {
    return EVE_EXPR_TRACK_TARGET;
  }
  if (s_docked && s_charging) {
    if (s_dockStillMs > 8000) {
      return EVE_EXPR_SLEEP;
    }
    return EVE_EXPR_SOFT_IDLE;
  }
  if (s_reconnectAffectMs != 0 && now < s_reconnectAffectMs) {
    return EVE_EXPR_AFFECTION;
  }
  if (!s_walleConn) {
    return EVE_EXPR_CONCERNED;
  }
  if (s_orchestratorActive) {
    return s_orchestrator;
  }
  if (eveAttachmentIsAttached()) {
    return EVE_EXPR_SOFT_IDLE;
  }
  if (s_legacy == 2) {
    return EVE_EXPR_ALERT;
  }
  if (s_legacy == 1) {
    return EVE_EXPR_HAPPY;
  }
  return EVE_EXPR_NEUTRAL_IDLE;
}

static void fillTargetFor(EveExpressionId e, EveEyeTarget* t) {
  memset(t, 0, sizeof(*t));
  t->eyeSep = 1.0f;
  t->eyeScaleX = 1.0f;
  t->eyeScaleY = 1.0f;
  t->glowOpa = 120.f;
  t->scanOpa = 25.f;
  t->lid = 0.f;
  t->tiltDeg = 0.f;
  t->squint = 0.f;

  switch (e) {
    case EVE_EXPR_NEUTRAL_IDLE:
      t->gazeX = 0.f;
      t->gazeY = 0.f;
      t->scanOpa = 28.f;
      break;
    case EVE_EXPR_SOFT_IDLE:
      t->gazeX = 0.f;
      t->gazeY = 0.05f;
      t->glowOpa = 140.f;
      t->eyeScaleY = 0.95f;
      t->scanOpa = 20.f;
      break;
    case EVE_EXPR_SLEEP:
      t->lid = 0.92f;
      t->glowOpa = 40.f;
      t->scanOpa = 10.f;
      t->eyeScaleY = 0.7f;
      break;
    case EVE_EXPR_WAKE:
      t->lid = 0.1f;
      t->glowOpa = 200.f;
      t->eyeScaleY = 1.05f;
      break;
    case EVE_EXPR_LOOK_LEFT:
      t->gazeX = -0.72f;
      t->gazeY = 0.f;
      break;
    case EVE_EXPR_LOOK_RIGHT:
      t->gazeX = 0.72f;
      t->gazeY = 0.f;
      break;
    case EVE_EXPR_LOOK_UP:
      t->gazeX = 0.f;
      t->gazeY = -0.55f;
      break;
    case EVE_EXPR_LOOK_DOWN:
      t->gazeX = 0.f;
      t->gazeY = 0.55f;
      break;
    case EVE_EXPR_TRACK_TARGET:
      t->gazeX = (s_tgtGazeNx - 0.5f) * 2.f;
      t->gazeY = (s_tgtGazeNy - 0.5f) * 2.f;
      t->glowOpa = 160.f;
      t->scanOpa = 35.f;
      break;
    case EVE_EXPR_HAPPY:
      t->eyeScaleX = 1.08f;
      t->eyeScaleY = 0.88f;
      t->glowOpa = 180.f;
      t->tiltDeg = -3.f;
      t->scanOpa = 18.f;
      break;
    case EVE_EXPR_CURIOUS:
      t->eyeSep = 1.06f;
      t->eyeScaleY = 1.12f;
      t->gazeX = 0.15f;
      t->glowOpa = 170.f;
      t->scanOpa = 40.f;
      break;
    case EVE_EXPR_SAD:
      t->eyeScaleY = 0.82f;
      t->eyeScaleX = 0.96f;
      t->tiltDeg = 4.f;
      t->squint = 0.18f;
      t->glowOpa = 75.f;
      t->gazeY = 0.18f;
      t->scanOpa = 14.f;
      break;
    case EVE_EXPR_ANGRY:
      t->eyeScaleX = 1.05f;
      t->eyeScaleY = 0.86f;
      t->squint = 0.55f;
      t->tiltDeg = -4.f;
      t->glowOpa = 200.f;
      t->scanOpa = 38.f;
      break;
    case EVE_EXPR_SLEEPY:
      t->lid = 0.35f;
      t->eyeScaleY = 0.84f;
      t->squint = 0.3f;
      t->glowOpa = 70.f;
      t->gazeY = 0.1f;
      t->scanOpa = 12.f;
      break;
    case EVE_EXPR_THINKING:
      t->gazeY = -0.35f;
      t->gazeX = 0.12f * sinf((float)millis() * 0.0015f);
      t->eyeScaleY = 0.94f;
      t->squint = 0.15f;
      t->glowOpa = 145.f;
      t->scanOpa = 32.f;
      break;
    case EVE_EXPR_SURPRISED:
      t->eyeScaleX = 1.14f;
      t->eyeScaleY = 1.18f;
      t->squint = 0.f;
      t->glowOpa = 210.f;
      t->scanOpa = 45.f;
      break;
    case EVE_EXPR_CONCERNED:
      t->tiltDeg = 5.f;
      t->squint = 0.25f;
      t->eyeSep = 0.92f;
      t->glowOpa = 90.f;
      t->scanOpa = 22.f;
      break;
    case EVE_EXPR_CONFUSED:
      t->tiltDeg = -6.f;
      t->eyeSep = 0.88f;
      t->eyeScaleX = 0.95f;
      t->glowOpa = 130.f;
      t->scanOpa = 48.f;
      break;
    case EVE_EXPR_AFFECTION:
      t->eyeScaleX = 1.12f;
      t->eyeScaleY = 0.9f;
      t->eyeSep = 0.96f;
      t->glowOpa = 210.f;
      t->tiltDeg = -2.f;
      t->scanOpa = 15.f;
      break;
    case EVE_EXPR_ALERT:
      t->eyeScaleY = 1.15f;
      t->squint = 0.05f;
      t->glowOpa = 220.f;
      t->scanOpa = 55.f;
      break;
    case EVE_EXPR_LOW_BATTERY:
      t->squint = 0.45f;
      t->glowOpa = 55.f;
      t->eyeScaleY = 0.78f;
      t->scanOpa = 12.f;
      t->gazeY = 0.08f;
      break;
    case EVE_EXPR_SEARCHING_FOR_WALLE:
      t->gazeX = 0.55f * sinf((float)millis() * 0.002f);
      t->gazeY = 0.12f * sinf((float)millis() * 0.0035f);
      t->scanOpa = 42.f;
      t->glowOpa = 150.f;
      break;
    default:
      break;
  }

  if (s_sharedVb) {
    t->glowOpa = fminf(255.f, t->glowOpa + 25.f);
    t->scanOpa = fminf(255.f, t->scanOpa + 8.f);
  }
  if (s_voiceActive) {
    t->eyeScaleY = fminf(1.22f, t->eyeScaleY * 1.06f);
    t->eyeScaleX = fminf(1.15f, t->eyeScaleX * 1.03f);
    t->squint *= 0.55f;
    t->lid *= 0.75f;
    t->glowOpa = fminf(255.f, t->glowOpa + 18.f);
  }
}

void eveExpressionInit(void) {
  s_autoExpr = EVE_EXPR_NEUTRAL_IDLE;
  s_shotExpr = EVE_EXPR_NEUTRAL_IDLE;
  s_shotUntil = 0;
  s_walleConn = false;
  s_search = false;
  s_track = false;
  s_sharedVb = false;
  s_docked = false;
  s_charging = false;
  s_dockStillMs = 0;
  s_lastDockChange = millis();
  s_reconnectAffectMs = 0;
  s_recordFailUntil = 0;
  s_legacy = 0;
  s_orchestrator = EVE_EXPR_NEUTRAL_IDLE;
  s_orchestratorActive = false;
  s_voiceActive = false;
}

void eveExpressionTick(uint32_t now) {
  static uint32_t s_last = 0;
  if (s_last == 0) {
    s_last = now;
  }
  uint32_t dt = now - s_last;
  s_last = now;

  bool dockChg = false;
  static bool prevDock = false;
  static bool prevChg = false;
  if (s_docked != prevDock || s_charging != prevChg) {
    dockChg = true;
    prevDock = s_docked;
    prevChg = s_charging;
    s_lastDockChange = now;
    s_dockStillMs = 0;
  } else {
    s_dockStillMs += dt;
  }

  (void)dockChg;

  s_autoExpr = pickAuto(now);
  if (s_shotUntil != 0 && now >= s_shotUntil) {
    s_shotUntil = 0;
  }
}

EveExpressionId eveExpressionGetCurrent(void) {
  uint32_t now = millis();
  if (s_shotUntil != 0 && now < s_shotUntil) {
    return s_shotExpr;
  }
  return s_autoExpr;
}

void eveExpressionGetTarget(EveEyeTarget* out) {
  if (!out) {
    return;
  }
  fillTargetFor(eveExpressionGetCurrent(), out);
}

void eveExpressionRequest(EveExpressionId id, uint32_t holdMs) {
  s_shotExpr = id;
  s_shotUntil = millis() + (holdMs ? holdMs : 2000);
  Serial.print(F("[EVE_FACE] Expression request -> "));
  Serial.println((int)id);
}

void eveExpressionNotifyWallEConnected(void) {
  s_walleConn = true;
  s_reconnectAffectMs = millis() + 2200;
  Serial.println(F("[EVE_FACE] WALL-E connected, affection response"));
}

void eveExpressionNotifyWallEDisconnected(void) {
  s_walleConn = false;
  s_reconnectAffectMs = 0;
  Serial.println(F("[EVE_FACE] WALL-E disconnected"));
}

void eveExpressionNotifyRecordFailure(void) {
  s_recordFailUntil = millis() + 2200;
  eveExpressionRequest(EVE_EXPR_CONFUSED, 1800);
  Serial.println(F("[EVE_FACE] Record unavailable -> confused"));
}

void eveExpressionNotifySharedVoicebox(bool on) {
  s_sharedVb = on;
  Serial.print(F("[EVE_FACE] Shared voicebox "));
  Serial.println(on ? F("on") : F("off"));
}

void eveExpressionSetTargetGaze(float nx, float ny) {
  s_tgtGazeNx = fminf(1.f, fmaxf(0.f, nx));
  s_tgtGazeNy = fminf(1.f, fmaxf(0.f, ny));
}

void eveExpressionSetSearchingWallE(bool on) {
  s_search = on;
}

void eveExpressionSetDockedCharging(bool docked, bool charging) {
  s_docked = docked;
  s_charging = charging;
}

void eveExpressionSetTracking(bool on) {
  s_track = on;
}

void eveExpressionSetLegacyMode(uint8_t mode) {
  s_legacy = mode;
}

void eveExpressionSetOrchestrator(EveExpressionId id) {
  s_orchestrator = id;
  s_orchestratorActive = true;
}

void eveExpressionSetVoiceActive(bool on) {
  s_voiceActive = on;
}

#if EVE_FACE_DEBUG_BENCH
const char* eveExpressionName(EveExpressionId id) {
  switch (id) {
    case EVE_EXPR_NEUTRAL_IDLE:
      return "NEUTRAL_IDLE";
    case EVE_EXPR_SOFT_IDLE:
      return "SOFT_IDLE";
    case EVE_EXPR_SLEEP:
      return "SLEEP";
    case EVE_EXPR_WAKE:
      return "WAKE";
    case EVE_EXPR_LOOK_LEFT:
      return "LOOK_LEFT";
    case EVE_EXPR_LOOK_RIGHT:
      return "LOOK_RIGHT";
    case EVE_EXPR_LOOK_UP:
      return "LOOK_UP";
    case EVE_EXPR_LOOK_DOWN:
      return "LOOK_DOWN";
    case EVE_EXPR_TRACK_TARGET:
      return "TRACK_TARGET";
    case EVE_EXPR_HAPPY:
      return "HAPPY";
    case EVE_EXPR_CURIOUS:
      return "CURIOUS";
    case EVE_EXPR_SAD:
      return "SAD";
    case EVE_EXPR_ANGRY:
      return "ANGRY";
    case EVE_EXPR_SLEEPY:
      return "SLEEPY";
    case EVE_EXPR_THINKING:
      return "THINKING";
    case EVE_EXPR_SURPRISED:
      return "SURPRISED";
    case EVE_EXPR_CONCERNED:
      return "CONCERNED";
    case EVE_EXPR_CONFUSED:
      return "CONFUSED";
    case EVE_EXPR_AFFECTION:
      return "AFFECTION";
    case EVE_EXPR_ALERT:
      return "ALERT";
    case EVE_EXPR_LOW_BATTERY:
      return "LOW_BATTERY";
    case EVE_EXPR_SEARCHING_FOR_WALLE:
      return "SEARCHING_FOR_WALLE";
    default:
      return "?";
  }
}

void eveExpressionDebugSerialPoll(void) {
  if (!Serial.available()) {
    return;
  }
  char c = (char)Serial.read();
  switch (c) {
    case '0':
      eveExpressionRequest(EVE_EXPR_NEUTRAL_IDLE, 5000);
      break;
    case '1':
      eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 5000);
      break;
    case '2':
      eveExpressionRequest(EVE_EXPR_SLEEP, 5000);
      break;
    case '3':
      eveExpressionRequest(EVE_EXPR_WAKE, 3000);
      break;
    case '4':
      eveExpressionRequest(EVE_EXPR_HAPPY, 4000);
      break;
    case '5':
      eveExpressionRequest(EVE_EXPR_CURIOUS, 4000);
      break;
    case '6':
      eveExpressionRequest(EVE_EXPR_CONFUSED, 4000);
      break;
    case '7':
      eveExpressionRequest(EVE_EXPR_AFFECTION, 4000);
      break;
    case '8':
      eveExpressionRequest(EVE_EXPR_ALERT, 4000);
      break;
    case '9':
      eveExpressionRequest(EVE_EXPR_LOW_BATTERY, 5000);
      break;
    case 'n':
      eveExpressionNotifyWallEConnected();
      break;
    case 'p':
      eveExpressionNotifyRecordFailure();
      break;
    case 's':
      eveExpressionSetSearchingWallE(true);
      break;
    case 'S':
      eveExpressionSetSearchingWallE(false);
      break;
    case 'h':
      eveExpressionSetDockedCharging(true, true);
      break;
    case 'c':
      eveExpressionSetDockedCharging(false, false);
      break;
    case 'l':
      eveExpressionRequest(EVE_EXPR_LOOK_LEFT, 2500);
      break;
    case 'r':
      eveExpressionRequest(EVE_EXPR_LOOK_RIGHT, 2500);
      break;
    case 'u':
      eveExpressionRequest(EVE_EXPR_LOOK_UP, 2500);
      break;
    case 'd':
      eveExpressionRequest(EVE_EXPR_LOOK_DOWN, 2500);
      break;
    case 'w':
      eveExpressionNotifyWallEDisconnected();
      break;
    case '?':
      Serial.println(F("[EVE_FACE] bench: 0-9 expr, n=reconnect, w=disc, p=rec fail, s/S search, h=dock+chg, c=undock"));
      break;
    default:
      break;
  }
}
#endif /* EVE_FACE_DEBUG_BENCH */

#else /* !EVE_ENABLE_EYES */

void eveExpressionInit(void) {}
void eveExpressionTick(uint32_t) {}
EveExpressionId eveExpressionGetCurrent(void) {
  return EVE_EXPR_NEUTRAL_IDLE;
}
void eveExpressionGetTarget(EveEyeTarget* out) {
  if (out) {
    memset(out, 0, sizeof(*out));
  }
}
void eveExpressionRequest(EveExpressionId, uint32_t) {}
void eveExpressionNotifyWallEConnected(void) {}
void eveExpressionNotifyWallEDisconnected(void) {}
void eveExpressionNotifyRecordFailure(void) {}
void eveExpressionNotifySharedVoicebox(bool) {}
void eveExpressionSetTargetGaze(float, float) {}
void eveExpressionSetSearchingWallE(bool) {}
void eveExpressionSetDockedCharging(bool, bool) {}
void eveExpressionSetTracking(bool) {}
void eveExpressionSetLegacyMode(uint8_t) {}
void eveExpressionSetOrchestrator(EveExpressionId) {}
void eveExpressionSetVoiceActive(bool) {}
#if EVE_FACE_DEBUG_BENCH
void eveExpressionDebugSerialPoll(void) {}
const char* eveExpressionName(EveExpressionId) {
  return "-";
}
#endif

#endif /* EVE_ENABLE_EYES */
