#pragma once

#include <cstdint>

// ============================================================
//  WALL-E Motor Control Header
//  L298N Dual H-Bridge on ESP32-S3
// ============================================================

// --- Pin Definitions ---
#define MOTOR_LEFT_IN1   4
#define MOTOR_LEFT_IN2   5
#define MOTOR_LEFT_ENA   6   // PWM

#define MOTOR_RIGHT_IN3  7
#define MOTOR_RIGHT_IN4  8
#define MOTOR_RIGHT_ENB  9   // PWM

// --- PWM Settings ---
#define PWM_FREQ         20000
#define PWM_RESOLUTION   8
#define PWM_CHANNEL_L    0
#define PWM_CHANNEL_R    1

#define SPEED_MAX        255
#define SPEED_DEFAULT    200
#define MOTOR_PWM_MIN    90   // Motors don't move below this; 0 = stop, 1-255 mapped to 90-255

void motorInit();
void motorForward(uint8_t speed);
void motorReverse(uint8_t speed);
void motorLeft(uint8_t speed);
void motorRight(uint8_t speed);
void motorStop();
void motorSetSpeed(uint8_t speed);
uint8_t motorGetSpeed();

// Tank drive: set each side -255..255 (negative = reverse). Smoother than discrete FWD/REV/LEFT/RIGHT.
void motorSetLeftRight(int16_t left, int16_t right);
