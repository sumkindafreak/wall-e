#pragma once

// ============================================================
// WALL-E shared control / telemetry protocol
//
// Single source of truth for the CYD <-> Base wire format.
// The P4 radio gateway transports these bytes unchanged.
// ============================================================

#include <stdint.h>
#include <stdbool.h>

#define WALLE_CONTROL_WIRE_VERSION 1u
#define WALLE_CONTROL_SERVO_SLOTS 10u

// ------------------------------------------------------------
// Drive State - local helper used by the master input layer
// ------------------------------------------------------------
typedef struct {
  int8_t leftSpeed;
  int8_t rightSpeed;
  bool precisionMode;
} DriveState;

// ------------------------------------------------------------
// Servo target wire slots
//
// Slot 1 is retained for byte compatibility with older controllers. The
// current master drives head tilt through NECK_TOP (slot 4), matching the
// physical Base which has nine PCA9685 servo channels and no separate tenth
// head-tilt actuator.
// ------------------------------------------------------------
enum WalleControlServoSlot : uint8_t {
  WALLE_CTRL_SERVO_HEAD_PAN = 0,
  WALLE_CTRL_SERVO_LEGACY_HEAD_TILT = 1,
  WALLE_CTRL_SERVO_EYE_LEFT = 2,
  WALLE_CTRL_SERVO_EYE_RIGHT = 3,
  WALLE_CTRL_SERVO_NECK_TOP = 4,
  WALLE_CTRL_SERVO_NECK_BOTTOM = 5,
  WALLE_CTRL_SERVO_LEFT_ARM = 6,
  WALLE_CTRL_SERVO_RIGHT_ARM = 7,
  WALLE_CTRL_SERVO_EYEBROW_RIGHT = 8,
  WALLE_CTRL_SERVO_EYEBROW_LEFT = 9
};

// ------------------------------------------------------------
// Control Packet - CYD -> Base
// ------------------------------------------------------------
typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;
  int8_t   rightSpeed;
  uint8_t  driveMode;
  uint8_t  behaviourMode;
  uint8_t  action;
  uint16_t systemFlags;
  uint8_t  servoTargets[WALLE_CONTROL_SERVO_SLOTS];
  uint8_t  aux0;
  uint8_t  aux1;
} ControlPacket;

// ------------------------------------------------------------
// Telemetry Packet - Base -> CYD
// ------------------------------------------------------------
typedef struct __attribute__((packed)) {
  float   batteryVoltage;
  float   currentDraw;
  float   temperature;
  uint8_t moodState;
  uint8_t autonomousState;
  uint8_t safetyState;

  uint8_t autonomyEnabled;
  uint8_t autonomyState;
  float   sonarDistanceCm;
  float   compassHeading;
  float   gpsLatitude;
  float   gpsLongitude;
  uint8_t gpsValid;
  uint8_t waypointMode;
  float   waypointDistanceM;
  float   waypointBearingDeg;
  uint8_t currentWaypoint;
  uint8_t totalWaypoints;
} TelemetryPacket;

static_assert(sizeof(ControlPacket) == 19, "ControlPacket wire size changed");
static_assert(sizeof(TelemetryPacket) == 45, "TelemetryPacket wire size changed");

// ------------------------------------------------------------
// Action codes
// ------------------------------------------------------------
#define ACTION_NONE             0
#define ACTION_SCAN             1
#define ACTION_BEEP             2
#define ACTION_LOOKAROUND       3
#define ACTION_SLEEP            4
#define ACTION_WAKE             5
#define ACTION_IMU_CAL          6
#define ACTION_MOTOR_RESET      7
#define ACTION_DOCK_GO          8
#define ACTION_DOCK_CANCEL      9
#define ACTION_STOP_ALL        10
#define ACTION_AUTONOMY_REMOTE 11

#define AUTONOMY_KEY_DETECT_CLOSE_CM       1
#define AUTONOMY_KEY_DETECT_INTEREST_CM    2
#define AUTONOMY_KEY_CURIOSITY            10
#define AUTONOMY_KEY_BRAVERY              11
#define AUTONOMY_KEY_ENERGY               12
#define AUTONOMY_KEY_RANDOMNESS           13
#define AUTONOMY_KEY_WAYPOINT_MODE        20
#define AUTONOMY_KEY_PRESET               30

// ------------------------------------------------------------
// Mood codes
// ------------------------------------------------------------
#define MOOD_CURIOUS  0
#define MOOD_HAPPY    1
#define MOOD_SHY      2
#define MOOD_TIRED    3
#define MOOD_EXCITED  4

// ------------------------------------------------------------
// System flags
// ------------------------------------------------------------
#define FLAG_ESTOP       0x0001u
#define FLAG_AUTONOMOUS  0x0002u
#define FLAG_PRECISION   0x0004u
#define FLAG_SUPERVISED  0x0008u
#define FLAG_SLEEP       0x0010u
#define FLAG_LASER       0x0020u
