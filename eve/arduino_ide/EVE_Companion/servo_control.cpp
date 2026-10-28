#include "servo_control.h"

#include <math.h>

#include "config.h"
#include "eve_output_ids.h"
#include "eve_safety.h"

// -----------------------------------------------------------------------------
// Channel table — everything comes from config.h
// -----------------------------------------------------------------------------
namespace {

struct ChannelCfg {
  const char* name;
  int pin;
  int16_t minDeg;
  int16_t maxDeg;
  int16_t restDeg;
  bool invert;
};

const ChannelCfg kCfg[EVE_SERVO_COUNT] = {
    {"head", EVE_SERVO_L_PIN, EVE_SERVO_HEAD_MIN_DEG, EVE_SERVO_HEAD_MAX_DEG, EVE_SERVO_HEAD_REST_DEG,
     EVE_SERVO_HEAD_INVERT != 0},
    {"arm", EVE_SERVO_R_PIN, EVE_SERVO_ARM_MIN_DEG, EVE_SERVO_ARM_MAX_DEG, EVE_SERVO_ARM_REST_DEG,
     EVE_SERVO_ARM_INVERT != 0},
};

struct ChannelState {
  bool attached;   /* LEDC PWM attached to the pin */
  bool active;     /* pulses currently being sent */
  bool known;      /* commanded position is meaningful (a pulse has been sent since boot/release) */
  float pos;       /* commanded angle (deg), what the pulse currently says */
  float vel;       /* deg/s, signed */
  float target;    /* deg */
  float maxDps;    /* cruise speed for the current move */
};

ChannelState s_ch[EVE_SERVO_COUNT];
uint32_t s_lastTickMs = 0;
uint32_t s_lastRefuseLogMs = 0;
bool s_refuseLogged = false;

const uint32_t kPwmHz = 50;
const uint8_t kPwmBits = 14;
const uint32_t kPeriodUs = 1000000UL / kPwmHz;

int16_t clampDeg(uint8_t ch, int32_t d) {
  if (d < kCfg[ch].minDeg) return kCfg[ch].minDeg;
  if (d > kCfg[ch].maxDeg) return kCfg[ch].maxDeg;
  return (int16_t)d;
}

uint32_t dutyFor(uint8_t ch, float logicalDeg) {
  float phys = kCfg[ch].invert ? (180.0f - logicalDeg) : logicalDeg;
  if (phys < 0.f) phys = 0.f;
  if (phys > 180.f) phys = 180.f;
  const float us = (float)EVE_SERVO_MIN_US + ((float)(EVE_SERVO_MAX_US - EVE_SERVO_MIN_US) * phys / 180.0f);
  return (uint32_t)((us / (float)kPeriodUs) * (float)((1UL << kPwmBits) - 1));
}

#if EVE_ENABLE_SERVOS
bool pwmAttach(uint8_t ch) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  return ledcAttach((uint8_t)kCfg[ch].pin, kPwmHz, kPwmBits);
#else
  /* Arduino-ESP32 2.x: explicit LEDC channels 6 and 7 (kept away from other users). */
  const uint8_t ledc = (uint8_t)(6 + ch);
  if (ledcSetup(ledc, kPwmHz, kPwmBits) == 0) return false;
  ledcAttachPin((uint8_t)kCfg[ch].pin, ledc);
  return true;
#endif
}
#endif /* EVE_ENABLE_SERVOS */

void pwmWrite(uint8_t ch, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite((uint8_t)kCfg[ch].pin, duty);
#else
  ledcWrite((uint8_t)(6 + ch), duty);
#endif
}

void output(uint8_t ch) {
  if (!s_ch[ch].attached) return;
  pwmWrite(ch, dutyFor(ch, s_ch[ch].pos));
  s_ch[ch].active = true;
}

void release(uint8_t ch) {
  if (!s_ch[ch].attached) return;
  pwmWrite(ch, 0); /* no pulses: most hobby servos stop driving */
  s_ch[ch].active = false;
  s_ch[ch].known = false; /* the horn may now be moved by gravity / hand */
}

void logRefuse(const char* what) {
  const uint32_t now = millis();
  if (s_refuseLogged && (uint32_t)(now - s_lastRefuseLogMs) < EVE_LOG_RATE_MS) return;
  s_refuseLogged = true;
  s_lastRefuseLogMs = now;
  Serial.printf("[EVE][SERVO] move refused: %s\n", what);
}

/** One easing step: accelerate to maxDps, decelerate to stop exactly on target. */
void step(ChannelState& c, float dt) {
  const float err = c.target - c.pos;
  if (fabsf(err) < 0.05f && fabsf(c.vel) < 1.0f) {
    c.pos = c.target;
    c.vel = 0.f;
    return;
  }
  const float accel = c.maxDps * 4.0f; /* full speed after ~0.25 s */
  const float dir = err > 0 ? 1.f : -1.f;
  const float stopDist = (c.vel * c.vel) / (2.f * accel);
  float desired;
  if (c.vel * dir < 0.f) {
    desired = 0.f; /* moving away from target (target changed): brake first */
  } else if (stopDist >= fabsf(err)) {
    desired = 0.f; /* time to decelerate */
  } else {
    desired = dir * c.maxDps;
  }
  const float dv = accel * dt;
  if (c.vel < desired) c.vel = fminf(c.vel + dv, desired);
  else if (c.vel > desired) c.vel = fmaxf(c.vel - dv, desired);
  if (fabsf(c.vel) < 2.0f && fabsf(err) > 0.05f) c.vel = dir * 2.0f; /* never stall short of target */
  float next = c.pos + c.vel * dt;
  if ((dir > 0 && next > c.target && c.vel > 0) || (dir < 0 && next < c.target && c.vel < 0)) {
    next = c.target;
    c.vel = 0.f;
  }
  c.pos = next;
}

}  // namespace

// =============================================================================
void servoInit(void) {
  s_lastTickMs = millis();
  for (uint8_t ch = 0; ch < EVE_SERVO_COUNT; ch++) {
    ChannelState& c = s_ch[ch];
    c = ChannelState();
    c.pos = c.target = (float)kCfg[ch].restDeg;
    c.maxDps = (float)EVE_SERVO_DEFAULT_DPS;
#if EVE_ENABLE_SERVOS
    if (kCfg[ch].pin < 0) {
      Serial.printf("[EVE][SERVO] %s: no pin configured - channel off\n", kCfg[ch].name);
      continue;
    }
    if (!pwmAttach(ch)) {
      Serial.printf("[EVE][SERVO] %s: PWM attach FAILED on GPIO%d\n", kCfg[ch].name, kCfg[ch].pin);
      eveSafetySetFault(EVE_FAULT_SERVO_DRIVER, true);
      continue;
    }
    pwmWrite(ch, 0); /* attached but silent: no pulse until the first command */
    c.attached = true;
    Serial.printf("[EVE][SERVO] %s: GPIO%d limits %d..%d rest %d%s, %d-%d us (UNVERIFIED), stop=%s\n", kCfg[ch].name,
                  kCfg[ch].pin, kCfg[ch].minDeg, kCfg[ch].maxDeg, kCfg[ch].restDeg, kCfg[ch].invert ? " inverted" : "",
                  EVE_SERVO_MIN_US, EVE_SERVO_MAX_US,
                  EVE_SERVO_STOP_POLICY == EVE_SERVO_STOP_RELEASE ? "release" : "hold");
#endif
  }
#if !EVE_ENABLE_SERVOS
  Serial.println(F("[EVE][SERVO] disabled (EVE_ENABLE_SERVOS 0) - no PWM generated"));
#endif
}

void servoTick(void) {
  const uint32_t now = millis();
  const uint32_t elapsed = now - s_lastTickMs;
  if (elapsed < EVE_SERVO_TICK_MS) return;
  s_lastTickMs = now;
  const float dt = (float)(elapsed > 100u ? 100u : elapsed) / 1000.0f;
  const bool motionOk = eveSafetyMotionAllowed();
  for (uint8_t ch = 0; ch < EVE_SERVO_COUNT; ch++) {
    ChannelState& c = s_ch[ch];
    if (!c.attached || !c.active) continue;
    if (!motionOk) {
      /* Belt and braces: whatever happens, nothing moves while motion is not allowed. */
      c.target = c.pos;
      c.vel = 0.f;
      continue;
    }
    const float before = c.pos;
    step(c, dt);
    if (fabsf(c.pos - before) > 0.001f) output(ch);
  }
}

bool servoAvailable(uint8_t ch) { return ch < EVE_SERVO_COUNT && s_ch[ch].attached; }

void servoGetLimits(uint8_t ch, int16_t* minDeg, int16_t* maxDeg) {
  if (ch >= EVE_SERVO_COUNT) return;
  if (minDeg) *minDeg = kCfg[ch].minDeg;
  if (maxDeg) *maxDeg = kCfg[ch].maxDeg;
}

bool servoMoveTo(uint8_t ch, int16_t deg, uint16_t degPerSec) {
  if (ch >= EVE_SERVO_COUNT || !s_ch[ch].attached) return false;
  if (!eveSafetyMotionAllowed()) {
    logRefuse(eveSafetyStopLatched() ? "stop latched" : (eveSafetyPowerInhibit() ? "battery critical" : "fault"));
    return false;
  }
  ChannelState& c = s_ch[ch];
  uint16_t dps = degPerSec ? degPerSec : (uint16_t)EVE_SERVO_DEFAULT_DPS;
  if (dps > EVE_SERVO_MAX_DPS) dps = EVE_SERVO_MAX_DPS;
  if (!c.known) {
    /* Physical position unknown: start from the rest pose, then ease to the target. */
    c.pos = (float)kCfg[ch].restDeg;
    c.vel = 0.f;
    c.known = true;
    Serial.printf("[EVE][SERVO] %s: first pulse at rest %d deg (real position was unknown)\n", kCfg[ch].name,
                  kCfg[ch].restDeg);
  }
  c.target = (float)clampDeg(ch, deg);
  c.maxDps = (float)dps;
  output(ch);
  return true;
}

void servoHaltAll(bool stop) {
  for (uint8_t ch = 0; ch < EVE_SERVO_COUNT; ch++) {
    ChannelState& c = s_ch[ch];
    c.target = c.pos;
    c.vel = 0.f;
    if (!c.attached) continue;
    if (stop && EVE_SERVO_STOP_POLICY == EVE_SERVO_STOP_RELEASE) {
      release(ch);
    }
    /* HOLD: keep sending the current pulse; the servo keeps its torque where it is. */
  }
  if (stop) {
    Serial.printf("[EVE][SERVO] halted (%s)\n", EVE_SERVO_STOP_POLICY == EVE_SERVO_STOP_RELEASE ? "released" : "holding");
  }
}

int16_t servoGetCommandedDeg(uint8_t ch) {
  if (ch >= EVE_SERVO_COUNT || !s_ch[ch].known) return -1;
  return (int16_t)lroundf(s_ch[ch].pos);
}

int16_t servoGetTargetDeg(uint8_t ch) {
  if (ch >= EVE_SERVO_COUNT || !s_ch[ch].known) return -1;
  return (int16_t)lroundf(s_ch[ch].target);
}

bool servoIsMoving(uint8_t ch) {
  return ch < EVE_SERVO_COUNT && s_ch[ch].known && fabsf(s_ch[ch].target - s_ch[ch].pos) > 0.05f;
}

bool servoOutputActive(uint8_t ch) { return ch < EVE_SERVO_COUNT && s_ch[ch].active; }

void servoSetAngles(int16_t headDeg, int16_t armDeg) {
  servoMoveTo(EVE_SERVO_HEAD_PAN, headDeg, EVE_SERVO_DEFAULT_DPS);
  servoMoveTo(EVE_SERVO_RIGHT_ARM, armDeg, EVE_SERVO_DEFAULT_DPS);
}

void servoSetHeadPanTarget(int16_t deg) { servoMoveTo(EVE_SERVO_HEAD_PAN, deg, EVE_SERVO_DEFAULT_DPS); }

void servoSetRightArmTarget(int16_t deg) { servoMoveTo(EVE_SERVO_RIGHT_ARM, deg, EVE_SERVO_DEFAULT_DPS); }
