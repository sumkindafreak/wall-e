// ============================================================
//  WALL-E Master Controller — Packet Control Implementation
//  Stable 50Hz send, safety lock, telemetry passthrough
// ============================================================

#include "packet_control.h"
#include "espnow_control.h"
#include "motion_engine.h"
#include "cyd_laser_ui.h"
#include <string.h>
#include <Arduino.h>

static unsigned long s_lastSendMs = 0;
static uint8_t s_pendingAction = ACTION_NONE;

void packetInit(void) {
  espnowInit();
  s_lastSendMs = 0;
}

void packetUpdate(unsigned long now, const DriveState* ds, bool estop) {
  espnowUpdate();

  // Stable 50Hz — send only every 20ms
  if (now - s_lastSendMs < PACKET_SEND_INTERVAL_MS)
    return;
  s_lastSendMs = now;

  ControlPacket pkt;
  memset(&pkt, 0, sizeof(pkt));
  
  // Drive state
  if (ds) {
    pkt.leftSpeed  = ds->leftSpeed;
    pkt.rightSpeed = ds->rightSpeed;
    pkt.driveMode  = ds->precisionMode ? 1 : 0;
  }
  
  // Servo targets from motion engine
  motionGetServoTargets(pkt.servoTargets);
  
  pkt.behaviourMode = 0;
  pkt.action = s_pendingAction;
  s_pendingAction = ACTION_NONE;
  pkt.systemFlags = estop ? FLAG_ESTOP : 0;
  if (estop) {
    cydLaserUiSetArmed(false);
  } else {
    pkt.systemFlags |= cydLaserUiGetExtraFlags();
  }

  espnowSend(&pkt);
  espnowBroadcastAudioEstopEdge(estop);
}

void packetSetPendingAction(uint8_t action) {
  s_pendingAction = action;
}

bool packetTelemetryValid(void) {
  return espnowTelemetryValid();
}

void packetGetTelemetry(TelemetryPacket* out) {
  espnowGetTelemetry(out);
}
