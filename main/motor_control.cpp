#include "motor_control.h"
#include <Arduino.h>

// ============================================================
//  WALL-E Motor Control Implementation
// ============================================================

static uint8_t _currentSpeed = SPEED_DEFAULT;

void motorInit() {
  // Configure direction pins
  pinMode(MOTOR_LEFT_IN1, OUTPUT);
  pinMode(MOTOR_LEFT_IN2, OUTPUT);
  pinMode(MOTOR_RIGHT_IN3, OUTPUT);
  pinMode(MOTOR_RIGHT_IN4, OUTPUT);

  // Configure PWM channels (ESP32 Arduino 3.x API: ledcAttach replaces ledcSetup+ledcAttachPin)
  ledcAttach(MOTOR_LEFT_ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(MOTOR_RIGHT_ENB, PWM_FREQ, PWM_RESOLUTION);

  motorStop();
}

// --- Internal Helpers ---
// Map logical speed 0-255 to PWM: 0 stays 0, 1-255 → MOTOR_PWM_MIN to 255 (motors don't move below min)
static uint8_t speedToPwm(uint8_t speed) {
  if (speed == 0) return 0;
  return (uint8_t)constrain((int)MOTOR_PWM_MIN + ((255 - MOTOR_PWM_MIN) * (int)speed) / 255, MOTOR_PWM_MIN, 255);
}

static void setLeft(bool fwd, uint8_t speed) {
  digitalWrite(MOTOR_LEFT_IN1, fwd ? HIGH : LOW);
  digitalWrite(MOTOR_LEFT_IN2, fwd ? LOW  : HIGH);
  ledcWrite(MOTOR_LEFT_ENA, speedToPwm(speed));
}

// Right motor direction inverted (wiring / L298N side); swap so logical "forward" = physical forward
static void setRight(bool fwd, uint8_t speed) {
  digitalWrite(MOTOR_RIGHT_IN3, fwd ? LOW  : HIGH);
  digitalWrite(MOTOR_RIGHT_IN4, fwd ? HIGH : LOW);
  ledcWrite(MOTOR_RIGHT_ENB, speedToPwm(speed));
}

// --- Public Commands ---

void motorForward(uint8_t speed) {
  setLeft(true,  speed);
  setRight(true, speed);
}

void motorReverse(uint8_t speed) {
  setLeft(false,  speed);
  setRight(false, speed);
}

void motorLeft(uint8_t speed) {
  // Spin left: left motor reverse, right motor forward
  setLeft(false, speed);
  setRight(true, speed);
}

void motorRight(uint8_t speed) {
  // Spin right: left motor forward, right motor reverse
  setLeft(true,  speed);
  setRight(false, speed);
}

void motorStop() {
  digitalWrite(MOTOR_LEFT_IN1,  LOW);
  digitalWrite(MOTOR_LEFT_IN2,  LOW);
  digitalWrite(MOTOR_RIGHT_IN3, LOW);
  digitalWrite(MOTOR_RIGHT_IN4, LOW);
  ledcWrite(MOTOR_LEFT_ENA, 0);
  ledcWrite(MOTOR_RIGHT_ENB, 0);
}

void motorSetSpeed(uint8_t speed) {
  _currentSpeed = speed;
}

uint8_t motorGetSpeed() {
  return _currentSpeed;
}

void motorSetLeftRight(int16_t left, int16_t right) {
  left  = constrain(left,  -255, 255);
  right = constrain(right, -255, 255);
  setLeft(left >= 0, (uint8_t)abs(left));
  setRight(right >= 0, (uint8_t)abs(right));
}
