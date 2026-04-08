#pragma once

#include <cstdint>

// ============================================================
//  WALL-E Motor Control Header
//  L298N Dual H-Bridge on ESP32-S3 + drive profiles
// ============================================================

// --- Pin Definitions (do not change without hardware update) ---
#define MOTOR_LEFT_IN1   4
#define MOTOR_LEFT_IN2   5
#define MOTOR_LEFT_ENA   6   // PWM

#define MOTOR_RIGHT_IN3  7
#define MOTOR_RIGHT_IN4  8
#define MOTOR_RIGHT_ENB  9   // PWM

// --- PWM (L298N enable pins) ---
#define PWM_FREQ         30000
#define PWM_RESOLUTION   8

#define SPEED_MAX        255
#define SPEED_DEFAULT    200

// ============================================================
//  Drive profiles — same ramped core, different feel (tune in motor_control.cpp)
// ============================================================

enum DriveProfile : uint8_t {
  DRIVE_PROFILE_NORMAL = 0,
  DRIVE_PROFILE_PRECISION,
  DRIVE_PROFILE_EXPRESSIVE,
  DRIVE_PROFILE_COUNT
};

struct DriveProfileConfig {
  uint8_t rampStep;        // PWM units per ramp step toward target
  uint8_t rampIntervalMs;  // how often ramp steps apply
  uint8_t minRunSpeed;     // stiction kick from rest (clamped to maxPwm)
  uint8_t maxPwm;          // cap on |PWM| for this mode (logical full stick = this)
};

void motorInit();
void motorHandle();  // ramp + apply (uses active profile)

void motorForward(uint8_t speed);
void motorReverse(uint8_t speed);
void motorLeft(uint8_t speed);
void motorRight(uint8_t speed);
void motorStop();  // immediate hard stop — ignores profile, clears ramp state
void motorSetSpeed(uint8_t speed);
uint8_t motorGetSpeed();

// Tank drive: targets -255..255 (scaled internally to active profile maxPwm)
void motorSetLeftRight(int16_t left, int16_t right);

// --- Drive profile API ---
void motorSetDriveProfile(DriveProfile profile);
// Second form logs (reason) when the profile actually changes — no spam if already active.
void motorSetDriveProfile(DriveProfile profile, const char* reason);
DriveProfile motorGetDriveProfile(void);
const char* motorGetDriveProfileName(DriveProfile profile);
const char* motorGetActiveDriveProfileName(void);
const DriveProfileConfig* motorGetDriveProfileConfig(DriveProfile profile);
const DriveProfileConfig* motorGetActiveDriveProfileConfig(void);
void motorApplyProfileDefaults(void);  // active profile = NORMAL + log
