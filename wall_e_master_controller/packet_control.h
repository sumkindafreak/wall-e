// ============================================================
//  WALL-E Master Controller — Packet Control
//  Thin wrapper: stable 50Hz send, safety lock, telemetry
// ============================================================

#ifndef PACKET_CONTROL_H
#define PACKET_CONTROL_H

#include "protocol.h"

#define PACKET_SEND_INTERVAL_MS  20   // 50 Hz stable
#define PACKET_SAFETY_TIMEOUT_MS 200  // No touch → STOP

void packetInit(void);
void packetUpdate(unsigned long now, const DriveState* ds, bool estop);

/* Set action for next packet(s) — e.g. ACTION_DOCK_GO, ACTION_DOCK_CANCEL. Cleared after send. */
void packetSetPendingAction(uint8_t action);

// Telemetry (read from ESP-NOW)
bool packetTelemetryValid(void);
void packetGetTelemetry(TelemetryPacket* out);

#endif // PACKET_CONTROL_H
