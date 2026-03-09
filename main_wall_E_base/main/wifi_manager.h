#pragma once

// ============================================================
//  WALL-E WiFi Manager
//  Handles AP-first boot, STA connection, NVS credential storage
// ============================================================

#include <Arduino.h>

// AP always-on settings
#define AP_SSID       "WALL-E-Control"
#define AP_PASSWORD   "walle1234"

// STA connection timeout
#define WIFI_CONNECT_TIMEOUT_MS 12000UL

// WiFi status codes returned to web clients
enum WiFiState {
  WS_AP_ONLY       = 0,   // AP running, no STA
  WS_CONNECTING    = 1,   // STA connection in progress
  WS_CONNECTED     = 2,   // AP + STA both running
  WS_FAILED        = 3    // STA failed, AP only
};

void     wifiManagerInit();                                    // Called once in setup()
void     wifiManagerHandle();                                  // Called in loop() — handles async connect

bool     wifiConnectSTA(const char* ssid, const char* pass);  // Connect to home WiFi (async start)
void     wifiDisconnectSTA();                                  // Drop STA, stay on AP
void     wifiClearCredentials();                               // Wipe NVS credentials

WiFiState  wifiGetState();
String     wifiGetSTA_IP();
String     wifiGetAP_IP();
String     wifiGetSTA_SSID();
String     wifiGetStatusJSON();   // Returns full status as JSON string
String     wifiScanJSON();        // Triggers scan, returns JSON array
