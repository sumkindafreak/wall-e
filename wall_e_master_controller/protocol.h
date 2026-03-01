// ============================================================
//  WALL-E Master Controller — Protocol Definitions
//  Control and telemetry packet structures for ESP-NOW
// ============================================================

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------
//  Drive State — abstract model for input layer
// ------------------------------------------------------------
typedef struct {
  int8_t  leftSpeed;    // -100 to 100
  int8_t  rightSpeed;   // -100 to 100
  bool    precisionMode;
} DriveState;

// ------------------------------------------------------------
//  Control Packet — sent to WALL-E Base via ESP-NOW
// ------------------------------------------------------------
typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;
  int8_t   rightSpeed;
  uint8_t  driveMode;     // 0=manual, 1=precision
  uint8_t  behaviourMode; // mood override
  uint8_t  action;        // trigger event (scan, beep, lookaround, etc.)
  uint16_t systemFlags;   // bitmask flags
  uint8_t  servoTargets[10];  // NEW: 10 servo positions (0-180 degrees)
} ControlPacket;

// ------------------------------------------------------------
//  Telemetry Packet — received from WALL-E Base
// ------------------------------------------------------------
typedef struct __attribute__((packed)) {
  float   batteryVoltage;
  float   currentDraw;
  float   temperature;
  uint8_t moodState;
  uint8_t autonomousState;
  uint8_t safetyState;
  
  // Autonomy telemetry (NEW)
  uint8_t autonomyEnabled;      // 0=disabled, 1=enabled
  uint8_t autonomyState;        // AutoState enum value
  float   sonarDistanceCm;      // Current sonar reading
  float   compassHeading;       // Current heading (0-360)
  float   gpsLatitude;          // GPS latitude (high precision)
  float   gpsLongitude;         // GPS longitude (high precision)
  uint8_t gpsValid;             // GPS fix status
  uint8_t waypointMode;         // Waypoint navigation active
  float   waypointDistanceM;    // Distance to current waypoint (meters)
  float   waypointBearingDeg;   // Bearing to waypoint (0-360)
  uint8_t currentWaypoint;      // Current waypoint index
  uint8_t totalWaypoints;       // Total waypoint count
} TelemetryPacket;

// ------------------------------------------------------------
//  Action codes (packet.action)
// ------------------------------------------------------------
#define ACTION_NONE        0
#define ACTION_SCAN        1
#define ACTION_BEEP        2
#define ACTION_LOOKAROUND  3
#define ACTION_SLEEP       4
#define ACTION_WAKE        5
#define ACTION_IMU_CAL     6
#define ACTION_MOTOR_RESET 7

// ------------------------------------------------------------
//  Mood codes (behaviourMode / moodState)
// ------------------------------------------------------------
#define MOOD_CURIOUS   0
#define MOOD_HAPPY     1
#define MOOD_SHY       2
#define MOOD_TIRED     3
#define MOOD_EXCITED   4

// ------------------------------------------------------------
//  Servo indices (match motion_engine.h)
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
//  System flags (systemFlags bitmask)
// ------------------------------------------------------------
#define FLAG_ESTOP      0x0001
#define FLAG_AUTONOMOUS 0x0002
#define FLAG_PRECISION  0x0004
#define FLAG_SUPERVISED 0x0008
#define FLAG_SLEEP      0x0010

#endif // PROTOCOL_H
