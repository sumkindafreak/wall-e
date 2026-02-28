// ============================================================
//  WALL-E Master Controller — ESP-NOW Control
//  Send ControlPacket at 50Hz, receive TelemetryPacket async
// ============================================================

#ifndef ESPNOW_CONTROL_H
#define ESPNOW_CONTROL_H

#include "protocol.h"
#include <stdbool.h>

#define ESPNOW_SEND_INTERVAL_MS  20   // 50 Hz
#define ESPNOW_NO_INPUT_TIMEOUT  200  // ms → STOP
#define ESPNOW_TELEM_TIMEOUT_MS  3000 // telemetry timeout warning

void espnowInit(void);
void espnowSend(const ControlPacket* pkt);
void espnowUpdate(void);

bool espnowIsConnected(void);
bool espnowTelemetryValid(void);
void espnowGetTelemetry(TelemetryPacket* out);
uint16_t espnowGetPacketRate(void);  // Packets/s (rolling)
void espnowSetPeerMac(const uint8_t mac[6]);

#endif // ESPNOW_CONTROL_H
