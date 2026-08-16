#pragma once

// ============================================================
// WALL-E shared control / telemetry protocol
//
// This is the single source of truth for the CYD <-> Base wire format.
// Keep the existing packed byte layout stable until an explicit protocol
// version migration is made.  The P4 radio gateway transports these bytes
// unchanged; it does not interpret robot commands.
// ============================================================

#include <stdint.h>
#include <stdbool.h>

#define WALLE_CONTROL_WIRE_VERSION 1u

// ------------------------------------------------------------
// Drive State - local helper used by the master input layer
// ------------------------------------------------------------
typedef struct {
  int8_t leftSpeed;      // -100 to +100
  int8_t rightSpeed;     // -100 to +100
  bool precisionMode;
} DriveState;

// ------------------------------------------------------------
// Control Packet - CYD -> Base
// ------------------------------------------------------------
typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;        // -100 to +100
  int8_t   rightSpeed;       // -100 to +100
  uint8_t  driveMode;        // 0=manual, 1=precision
  uint8_t  behaviourMode;    // mood / behaviour request
  uint8_t  action;           // ACTION_* below
  uint16_t systemFlags;      // FLAG_* below
  uint8_t  servoTargets[10]; // 0-180 degrees
  uint8_t  aux0;             // action-specific key
  uint8_t  aux1;             // action-specific value
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

// Compile-time guards catch accidental byte-layout drift immediately.
static_assert(sizeof(ControlPacket) == 19, "ControlPacket wire size changed");
static_assert(sizeof(TelemetryPacket) == 43, "TelemetryPacket wire size changed");

// ------------------------------------------------------------
// Action codes (ControlPacket.action)
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

// ACTION_AUTONOMY_REMOTE keys (aux0=key, aux1=value)
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
// Servo indices carried by the master packet
// ------------------------------------------------------------
#define SERVO_HEAD_PAN       0
#define SERVO_HEAD_TILT      1
#define SERVO_EYE_LEFT       2
#define SERVO_EYE_RIGHT      3
#define SERVO_NECK_TOP       4
#define SERVO_NECK_BOTTOM    5
#define SERVO_LEFT_ARM       6
#define SERVO_RIGHT_ARM      7
#define SERVO_LEFT_TRACK     8
#define SERVO_RIGHT_TRACK    9

// ------------------------------------------------------------
// System flags
// ------------------------------------------------------------
#define FLAG_ESTOP       0x0001u
#define FLAG_AUTONOMOUS  0x0002u
#define FLAG_PRECISION   0x0004u
#define FLAG_SUPERVISED  0x0008u
#define FLAG_SLEEP       0x0010u
#define FLAG_LASER       0x0020u
