// ============================================================
// WALL-E eye laser — GPIO + PCA9685 aim (pan / tilt)
// ============================================================

#include "laser_control.h"
#include "servo_manager.h"
#include "ledc_compat.h"
#include <Arduino.h>
#include <cmath>

static uint8_t s_brightness = 0;
static bool s_laserEmitting = false;
static unsigned long s_laserOnSince = 0;

static float s_smoothPan = 50.0f;
static float s_smoothTilt = 50.0f;
static int s_tgtPan = 50;
static int s_tgtTilt = 50;
static bool s_smoothActive = false;
static uint16_t s_smoothPeriodMs = 30;
static unsigned long s_lastSmoothStepMs = 0;

enum FirePhase : uint8_t { FIRE_IDLE = 0, FIRE_SETTLE, FIRE_ON };
static FirePhase s_firePhase = FIRE_IDLE;
static unsigned long s_fireT0 = 0;
static uint32_t s_fireDuration = 0;
static int s_firePan = 50;
static int s_fireTilt = 50;

static bool s_scanOn = false;
static int8_t s_mood = 0;
static float s_scanAngle = 50.0f;
static int s_scanDir = 1;
static unsigned long s_lastScanMs = 0;

static int clamp100(int v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return v;
}

static void writeLaserPwm(uint8_t duty) {
  (void)walleLedcWrite(LASER_PIN, WALLE_LEDC_CH_LASER, duty);
}

void laserInit() {
  randomSeed(millis() ^ (uint32_t)micros());
  if (!walleLedcAttach(LASER_PIN,
                       WALLE_LEDC_CH_LASER,
                       LASER_PWM_FREQ,
                       LASER_PWM_RES)) {
    Serial.println(F("[LASER] ERROR: LEDC attach failed"));
  }
  writeLaserPwm(0);
  s_brightness = 0;
  s_laserEmitting = false;
  s_smoothPan = s_smoothTilt = 50.0f;
  s_tgtPan = s_tgtTilt = 50;
  s_smoothActive = false;
  s_firePhase = FIRE_IDLE;
  s_scanOn = false;
  Serial.printf("[LASER] Init GPIO %d, OFF\n", LASER_PIN);
}

static void applyLaserOutput() {
  writeLaserPwm(s_brightness);
  const bool on = s_brightness > 0;
  if (on && !s_laserEmitting) {
    s_laserEmitting = true;
    s_laserOnSince = millis();
    Serial.println(F("[LASER] emitting"));
  } else if (!on && s_laserEmitting) {
    s_laserEmitting = false;
    Serial.println(F("[LASER] dark"));
  }
}

void laserOn() {
  s_brightness = 255;
  applyLaserOutput();
}

void laserOff() {
  s_brightness = 0;
  applyLaserOutput();
}

void laserSetBrightness(uint8_t value) {
  s_brightness = value;
  applyLaserOutput();
}

static void aimServosNoLog(int pan, int tilt) {
  servoSet((uint8_t)SERVO_PAN_CHANNEL, clamp100(pan), SERVO_FAST_SPEED);
  servoSet((uint8_t)SERVO_TILT_CHANNEL, clamp100(tilt), SERVO_FAST_SPEED);
}

void laserAim(int pan, int tilt) {
  s_scanOn = false;
  aimServosNoLog(pan, tilt);
  Serial.printf("[LASER] Aim PAN:%d TILT:%d\n", clamp100(pan), clamp100(tilt));
}

void laserFire(int pan, int tilt, uint32_t durationMs) {
  if (s_firePhase != FIRE_IDLE) {
    Serial.println(F("[LASER] Fire ignored (sequence active)"));
    return;
  }
  s_scanOn = false;
  s_firePan = clamp100(pan);
  s_fireTilt = clamp100(tilt);
  s_fireDuration = durationMs ? durationMs : 500u;
  aimServosNoLog(s_firePan, s_fireTilt);
  s_firePhase = FIRE_SETTLE;
  s_fireT0 = millis();
}

void laserSmoothSetTarget(int pan, int tilt, uint16_t stepPeriodMs) {
  s_scanOn = false;
  s_smoothPan = (float)servoGetPos(SERVO_PAN_CHANNEL);
  s_smoothTilt = (float)servoGetPos(SERVO_TILT_CHANNEL);
  s_tgtPan = clamp100(pan);
  s_tgtTilt = clamp100(tilt);
  s_smoothPeriodMs = stepPeriodMs < 5 ? 5 : stepPeriodMs;
  s_smoothActive = true;
}

void laserScanSetEnabled(bool on) {
  if (s_scanOn == on) return;
  s_scanOn = on;
}

void laserSetMoodMode(int8_t mood) {
  s_mood = constrain((int)mood, 0, 2);
}

static void updateLaserSafety(uint32_t now) {
  if (!s_laserEmitting || s_brightness == 0) return;
  if (now - s_laserOnSince > LASER_TIMEOUT_MS) {
    laserOff();
    Serial.println(F("[LASER] AUTO SHUTOFF (timeout)"));
  }
}

static void updateFireMachine(uint32_t now) {
  switch (s_firePhase) {
    case FIRE_IDLE:
      return;
    case FIRE_SETTLE:
      if (now - s_fireT0 >= LASER_SETTLE_MS) {
        laserOn();
        s_firePhase = FIRE_ON;
        s_fireT0 = now;
      }
      break;
    case FIRE_ON:
      if (now - s_fireT0 >= s_fireDuration) {
        laserOff();
        s_firePhase = FIRE_IDLE;
      }
      break;
  }
}

static void updateSmooth(uint32_t now) {
  if (!s_smoothActive || now - s_lastSmoothStepMs < s_smoothPeriodMs) return;
  s_lastSmoothStepMs = now;

  bool moved = false;
  if (s_smoothPan < (float)s_tgtPan) {
    s_smoothPan += 1.0f;
    moved = true;
  } else if (s_smoothPan > (float)s_tgtPan) {
    s_smoothPan -= 1.0f;
    moved = true;
  }
  if (s_smoothTilt < (float)s_tgtTilt) {
    s_smoothTilt += 1.0f;
    moved = true;
  } else if (s_smoothTilt > (float)s_tgtTilt) {
    s_smoothTilt -= 1.0f;
    moved = true;
  }

  if (moved) {
    aimServosNoLog((int)(s_smoothPan + 0.5f),
                   (int)(s_smoothTilt + 0.5f));
  } else {
    s_smoothActive = false;
  }
}

static uint16_t scanPeriodMsForMood() {
  switch (s_mood) {
    case 1: return 18;
    case 2: return 35;
    default: return 55;
  }
}

static void updateScan(uint32_t now) {
  if (!s_scanOn) return;
  const uint16_t period = scanPeriodMsForMood();
  if (now - s_lastScanMs < period) return;
  s_lastScanMs = now;

  const float step = (s_mood == 1) ? 4.0f : (s_mood == 2 ? 2.2f : 1.2f);
  s_scanAngle += (float)s_scanDir * step;
  if (s_mood == 1) s_scanAngle += (float)random(-3, 4);

  if (s_scanAngle >= 85.0f) {
    s_scanAngle = 85.0f;
    s_scanDir = -1;
  } else if (s_scanAngle <= 15.0f) {
    s_scanAngle = 15.0f;
    s_scanDir = 1;
  }

  int tilt = 50;
  if (s_mood == 2) {
    tilt = constrain(42 + (int)(8.0f * sinf((float)now * 0.003f)), 35, 65);
  }
  aimServosNoLog((int)(s_scanAngle + 0.5f), tilt);
}

void laserUpdate(uint32_t now) {
  updateLaserSafety(now);
  updateFireMachine(now);
  updateSmooth(now);
  updateScan(now);
}

String laserGetStatusJSON() {
  String j = "{\"pin\":";
  j += LASER_PIN;
  j += ",\"on\":";
  j += (s_brightness > 0) ? "true" : "false";
  j += ",\"brightness\":";
  j += String((int)s_brightness);
  j += ",\"pan\":";
  j += String(servoGetPos(SERVO_PAN_CHANNEL));
  j += ",\"tilt\":";
  j += String(servoGetPos(SERVO_TILT_CHANNEL));
  j += ",\"scan\":";
  j += s_scanOn ? "true" : "false";
  j += ",\"mood\":";
  j += String((int)s_mood);
  j += "}";
  return j;
}
