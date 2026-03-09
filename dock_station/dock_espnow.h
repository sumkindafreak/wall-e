/*******************************************************************************
 * dock_espnow.h
 * ESP-NOW beacon broadcast for WALL-E homing
 ******************************************************************************/

#ifndef DOCK_ESPNOW_H
#define DOCK_ESPNOW_H

#include "dock_protocol.h"

/* Call once after WiFi.init(). Initializes ESP-NOW and broadcast peer. */
bool dockEspNowBegin(void);

/* Call every loop. Sends beacon at 10 Hz (internal rate limiting). */
void dockEspNowSendBeacon(const DockBeaconPacket_t *pkt);

/* Get last send result: true = success, false = fail */
bool dockEspNowLastSendOk(void);

/* Get send success/fail counts (for debug stats) */
void dockEspNowGetStats(uint32_t *ok, uint32_t *fail);

/* Receive callback - call from main or register in dockEspNowBegin.
   Pass len and data; returns true if handled. */
bool dockEspNowHandleRecv(const uint8_t *data, int len);

#endif /* DOCK_ESPNOW_H */
