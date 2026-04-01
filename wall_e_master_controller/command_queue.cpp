// ============================================================
//  Command queue implementation — ring buffer
// ============================================================

#include "command_queue.h"
#include "ui_state.h"
#include "motion_engine.h"
#include "cyd_laser_ui.h"
#include <Arduino.h>

extern void packetSetPendingAction(uint8_t action);
#include <string.h>

#define CQ_CAP 24

static QueuedCommand s_q[CQ_CAP];
static uint8_t s_head = 0;
static uint8_t s_tail = 0;
static uint8_t s_count = 0;

static int8_t  s_ovLeft = 0, s_ovRight = 0;
static uint32_t s_ovUntil = 0;
static const uint32_t CQ_DRIVE_PULSE_MS = 220;

void commandQueueInit(void) {
  s_head = s_tail = s_count = 0;
  memset(s_q, 0, sizeof(s_q));
  s_ovLeft = s_ovRight = 0;
  s_ovUntil = 0;
}

bool commandQueuePush(uint8_t type, int16_t p0, int16_t p1) {
  if (s_count >= CQ_CAP) {
    Serial.println(F("[CQ] Full — drop"));
    return false;
  }
  s_q[s_tail].type = type;
  s_q[s_tail].param0 = p0;
  s_q[s_tail].param1 = p1;
  s_tail = (uint8_t)((s_tail + 1) % CQ_CAP);
  s_count++;
  return true;
}

bool commandQueuePop(QueuedCommand* out) {
  if (!out || s_count == 0) return false;
  *out = s_q[s_head];
  s_head = (uint8_t)((s_head + 1) % CQ_CAP);
  s_count--;
  return true;
}

void commandQueueClear(void) {
  s_head = s_tail = s_count = 0;
}

uint8_t commandQueueDepth(void) {
  return s_count;
}

void commandQueueDrainAll(void) {
  QueuedCommand q;
  while (commandQueuePop(&q)) {
    int16_t sp = q.param0;
    if (sp < 0) sp = 0;
    if (sp > 100) sp = 100;
    switch ((CmdType)q.type) {
      case CmdType_MOVE_FORWARD:
        s_ovLeft = s_ovRight = (int8_t)sp;
        s_ovUntil = millis() + CQ_DRIVE_PULSE_MS;
        Serial.printf("[CMD] MOVE_FORWARD %d%%\n", (int)sp);
        break;
      case CmdType_MOVE_BACKWARD:
        s_ovLeft = s_ovRight = (int8_t)-sp;
        s_ovUntil = millis() + CQ_DRIVE_PULSE_MS;
        Serial.printf("[CMD] MOVE_BACKWARD %d%%\n", (int)sp);
        break;
      case CmdType_TURN_LEFT:
        s_ovLeft = (int8_t)-sp;
        s_ovRight = (int8_t)sp;
        s_ovUntil = millis() + CQ_DRIVE_PULSE_MS;
        Serial.printf("[CMD] TURN_LEFT %d%%\n", (int)sp);
        break;
      case CmdType_TURN_RIGHT:
        s_ovLeft = (int8_t)sp;
        s_ovRight = (int8_t)-sp;
        s_ovUntil = millis() + CQ_DRIVE_PULSE_MS;
        Serial.printf("[CMD] TURN_RIGHT %d%%\n", (int)sp);
        break;
      case CmdType_HEAD_ROTATE:
        if (q.param0 >= 0 && q.param0 <= 180)
          motionSetServoDirect(SERVO_HEAD_PAN, (uint8_t)q.param0);
        Serial.printf("[CMD] HEAD_ROTATE %d\n", (int)q.param0);
        break;
      case CmdType_HEAD_TILT:
        if (q.param0 >= 0 && q.param0 <= 180)
          motionSetServoDirect(SERVO_HEAD_TILT, (uint8_t)q.param0);
        Serial.printf("[CMD] HEAD_TILT %d\n", (int)q.param0);
        break;
      case CmdType_LASER_ON:
        cydLaserUiSetArmed(true);
        Serial.println(F("[CMD] LASER_ON"));
        break;
      case CmdType_LASER_OFF:
        cydLaserUiSetArmed(false);
        Serial.println(F("[CMD] LASER_OFF"));
        break;
      case CmdType_STOP_ALL:
        g_estop = true;
        g_controlAuthority = CTRL_SAFETY;
        motionEmergencyStop();
        cydLaserUiSetArmed(false);
        packetSetPendingAction(ACTION_STOP_ALL);
        Serial.println(F("[CMD] STOP_ALL"));
        break;
      default:
        break;
    }
  }
}

void commandQueueApplyDriveOverride(DriveState* ds, bool joystickActive) {
  if (!ds) return;
  if (joystickActive) return;
  if ((int32_t)(millis() - s_ovUntil) >= 0) return;
  ds->leftSpeed = s_ovLeft;
  ds->rightSpeed = s_ovRight;
}
