// ============================================================
//  WALL-E Master Controller — Motion Engine
//  9-servo offset blending + joystick override per axis
//  2 animation tracks with crossfade + idle layer
// ============================================================

#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H

#include <Arduino.h>

// Servo indices (match Base controller servo mapping)
#define SERVO_HEAD_PAN       0
#define SERVO_HEAD_TILT      1
#define SERVO_EYE_LEFT       2
#define SERVO_EYE_RIGHT      3
#define SERVO_NECK_TOP       4
#define SERVO_NECK_BOTTOM    5
#define SERVO_LEFT_ARM       6
#define SERVO_RIGHT_ARM      7
#define SERVO_EYEBROW_RIGHT  8  // NEW
#define SERVO_EYEBROW_LEFT   9  // NEW
#define SERVO_COUNT          10

// Servo limits (degrees, 0-180)
#define SERVO_MIN 0
#define SERVO_MAX 180

// Animation system
#define MAX_ANIMATION_TRACKS 2
#define ANIMATION_CROSSFADE_MS 300

// Animation track
struct AnimationTrack {
  bool active;
  unsigned long startTime;
  float fadeWeight;
  uint8_t animId;
  uint8_t currentFrame;
  unsigned long frameStartTime;
};

// Motion engine state
struct ServoState {
  float basePosition[SERVO_COUNT];      // Base position (from profile/neutral)
  float animationOffset[SERVO_COUNT];   // Current animation influence
  float joystickOffset[SERVO_COUNT];    // Joystick override
  float joystickInfluence[SERVO_COUNT]; // 0.0 = animation, 1.0 = joystick
  float targetPosition[SERVO_COUNT];    // Final output (after blending)
  float currentPosition[SERVO_COUNT];   // Smoothed output
};

// Init
void motionInit();

// Update (call every loop)
void motionUpdate(unsigned long now);

// Profile tuning
void motionSetHeadSensitivity(float sensitivity);  // 0.5 to 2.0 multiplier
void motionSetServoSpeedLimit(float limit);        // 0.0 to 1.0

// Set joystick inputs (velocity-based for head, position-based for others)
void motionSetHeadPanVelocity(float vel);   // -1.0 to 1.0 (rad/s scaled)
void motionSetHeadTiltVelocity(float vel);  // -1.0 to 1.0
void motionSetJoystickOverride(uint8_t servoIndex, float offset, float influence);

// Get final servo targets (0-180 degrees)
void motionGetServoTargets(uint8_t* targets);  // Array of 10 values

// Animation control
void motionTriggerAnimation(uint8_t animId);
void motionStopAllAnimations();

// Safety
void motionEmergencyStop();  // Neutral all servos
void motionSetNeutralPositions(const uint8_t* neutral);  // Set base positions

// Direct servo control (for servo test page)
void motionSetServoDirect(uint8_t servoIndex, uint8_t degrees);  // Set single servo directly
void motionSetAllNeutral();  // Set all servos to 90°
void motionTestPose1();  // Test pose 1
void motionTestPose2();  // Test pose 2

#endif // MOTION_ENGINE_H
