// ============================================================
// WALL-E Master Controller — Motion Engine
// 9 physical Base servos carried in a 10-slot compatibility packet.
// ============================================================

#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H

#include <Arduino.h>

// Packet/motion slots. Slot 1 is retained but unused for backward wire
// compatibility. Head tilt is the physical upper-neck servo, so both names
// intentionally address slot 4.
#define SERVO_HEAD_PAN       0
#define SERVO_UNUSED_1       1
#define SERVO_EYE_LEFT       2
#define SERVO_EYE_RIGHT      3
#define SERVO_NECK_TOP       4
#define SERVO_HEAD_TILT      SERVO_NECK_TOP
#define SERVO_NECK_BOTTOM    5
#define SERVO_LEFT_ARM       6
#define SERVO_RIGHT_ARM      7
#define SERVO_EYEBROW_RIGHT  8
#define SERVO_EYEBROW_LEFT   9
#define SERVO_COUNT          10

#define SERVO_MIN 0
#define SERVO_MAX 180

#define MAX_ANIMATION_TRACKS 2
#define ANIMATION_CROSSFADE_MS 300

struct AnimationTrack {
  bool active;
  unsigned long startTime;
  float fadeWeight;
  uint8_t animId;
  uint8_t currentFrame;
  unsigned long frameStartTime;
};

struct ServoState {
  float basePosition[SERVO_COUNT];
  float animationOffset[SERVO_COUNT];
  float joystickOffset[SERVO_COUNT];
  float joystickInfluence[SERVO_COUNT];
  float targetPosition[SERVO_COUNT];
  float currentPosition[SERVO_COUNT];
};

void motionInit();
void motionUpdate(unsigned long now);

void motionSetHeadSensitivity(float sensitivity);
void motionSetServoSpeedLimit(float limit);

void motionSetHeadPanVelocity(float vel);
void motionSetHeadTiltVelocity(float vel);
void motionSetJoystickOverride(uint8_t servoIndex, float offset, float influence);

void motionGetServoTargets(uint8_t* targets);

void motionTriggerAnimation(uint8_t animId);
void motionStopAllAnimations();

void motionEmergencyStop();
void motionSetNeutralPositions(const uint8_t* neutral);

void motionSetServoDirect(uint8_t servoIndex, uint8_t degrees);
void motionSetAllNeutral();
void motionTestPose1();
void motionTestPose2();

#endif // MOTION_ENGINE_H
