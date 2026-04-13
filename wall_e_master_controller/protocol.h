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
  uint8_t  aux0;            // ACTION_AUTONOMY_REMOTE: config key (see below)
  uint8_t  aux1;            // ACTION_AUTONOMY_REMOTE: value 0-255
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
  /** Base motion policy: 0=any, 1=cyd_only, 2=web_only (see MotionAuthorityMode on base) */
  uint8_t motionPolicy;
  /** 1 if CYD sticks are active but policy blocks drive (e.g. web_only + non-idle sticks) */
  uint8_t policyDenyCyd;
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
#define ACTION_DOCK_GO     8
#define ACTION_DOCK_CANCEL 9
#define ACTION_STOP_ALL    10
/** Remote autonomy/personality tuning (aux0=key, aux1=value) — must match Base espnow_receiver */
#define ACTION_AUTONOMY_REMOTE  11
/** Set motion authority: aux0 = 0 any, 1 cyd_only, 2 web_only (paired CYD only; same trust as drive) */
#define ACTION_MOTION_POLICY    12

/** Keys for ACTION_AUTONOMY_REMOTE (aux0) */
#define AUTONOMY_KEY_DETECT_CLOSE_CM    1   /* aux1: cm 10-150 */
#define AUTONOMY_KEY_DETECT_INTEREST_CM   2   /* aux1: cm, > close */
#define AUTONOMY_KEY_CURIOSITY          10   /* aux1: 0-100 → personality */
#define AUTONOMY_KEY_BRAVERY            11
#define AUTONOMY_KEY_ENERGY             12
#define AUTONOMY_KEY_RANDOMNESS         13
#define AUTONOMY_KEY_WAYPOINT_MODE      20   /* aux1: 0/1 */
#define AUTONOMY_KEY_PRESET             30   /* aux1: PersonalityPreset 0-3 */

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
#define FLAG_LASER      0x0020  /* Laser diode on (base); points with head pose from CYD */

#endif // PROTOCOL_H
