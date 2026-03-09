#pragma once

#include <cstdint>

// ============================================================
//  WALL-E Dock Controller
//  Sends ESP-NOW commands to Smart Charging Crate (dock_station)
//  Requires same WiFi channel for ESP-NOW to reach dock
// ============================================================

/* Call after WiFi/ESP-NOW init (espnowReceiverInit) */
void dockControllerInit(void);

/* Send command to dock. dock_id: 0 = any, or specific ID. Returns true if send initiated. */
bool dockControllerSendForceOff(uint32_t dock_id);
bool dockControllerSendReset(uint32_t dock_id);

/* Share WiFi credentials with dock (ssid/pass from WALL-E's NVS). dock_id: 0 = any. */
bool dockControllerSendWifiConfig(const char* ssid, const char* pass, uint32_t dock_id);

/* Share current time with dock (Unix timestamp). Call when NTP synced. */
bool dockControllerSendTime(uint32_t unix_time, uint32_t dock_id);
