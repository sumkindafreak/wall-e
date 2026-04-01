// ============================================================
//  Structured command queue — non-blocking FIFO for CYD master
//  Drained each loop; drive overrides are short pulses unless
//  physical joystick is active (joystick always wins).
// ============================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef enum {
  CmdType_NONE = 0,
  CmdType_MOVE_FORWARD,
  CmdType_MOVE_BACKWARD,
  CmdType_TURN_LEFT,
  CmdType_TURN_RIGHT,
  CmdType_HEAD_ROTATE,
  CmdType_HEAD_TILT,
  CmdType_LASER_ON,
  CmdType_LASER_OFF,
  CmdType_STOP_ALL,
} CmdType;

typedef struct {
  uint8_t  type;
  int16_t  param0;
  int16_t  param1;
} QueuedCommand;

void commandQueueInit(void);

bool commandQueuePush(uint8_t type, int16_t p0, int16_t p1);
bool commandQueuePop(QueuedCommand* out);
void commandQueueClear(void);
uint8_t commandQueueDepth(void);

/** Process all pending commands (mutates motion, laser UI, E-STOP, etc.). */
void commandQueueDrainAll(void);

/** Optional timed drive override (200 ms default) from MOVE_* / TURN_* commands. */
void commandQueueApplyDriveOverride(DriveState* ds, bool joystickActive);
