#include "eve_desktop_companion.h"

#include "audio_control.h"
#include "config.h"
#include "eyes_control.h"
#include "mic_input.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "uart_link.h"
#include "../../firmware_common/include/eve_personality.h"

#if EVE_ENABLE_EYES
#include "eve_expression_state.h"
#endif

#if EVE_ENABLE_TOF
#include "eve_target_tracker.h"
#include "eve_tof_manager.h"
#endif

#include <ArduinoJson.h>
#include <string.h>

static bool s_active = false;
static bool s_charging = false;
static EvePersonalityEngine s_personality;
static uint32_t s_lastMicReactionMs = 0;

static void reactToMicEvent_(uint32_t nowMs, EveMicEvent event);

static void requestExpression_(uint8_t legacyMode) {
  eyesSetMode(legacyMode);
}

static void setGaze_(float gx, float gy) {
#if EVE_ENABLE_EYES
  eveExpressionSetTargetGaze(gx, gy);
#else
  (void)gx;
  (void)gy;
#endif
}

static void requestDockExpression_(uint32_t nowMs, bool interacting) {
#if EVE_ENABLE_EYES
  if (interacting) {
    eveExpressionRequest(EVE_EXPR_HAPPY, 1800u);
  } else if ((nowMs / 7000u) & 1u) {
    eveExpressionRequest(EVE_EXPR_AFFECTION, 1800u);
  } else {
    eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 1800u);
  }
#else
  (void)nowMs;
  (void)interacting;
#endif
}

static void personalityEyes_(uint8_t mode) {
  requestExpression_(mode);
}

static void personalityGlow_(uint8_t pattern) {
  neopixelSetPattern(pattern);
}

static void personalityServo_(int16_t headPanDeg, int16_t rightArmDeg) {
  float gazeX = 0.0f;
  if (headPanDeg < 80) {
    gazeX = -0.55f;
  } else if (headPanDeg > 100) {
    gazeX = 0.55f;
  }
  setGaze_(gazeX, -0.08f);
  servoSetHeadPanTarget(headPanDeg);
  servoSetRightArmTarget(rightArmDeg);
}

static void personalityAudio_(uint8_t track) {
  audioPlayTrack(track);
}

void eveDesktopCompanionInit(void) {
  s_active = false;
  s_charging = false;
  EvePersonalityHardware hw;
  hw.setEyes = personalityEyes_;
  hw.setGlow = personalityGlow_;
  hw.setServo = personalityServo_;
  hw.playAudio = personalityAudio_;
  s_personality.begin(hw);
}

void eveDesktopCompanionSetActive(bool active, bool charging) {
  if (s_active == active && s_charging == charging) {
    return;
  }

  s_active = active;
  s_charging = charging;
  const uint32_t now = millis();

  if (s_active) {
    Serial.println(F("[EVE][DESK] desktop companion active"));
    eyesNotifyDockingState(true, s_charging);
    requestDockExpression_(now, false);
    setGaze_(0.0f, 0.0f);
    servoSetHeadPanTarget(90);
    servoSetRightArmTarget(88);
    neopixelSetPattern(s_charging ? 4u : 1u);
  } else {
    Serial.println(F("[EVE][DESK] desktop companion inactive"));
    eyesNotifyDockingState(false, false);
    setGaze_(0.0f, 0.0f);
    servoSetHeadPanTarget(90);
    servoSetRightArmTarget(90);
  }
  s_personality.setActive(s_active, s_charging, now);
}

bool eveDesktopCompanionIsActive(void) {
  return s_active;
}

bool eveDesktopCompanionApplyConfigJson(const char* json) {
  if (!json || !*json) {
    return false;
  }
  StaticJsonDocument<192> doc;
  if (deserializeJson(doc, json)) {
    return false;
  }
  const char* cmd = doc["cmd"] | "";
  if (strcmp(cmd, "mic_settings") == 0) {
    EveMicSettings settings = micGetSettings();
    settings.reactionsEnabled = doc["enabled"] | settings.reactionsEnabled;
    settings.spikeThreshold = doc["spike"] | settings.spikeThreshold;
    settings.clapThreshold = doc["clap"] | settings.clapThreshold;
    settings.quietThreshold = doc["quiet"] | settings.quietThreshold;
    settings.reactionCooldownMs = doc["cooldown"] | settings.reactionCooldownMs;
    micSetSettings(settings);
    Serial.println(F("[EVE][DESK] mic config updated"));
    return true;
  }
  if (strcmp(cmd, "mic_test") == 0) {
    reactToMicEvent_(millis(), EVE_MIC_EVENT_CLAP);
    Serial.println(F("[EVE][DESK] mic test command applied"));
    return true;
  }
  if (strcmp(cmd, "personality") != 0) {
    return false;
  }
  EvePersonalityConfig cfg = s_personality.config();
  cfg.curiosity = doc["curiosity"] | cfg.curiosity;
  cfg.comfort = doc["comfort"] | cfg.comfort;
  cfg.excitement = doc["excitement"] | cfg.excitement;
  cfg.sleepiness = doc["sleepiness"] | cfg.sleepiness;
  cfg.responsiveness = doc["responsiveness"] | cfg.responsiveness;
  cfg.activity = doc["activity"] | cfg.activity;
  s_personality.setConfig(cfg);
  Serial.println(F("[EVE][DESK] personality config updated"));
  return true;
}

static void sendMicEvent_(EveMicEvent event) {
  if (event == EVE_MIC_EVENT_NONE) return;
  char json[160];
  snprintf(json, sizeof(json),
           "{\"event\":\"%s\",\"level\":%.1f,\"ambient\":%.1f,\"quiet\":%s}",
           micEventName(event), getMicLevel(), getMicAmbientAverage(), isRoomQuiet() ? "true" : "false");
  uartLinkSendJson(MSG_BUS_EVENT, json);
}

static void reactToMicEvent_(uint32_t nowMs, EveMicEvent event) {
  if (event == EVE_MIC_EVENT_NONE) return;
  if ((uint32_t)(nowMs - s_lastMicReactionMs) < 1800u) return;
  s_lastMicReactionMs = nowMs;
  sendMicEvent_(event);

  switch (event) {
    case EVE_MIC_EVENT_CLAP:
      requestDockExpression_(nowMs, true);
      setGaze_(0.0f, -0.16f);
      servoSetHeadPanTarget(90);
      servoSetRightArmTarget(140);
      neopixelSetPattern(5);
      audioPlayTrack(1);
      break;
    case EVE_MIC_EVENT_SOUND_SPIKE:
      requestDockExpression_(nowMs, true);
      setGaze_(0.35f, -0.10f);
      servoSetHeadPanTarget(108);
      servoSetRightArmTarget(104);
      neopixelSetPattern(5);
      break;
    case EVE_MIC_EVENT_QUIET_ROOM:
      requestDockExpression_(nowMs, false);
      setGaze_(0.0f, 0.08f);
      servoSetHeadPanTarget(90);
      servoSetRightArmTarget(76);
      neopixelSetPattern(s_charging ? 4u : 1u);
      break;
    case EVE_MIC_EVENT_LOUD_ENVIRONMENT:
    case EVE_MIC_EVENT_REPEATED_PATTERN:
      requestExpression_(2);
      setGaze_(0.0f, -0.22f);
      servoSetHeadPanTarget(90);
      servoSetRightArmTarget(70);
      neopixelSetPattern(3);
      break;
    default:
      break;
  }
}

static bool readNearInteraction_(uint32_t nowMs, float* gazeX, int16_t* panDeg) {
  if (gazeX) {
    *gazeX = 0.0f;
  }
  if (panDeg) {
    *panDeg = 90;
  }

#if EVE_ENABLE_TOF
  eveTofManagerPoll(nowMs);

  EveTofRawFrame raw;
  if (!eveTofManagerGetLastFrame(&raw)) {
    return false;
  }

  eveTargetTrackerUpdate(&raw, nowMs);
  EveTargetSnapshot snap;
  eveTargetTrackerGetSnapshot(&snap);

  const bool nearEnough = snap.distanceMm > 0 && snap.distanceMm <= 850;
  const bool confident = snap.confidencePct >= 25u;
  if (!nearEnough || !confident) {
    return false;
  }

  if (snap.zone == EVE_TARGET_MODEL_LEFT) {
    if (gazeX) *gazeX = -0.65f;
    if (panDeg) *panDeg = 72;
  } else if (snap.zone == EVE_TARGET_MODEL_RIGHT) {
    if (gazeX) *gazeX = 0.65f;
    if (panDeg) *panDeg = 108;
  } else {
    if (gazeX) *gazeX = 0.0f;
    if (panDeg) *panDeg = 90;
  }
  return true;
#else
  (void)nowMs;
  return false;
#endif
}

void eveDesktopCompanionTick(uint32_t nowMs) {
  if (!s_active) {
    return;
  }

  EvePersonalityInput input;
  input.charging = s_charging;
  input.docked = s_active;

  float gazeX = 0.0f;
  int16_t panDeg = 90;
  const bool interacting = readNearInteraction_(nowMs, &gazeX, &panDeg);
  if (interacting) {
    if (panDeg < 85) {
      input.tofLeftMm = 650;
    } else if (panDeg > 95) {
      input.tofRightMm = 650;
    } else {
      input.tofCenterMm = 650;
    }
  }
  s_personality.tick(nowMs, input);
  reactToMicEvent_(nowMs, micConsumeEvent());
}
