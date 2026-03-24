#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "dock_protocol.h"

void nodeHealthInit(void);

/** Incoming WalleNodeHealthPacket_t from ESP-NOW (already validated). */
void nodeHealthOnPacket(const uint8_t* data, int len);

/** Derive dock node slot from dock beacon (no separate health packet required). */
void nodeHealthOnDockBeacon(const DockBeaconPacket_t* bp);

/** Vision node seen (ESP-NOW vision packets). */
void nodeHealthMarkVisionSeen(void);

/** Call from main loop: refresh local BASE, broadcast health, age-out slots. */
void nodeHealthTick(void);

String nodeHealthGetJSON(void);

/** True if CYD master heartbeat is stale (optional motion failsafe). */
bool nodeHealthIsMasterStale(void);

/** True if node slot has been seen recently (same timeout as JSON /api). */
bool nodeHealthIsOnline(uint8_t node_id);

/** Flags from last packet (0 if never seen). */
uint16_t nodeHealthGetFlags(uint8_t node_id);
