// ============================================================
//  WALL-E Master Controller — Protocol Definitions
//  Control and telemetry packet structures for ESP-NOW
// ============================================================

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "walle_link_packet.h" /* ControlPacket, TelemetryPacket, v2 comms + CRC */

// ------------------------------------------------------------
//  Drive State — abstract model for input layer
// ------------------------------------------------------------
typedef struct {
  int8_t  leftSpeed;    // -100 to 100
  int8_t  rightSpeed;   // -100 to 100
  bool    precisionMode;
} DriveState;

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
/** Forward CYD EVE panel targets to base → EVE UART (aux0=head pan deg, aux1=right arm deg) */
#define ACTION_EVE_UART_SERVO   13

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
