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
//  System flags (systemFlags bitmask)
// ------------------------------------------------------------
#define FLAG_ESTOP      0x0001
#define FLAG_AUTONOMOUS 0x0002
#define FLAG_PRECISION  0x0004
#define FLAG_SUPERVISED 0x0008
#define FLAG_SLEEP      0x0010

#endif // PROTOCOL_H
