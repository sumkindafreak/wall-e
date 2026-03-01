// ==========================================================
// ADS1115 Joystick Input
// 4 channels: Joy1_X, Joy1_Y, Joy2_X, Joy2_Y
// Integrates with existing DriveState
// ==========================================================

#ifndef ADS1115_INPUT_H
#define ADS1115_INPUT_H

#include <Arduino.h>
#include "protocol.h"

// Joystick channel indices
#define JOY1_X 0  // ADS A0 -> Head Pan (future)
#define JOY1_Y 1  // ADS A1 -> Head Tilt (future)
#define JOY2_X 2  // ADS A2 -> Turn
#define JOY2_Y 3  // ADS A3 -> Throttle

struct JoystickState {
  float raw[4];        // Normalized -1..1
  float processed[4];  // After deadzone + expo
  bool active[4];      // Outside deadzone
};

bool ads1115Init();
void ads1115Update();
const JoystickState& getJoystickState();

// Convert joystick to DriveState (tank mix)
void joystickToDriveState(DriveState* ds);

// Tuning
void setJoystickDeadzone(float dz);
void setJoystickExpo(float exp);
void setJoystickMaxOutput(float max);

#endif
