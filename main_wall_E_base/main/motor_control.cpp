#include "motor_control.h"
#include "ledc_compat.h"
#include <Arduino.h>

// ============================================================
// WALL-E Motor Control — L298N + profile-based ramp
// motorSetLeftRight() = targets only; motorHandle() = ramp + output
// motorStop() = immediate hardware stop (safety)
// ============================================================

static uint8_t _currentSpeed = SPEED_DEFAULT;
static int16_t s_tgtL = 0;
static int16_t s_tgtR = 0;
static int16_t s_currL = 0;
static int16_t s_currR = 0;
static unsigned long s_lastMotorRampMs = 0;

static const DriveProfileConfig kProfileDefaults[DRIVE_PROFILE_COUNT] = {
  { 5, 20, 220, 210 },
  { 3, 28, 95, 115 },
  { 8, 15, 200, 255 },
};

static DriveProfile s_activeProfile = DRIVE_PROFILE_NORMAL;

static const DriveProfileConfig& activeCfg() {
  return kProfileDefaults[s_activeProfile];
}

static void setLeft(bool fwd, uint8_t pwm) {
  digitalWrite(MOTOR_LEFT_IN1, fwd ? HIGH : LOW);
  digitalWrite(MOTOR_LEFT_IN2, fwd ? LOW : HIGH);
  (void)walleLedcWrite(MOTOR_LEFT_ENA, WALLE_LEDC_CH_MOTOR_LEFT, pwm);
}

static void setRight(bool fwd, uint8_t pwm) {
  digitalWrite(MOTOR_RIGHT_IN3, fwd ? LOW : HIGH);
  digitalWrite(MOTOR_RIGHT_IN4, fwd ? HIGH : LOW);
  (void)walleLedcWrite(MOTOR_RIGHT_ENB, WALLE_LEDC_CH_MOTOR_RIGHT, pwm);
}

static void applyHardware(int16_t left, int16_t right) {
  left = (int16_t)constrain((int)left, -255, 255);
  right = (int16_t)constrain((int)right, -255, 255);
  setLeft(left >= 0, (uint8_t)abs(left));
  setRight(right >= 0, (uint8_t)abs(right));
}

static int16_t scaleTargetToCap(int16_t raw, uint8_t maxPwm) {
  const int32_t v = (int32_t)raw * (int32_t)maxPwm / 255;
  return (int16_t)constrain(v, -(int)maxPwm, (int)maxPwm);
}

static int16_t rampToward(int16_t curr, int16_t tgt,
                          const DriveProfileConfig& cfg) {
  const uint8_t cap = cfg.maxPwm;
  const uint8_t step = cfg.rampStep;
  uint8_t minRun = cfg.minRunSpeed;
  if (minRun > cap) minRun = cap;

  if (curr == tgt) return tgt;

  if (tgt == 0) {
    const int dec = (abs(curr) < (int)step) ? abs(curr) : (int)step;
    return (curr > 0) ? (int16_t)(curr - dec) : (int16_t)(curr + dec);
  }

  if (curr == 0) {
    const int at = abs(tgt);
    if (at < (int)minRun) return tgt;
    const int16_t kick = (int16_t)((tgt > 0) ? minRun : -(int)minRun);
    if (abs(kick) > abs(tgt)) return tgt;
    return kick;
  }

  const int32_t delta = (int32_t)tgt - (int32_t)curr;
  if (labs(delta) <= (int32_t)step) return tgt;
  return (int16_t)(curr + (delta > 0 ? (int32_t)step : -(int32_t)step));
}

void motorInit() {
  pinMode(MOTOR_LEFT_IN1, OUTPUT);
  pinMode(MOTOR_LEFT_IN2, OUTPUT);
  pinMode(MOTOR_RIGHT_IN3, OUTPUT);
  pinMode(MOTOR_RIGHT_IN4, OUTPUT);

  const bool leftPwmOk = walleLedcAttach(MOTOR_LEFT_ENA,
                                         WALLE_LEDC_CH_MOTOR_LEFT,
                                         PWM_FREQ,
                                         PWM_RESOLUTION);
  const bool rightPwmOk = walleLedcAttach(MOTOR_RIGHT_ENB,
                                          WALLE_LEDC_CH_MOTOR_RIGHT,
                                          PWM_FREQ,
                                          PWM_RESOLUTION);
  if (!leftPwmOk || !rightPwmOk) {
    Serial.println(F("[Motor] ERROR: LEDC attach failed; outputs forced stopped"));
  }

  s_tgtL = s_tgtR = s_currL = s_currR = 0;
  s_lastMotorRampMs = millis();
  s_activeProfile = DRIVE_PROFILE_NORMAL;
  motorStop();

  Serial.println(F("[Motor] Drive profiles:"));
  for (uint8_t i = 0; i < (uint8_t)DRIVE_PROFILE_COUNT; ++i) {
    const DriveProfileConfig& c = kProfileDefaults[i];
    Serial.printf("  %s: step=%u int=%ums minRun=%u maxPwm=%u\n",
                  motorGetDriveProfileName((DriveProfile)i),
                  (unsigned)c.rampStep,
                  (unsigned)c.rampIntervalMs,
                  (unsigned)c.minRunSpeed,
                  (unsigned)c.maxPwm);
  }
}

void motorHandle() {
  const DriveProfileConfig& cfg = activeCfg();
  const unsigned long now = millis();
  if ((now - s_lastMotorRampMs) >= (unsigned)cfg.rampIntervalMs) {
    s_lastMotorRampMs = now;
    const int16_t effL = scaleTargetToCap(s_tgtL, cfg.maxPwm);
    const int16_t effR = scaleTargetToCap(s_tgtR, cfg.maxPwm);
    s_currL = rampToward(s_currL, effL, cfg);
    s_currR = rampToward(s_currR, effR, cfg);
  }
  applyHardware(s_currL, s_currR);
}

static uint8_t clampSpeedToProfile(uint8_t speed) {
  const uint8_t cap = activeCfg().maxPwm;
  return (uint8_t)constrain((int)speed, 0, (int)cap);
}

void motorForward(uint8_t speed) {
  speed = clampSpeedToProfile(speed);
  motorSetLeftRight((int16_t)speed, (int16_t)speed);
}

void motorReverse(uint8_t speed) {
  speed = clampSpeedToProfile(speed);
  motorSetLeftRight((int16_t)-speed, (int16_t)-speed);
}

void motorLeft(uint8_t speed) {
  speed = clampSpeedToProfile(speed);
  motorSetLeftRight((int16_t)-speed, (int16_t)speed);
}

void motorRight(uint8_t speed) {
  speed = clampSpeedToProfile(speed);
  motorSetLeftRight((int16_t)speed, (int16_t)-speed);
}

void motorStop() {
  s_tgtL = s_tgtR = 0;
  s_currL = s_currR = 0;
  digitalWrite(MOTOR_LEFT_IN1, LOW);
  digitalWrite(MOTOR_LEFT_IN2, LOW);
  digitalWrite(MOTOR_RIGHT_IN3, LOW);
  digitalWrite(MOTOR_RIGHT_IN4, LOW);
  (void)walleLedcWrite(MOTOR_LEFT_ENA, WALLE_LEDC_CH_MOTOR_LEFT, 0);
  (void)walleLedcWrite(MOTOR_RIGHT_ENB, WALLE_LEDC_CH_MOTOR_RIGHT, 0);
}

void motorSetSpeed(uint8_t speed) {
  _currentSpeed = speed;
}

uint8_t motorGetSpeed() {
  return _currentSpeed;
}

void motorSetLeftRight(int16_t left, int16_t right) {
  s_tgtL = (int16_t)constrain((int)left, -255, 255);
  s_tgtR = (int16_t)constrain((int)right, -255, 255);
}

void motorSetDriveProfile(DriveProfile profile) {
  motorSetDriveProfile(profile, nullptr);
}

void motorSetDriveProfile(DriveProfile profile, const char* reason) {
  if (profile >= DRIVE_PROFILE_COUNT) profile = DRIVE_PROFILE_NORMAL;
  if (profile == s_activeProfile) return;
  s_activeProfile = profile;
  const DriveProfileConfig& c = activeCfg();
  if (reason && reason[0]) {
    Serial.printf("[Motor] Drive profile -> %s (%s) ramp=%u/%ums minRun=%u maxPwm=%u\n",
                  motorGetDriveProfileName(s_activeProfile), reason,
                  (unsigned)c.rampStep, (unsigned)c.rampIntervalMs,
                  (unsigned)c.minRunSpeed, (unsigned)c.maxPwm);
  } else {
    Serial.printf("[Motor] Drive profile -> %s ramp=%u/%ums minRun=%u maxPwm=%u\n",
                  motorGetDriveProfileName(s_activeProfile),
                  (unsigned)c.rampStep, (unsigned)c.rampIntervalMs,
                  (unsigned)c.minRunSpeed, (unsigned)c.maxPwm);
  }
}

DriveProfile motorGetDriveProfile(void) {
  return s_activeProfile;
}

const char* motorGetDriveProfileName(DriveProfile profile) {
  switch (profile) {
    case DRIVE_PROFILE_NORMAL: return "NORMAL";
    case DRIVE_PROFILE_PRECISION: return "PRECISION";
    case DRIVE_PROFILE_EXPRESSIVE: return "EXPRESSIVE";
    default: return "?";
  }
}

const char* motorGetActiveDriveProfileName(void) {
  return motorGetDriveProfileName(s_activeProfile);
}

const DriveProfileConfig* motorGetDriveProfileConfig(DriveProfile profile) {
  if (profile >= DRIVE_PROFILE_COUNT) {
    return &kProfileDefaults[DRIVE_PROFILE_NORMAL];
  }
  return &kProfileDefaults[profile];
}

const DriveProfileConfig* motorGetActiveDriveProfileConfig(void) {
  return &kProfileDefaults[s_activeProfile];
}

void motorApplyProfileDefaults(void) {
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "apply defaults");
}
