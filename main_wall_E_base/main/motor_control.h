#pragma once

#include <cstdint>
#include "base_board_pins.h"

// ============================================================
// WALL-E Motor Control Header
// L298N dual H-bridge + drive profiles
// ============================================================

#define MOTOR_LEFT_IN1   BASE_PIN_MOTOR_LEFT_IN1
#define MOTOR_LEFT_IN2   BASE_PIN_MOTOR_LEFT_IN2
#define MOTOR_LEFT_ENA   BASE_PIN_MOTOR_LEFT_ENA

#define MOTOR_RIGHT_IN3  BASE_PIN_MOTOR_RIGHT_IN3
#define MOTOR_RIGHT_IN4  BASE_PIN_MOTOR_RIGHT_IN4
#define MOTOR_RIGHT_ENB  BASE_PIN_MOTOR_RIGHT_ENB

#define PWM_FREQ         30000
#define PWM_RESOLUTION   8

#define SPEED_MAX        255
#define SPEED_DEFAULT    200

enum DriveProfile : uint8_t {
  DRIVE_PROFILE_NORMAL = 0,
  DRIVE_PROFILE_PRECISION,
  DRIVE_PROFILE_EXPRESSIVE,
  DRIVE_PROFILE_COUNT
};

struct DriveProfileConfig {
  uint8_t rampStep;
  uint8_t rampIntervalMs;
  uint8_t minRunSpeed;
  uint8_t maxPwm;
};

void motorInit();
void motorHandle();

void motorForward(uint8_t speed);
void motorReverse(uint8_t speed);
void motorLeft(uint8_t speed);
void motorRight(uint8_t speed);
void motorStop();
void motorSetSpeed(uint8_t speed);
uint8_t motorGetSpeed();

void motorSetLeftRight(int16_t left, int16_t right);

void motorSetDriveProfile(DriveProfile profile);
void motorSetDriveProfile(DriveProfile profile, const char* reason);
DriveProfile motorGetDriveProfile(void);
const char* motorGetDriveProfileName(DriveProfile profile);
const char* motorGetActiveDriveProfileName(void);
const DriveProfileConfig* motorGetDriveProfileConfig(DriveProfile profile);
const DriveProfileConfig* motorGetActiveDriveProfileConfig(void);
void motorApplyProfileDefaults(void);
