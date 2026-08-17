/*******************************************************************************
 * dock_espnow.h
 * ESP-NOW beacon broadcast for WALL-E homing
 ******************************************************************************/

#ifndef DOCK_ESPNOW_H
#define DOCK_ESPNOW_H

#include "dock_protocol.h"

/*
 * Call once in setup().
 * ESP-NOW is brought up immediately on WALL-E's dedicated radio channel and
 * does NOT depend on a home Wi-Fi connection. Optional home Wi-Fi/OTA is a
 * secondary service controlled by ENABLE_WIFI in dock_config.h.
 */
bool dockEspNowBegin(void);

/*
 * Call every loop. Maintains the optional home Wi-Fi service without allowing
 * it to strand the docking radio on a different channel. Returns true whenever
 * the ESP-NOW beacon transport is available.
 */
bool dockEspNowPoll(void);

/* Call every loop when dockEspNowPoll() is true. Sends beacon at 10 Hz. */
void dockEspNowSendBeacon(const DockBeaconPacket_t *pkt);

/** Periodic compact node health (same channel as beacon). */
void dockEspNowSendNodeHealth(void);

/* Get last send result: true = success, false = fail */
bool dockEspNowLastSendOk(void);

/* Get send success/fail counts (for debug stats) */
void dockEspNowGetStats(uint32_t *ok, uint32_t *fail);

/* Receive callback handler. Returns true if the packet was handled. */
bool dockEspNowHandleRecv(const uint8_t *data, int len);

#endif /* DOCK_ESPNOW_H */
