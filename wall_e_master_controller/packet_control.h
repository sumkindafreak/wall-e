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

/** One-shot remote autonomy config (ACTION_AUTONOMY_REMOTE + aux bytes). */
void packetSetAutonomyConfig(uint8_t key, uint8_t value);

/** One-shot motion authority (ACTION_MOTION_POLICY): mode 0=any, 1=cyd_only, 2=web_only */
void packetSetMotionPolicy(uint8_t mode);

/** One-shot EVE targets (ACTION_EVE_UART_SERVO) for LVGL / extended UI */
void packetSetEveServo(uint8_t headPanDeg, uint8_t rightArmDeg);

// Telemetry (read from ESP-NOW)
bool packetTelemetryValid(void);
void packetGetTelemetry(TelemetryPacket* out);

#endif // PACKET_CONTROL_H
