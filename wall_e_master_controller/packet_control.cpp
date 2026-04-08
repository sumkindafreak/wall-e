// ============================================================
//  WALL-E Master Controller — Packet Control Implementation
//  Stable 50Hz send, safety lock, telemetry passthrough
// ============================================================

#include "packet_control.h"
#include "espnow_control.h"
#include "motion_engine.h"
#include "cyd_laser_ui.h"
#include "ui_state.h"
#include <string.h>
#include <Arduino.h>

static unsigned long s_lastSendMs = 0;
static uint8_t s_pendingAction = ACTION_NONE;
static uint8_t s_pendingAux0 = 0;
static uint8_t s_pendingAux1 = 0;

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
  pkt.aux0 = s_pendingAux0;
  pkt.aux1 = s_pendingAux1;
  if (s_pendingAction == ACTION_AUTONOMY_REMOTE) {
    /* aux already set */
  } else {
    pkt.aux0 = 0;
    pkt.aux1 = 0;
  }
  s_pendingAction = ACTION_NONE;
  s_pendingAux0 = 0;
  s_pendingAux1 = 0;
  pkt.systemFlags = estop ? FLAG_ESTOP : 0;
  if (estop) {
    cydLaserUiSetArmed(false);
  } else {
    pkt.systemFlags |= cydLaserUiGetExtraFlags();
    if (g_remoteAutonomyArm) {
      pkt.systemFlags |= FLAG_AUTONOMOUS;
    }
  }

  espnowSend(&pkt);
  espnowBroadcastAudioEstopEdge(estop);
}

void packetSetPendingAction(uint8_t action) {
  s_pendingAction = action;
}

void packetSetAutonomyConfig(uint8_t key, uint8_t value) {
  s_pendingAction = ACTION_AUTONOMY_REMOTE;
  s_pendingAux0 = key;
  s_pendingAux1 = value;
}

bool packetTelemetryValid(void) {
  return espnowTelemetryValid();
}

void packetGetTelemetry(TelemetryPacket* out) {
  espnowGetTelemetry(out);
}
