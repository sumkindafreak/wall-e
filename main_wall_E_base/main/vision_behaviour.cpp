/**
 * Vision Behaviour Engine
 * State machine + servo tracking from vision packets.
 */

#include "vision_behaviour.h"
#include "vision_protocol.h"
#include "servo_manager.h"
#include <Arduino.h>
#include <Preferences.h>

#define CAM_CENTER_X   80
#define VISION_PREFS_NS  "walle_cfg"
#define VISION_EN_KEY   "vis_en"
#define CAM_CENTER_Y   60
#define TRACKING_GAIN  0.05f
#define SERVO_SMOOTH   0.2f
#define SCAN_INACTIVITY_MS  8000
#define OCCLUSION_MS   500

static bool s_visionEnabled = false;
static VisionBehaviourState s_state = VBEHAVE_IDLE;
static uint32_t s_stateEnterMs = 0;
static uint32_t s_lastVisionMs = 0;
static uint32_t s_lastMotionMs = 0;

/* Cached latest packet for web API */
static VisionPacket_t s_lastPacket;
static bool s_hasPacket = false;

static float s_rotateTarget = 50.0f;   /* 0-100 */
static float s_tiltTarget = 50.0f;
static float s_neckTarget = 50.0f;

static uint8_t s_lastObjectClass = OBJ_CLASS_NONE;

static void setServoSmooth(uint8_t ch, float* current, float target) {
  float d = target - *current;
  *current += d * SERVO_SMOOTH;
  int pos = (int)constrain(*current, 0.0f, 100.0f);
  servoSet(ch, pos, 60);
}

static void doScanStep(uint32_t now) {
  uint32_t phase = (now - s_stateEnterMs) % 4000;
  if (phase < 1000) {
    s_rotateTarget = 25.0f;
    s_tiltTarget = 50.0f;
  } else if (phase < 2000) {
    s_rotateTarget = 75.0f;
    s_tiltTarget = 50.0f;
  } else if (phase < 3000) {
    s_rotateTarget = 50.0f;
    s_tiltTarget = 25.0f;
  } else {
    s_rotateTarget = 50.0f;
    s_tiltTarget = 50.0f;
  }
  servoSet(SERVO_HEAD_PAN, (int)s_rotateTarget, 30);
  servoSet(SERVO_NECK_TOP, (int)s_tiltTarget, 30);
}

void visionBehaviourInit(void) {
  Preferences prefs;
  prefs.begin(VISION_PREFS_NS, true);
  s_visionEnabled = prefs.getBool(VISION_EN_KEY, false);
  prefs.end();
  s_state = VBEHAVE_IDLE;
  s_stateEnterMs = millis();
  s_rotateTarget = 50.0f;
  s_tiltTarget = 50.0f;
  s_neckTarget = 50.0f;
  Serial.printf("[VisionBehave] Init (enabled=%s)\n", s_visionEnabled ? "yes" : "no");
}

void visionBehaviourOnPacket(const VisionPacket_t* pkt, size_t len) {
  if (!pkt || pkt->magic != VISION_MAGIC || len < 20) return;
  size_t copyLen = (len < sizeof(VisionPacket_t)) ? len : sizeof(VisionPacket_t);
  memcpy(&s_lastPacket, pkt, copyLen);
  if (copyLen < sizeof(VisionPacket_t)) memset((uint8_t*)&s_lastPacket + copyLen, 0, sizeof(VisionPacket_t) - copyLen);
  s_hasPacket = true;
  s_lastVisionMs = millis();
  if (pkt->motionDetected) s_lastMotionMs = millis();

  if (!s_visionEnabled) return;

  if (s_state == VBEHAVE_SCANNING && pkt->motionDetected) {
    s_state = VBEHAVE_CURIOUS;
    s_stateEnterMs = millis();
  }

  if (s_state == VBEHAVE_IDLE && pkt->motionDetected) {
    s_state = VBEHAVE_CURIOUS;
    s_stateEnterMs = millis();
  }

  if (s_state == VBEHAVE_CURIOUS && pkt->motionDetected) {
    s_state = VBEHAVE_TRACKING;
    s_stateEnterMs = millis();
  }

  if (pkt->objectClass == OBJ_CLASS_LARGE && pkt->motionDetected) {
    s_state = VBEHAVE_FOLLOWING;
    s_stateEnterMs = millis();
  }

  if (!pkt->motionDetected && (millis() - s_lastMotionMs) > OCCLUSION_MS) {
    if (s_state == VBEHAVE_TRACKING || s_state == VBEHAVE_FOLLOWING) {
      s_state = VBEHAVE_IDLE;
      s_stateEnterMs = millis();
    }
  }

  s_lastObjectClass = pkt->objectClass;

  if (s_state == VBEHAVE_TRACKING || s_state == VBEHAVE_FOLLOWING) {
    int errX = pkt->targetX - CAM_CENTER_X;
    int errY = pkt->targetY - CAM_CENTER_Y;
    float gain = TRACKING_GAIN;
    s_rotateTarget += errX * gain;
    s_tiltTarget += errY * gain;
    s_rotateTarget = constrain(s_rotateTarget, 10.0f, 90.0f);
    s_tiltTarget = constrain(s_tiltTarget, 20.0f, 80.0f);

    if (pkt->objectClass == OBJ_CLASS_LARGE) {
      s_neckTarget = 35.0f;
    } else if (pkt->objectClass == OBJ_CLASS_MEDIUM) {
      s_neckTarget = 42.0f;
    } else {
      s_neckTarget = 50.0f;
    }
  }
}

void visionBehaviourUpdate(uint32_t now) {
  if (!s_visionEnabled) return;
  if (s_state == VBEHAVE_IDLE && (now - s_lastMotionMs) > SCAN_INACTIVITY_MS) {
    s_state = VBEHAVE_SCANNING;
    s_stateEnterMs = now;
  }

  if (s_state == VBEHAVE_SCANNING) {
    if (now - s_lastMotionMs < 500) {
      s_state = VBEHAVE_IDLE;
      s_stateEnterMs = now;
    } else {
      doScanStep(now);
    }
    return;
  }

  if (s_state == VBEHAVE_TRACKING || s_state == VBEHAVE_FOLLOWING) {
    static float s_rotCur = 50.0f, s_tiltCur = 50.0f, s_neckCur = 50.0f;
    setServoSmooth(SERVO_HEAD_PAN, &s_rotCur, s_rotateTarget);
    setServoSmooth(SERVO_NECK_TOP, &s_tiltCur, s_tiltTarget);
    setServoSmooth(SERVO_NECK_BOT, &s_neckCur, s_neckTarget);
  }
}

VisionBehaviourState visionBehaviourGetState(void) {
  return s_state;
}

const char* visionBehaviourGetStateName(void) {
  switch (s_state) {
    case VBEHAVE_IDLE:      return "IDLE";
    case VBEHAVE_CURIOUS:   return "CURIOUS";
    case VBEHAVE_TRACKING:  return "TRACKING";
    case VBEHAVE_FOLLOWING: return "FOLLOWING";
    case VBEHAVE_SCANNING:  return "SCANNING";
    case VBEHAVE_STARTLED:  return "STARTLED";
    case VBEHAVE_DOCKING:   return "DOCKING";
    case VBEHAVE_SLEEP:     return "SLEEP";
    default:                return "?";
  }
}

String visionGetStatusJSON(void) {
  String json = "{";
  json += "\"connected\":"; json += s_hasPacket && (millis() - s_lastVisionMs < 2000) ? "true" : "false";
  json += ",\"state\":\""; json += visionBehaviourGetStateName();
  json += "\",\"motion\":"; json += s_hasPacket && s_lastPacket.motionDetected ? "true" : "false";
  json += ",\"targetX\":"; json += s_hasPacket ? (int)s_lastPacket.targetX : 0;
  json += ",\"targetY\":"; json += s_hasPacket ? (int)s_lastPacket.targetY : 0;
  json += ",\"objectSize\":"; json += s_hasPacket ? (uint32_t)s_lastPacket.objectSize : 0;
  json += ",\"objectClass\":"; json += s_hasPacket ? (int)s_lastPacket.objectClass : 0;
  if (s_hasPacket) {
    json += ",\"objectClassStr\":\"";
    if (s_lastPacket.objectClass == OBJ_CLASS_SMALL) json += "small";
    else if (s_lastPacket.objectClass == OBJ_CLASS_MEDIUM) json += "medium";
    else if (s_lastPacket.objectClass == OBJ_CLASS_LARGE) json += "large";
    else json += "none";
    json += "\"";
  } else {
    json += ",\"objectClassStr\":\"none\"";
  }
  json += "\",\"bboxW\":"; json += s_hasPacket ? (int)s_lastPacket.bboxWidth : 0;
  json += ",\"bboxH\":"; json += s_hasPacket ? (int)s_lastPacket.bboxHeight : 0;
  json += ",\"frameID\":"; json += s_hasPacket ? (uint32_t)s_lastPacket.frameID : 0;
  json += ",\"ageMs\":"; json += s_hasPacket ? (uint32_t)(millis() - s_lastVisionMs) : 9999;
  if (s_hasPacket && s_lastPacket.visionNodeIp != 0) {
    uint32_t ip = s_lastPacket.visionNodeIp;
    json += ",\"visionNodeIp\":\""; json += (int)((ip >> 24) & 0xFF); json += "."; json += (int)((ip >> 16) & 0xFF); json += "."; json += (int)((ip >> 8) & 0xFF); json += "."; json += (int)(ip & 0xFF); json += "\"";
  } else {
    json += ",\"visionNodeIp\":null";
  }
  json += ",\"enabled\":";
  json += s_visionEnabled ? "true" : "false";
  json += "}";
  return json;
}

void visionSetEnabled(bool en) {
  s_visionEnabled = en;
  Preferences prefs;
  prefs.begin(VISION_PREFS_NS, false);
  prefs.putBool(VISION_EN_KEY, en);
  prefs.end();
}

bool visionIsEnabled(void) {
  return s_visionEnabled;
}
