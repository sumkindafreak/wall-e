// ============================================================
//  WALL-E Master Controller — ESP-NOW Control Implementation
// ============================================================

#include "espnow_control.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>

// ============================================================
//  PEER MAC ADDRESS CONFIGURATION
//  
//  Set this to your WALL-E Brain's MAC address.
//  
//  How to find it:
//  1. Upload code to Brain (main.ino)
//  2. Open Serial Monitor at 115200 baud
//  3. Look for: "[ESP-NOW] Receiver ready. MAC: XX:XX:XX:XX:XX:XX"
//  4. Copy that MAC here
//
//  Broadcast mode (current): Works with any ESP-NOW device
//  Specific MAC: More secure, less interference
// ============================================================

// OPTION 1: Broadcast (default - works immediately, less secure)
// static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// OPTION 2: Specific MAC (recommended - uncomment and set your Brain's MAC)
// Brain MAC: 20:6E:F1:98:B5:38 (WiFi Station)
static uint8_t s_peerMac[6] = {0x20, 0x6E, 0xF1, 0x98, 0xB5, 0x38};
static TelemetryPacket s_telemetry;
static unsigned long s_lastSendMs = 0;
static unsigned long s_lastTelemMs = 0;
static bool s_sendOk = false;
static uint16_t s_sendCount = 0;
static unsigned long s_rateWindowStartMs = 0;
static uint16_t s_lastRate = 0;

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  s_sendOk = (status == ESP_NOW_SEND_SUCCESS);
}

static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len >= (int)sizeof(TelemetryPacket)) {
    memcpy(&s_telemetry, data, sizeof(TelemetryPacket));
    s_lastTelemMs = millis();
  }
}

void espnowInit(void) {
  s_rateWindowStartMs = millis();
  s_sendCount = 0;
  s_lastRate = 0;
  WiFi.mode(WIFI_STA);
  
  // CRITICAL: Set WiFi channel to match Brain's AP (Brain is on channel 11)
  int channel = 11;  // Changed from 1 to 11
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  Serial.print("[ESP-NOW] Controller MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("[ESP-NOW] WiFi Channel: %d\n", channel);
  Serial.print("[ESP-NOW] Target Brain MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", s_peerMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  if (s_peerMac[0] == 0xFF) {
    Serial.println("[ESP-NOW] WARNING: Broadcast mode - set specific MAC for production");
  }
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED");
    return;
  }
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_peerMac, 6);
  peer.channel = channel;  // Use channel 11
  peer.encrypt = false;
  esp_err_t result = esp_now_add_peer(&peer);
  if (result == ESP_OK) {
    Serial.println("[ESP-NOW] Peer added successfully");
  } else {
    Serial.printf("[ESP-NOW] Failed to add peer: %d\n", result);
  }

  memset(&s_telemetry, 0, sizeof(s_telemetry));
}

void espnowSend(const ControlPacket* pkt) {
  if (!pkt) return;
  esp_now_send(s_peerMac, (uint8_t*)pkt, sizeof(ControlPacket));
  unsigned long now = millis();
  s_lastSendMs = now;
  if (now - s_rateWindowStartMs >= 1000) {
    s_lastRate = s_sendCount;
    s_sendCount = 0;
    s_rateWindowStartMs = now;
  }
  s_sendCount++;
}

void espnowUpdate(void) {
  unsigned long now = millis();
  if (now - s_lastTelemMs > ESPNOW_TELEM_TIMEOUT_MS) {
    s_telemetry.batteryVoltage = 0;
    s_telemetry.safetyState = 0xFF;
  }
}

bool espnowIsConnected(void) {
  return s_sendOk;
}

bool espnowTelemetryValid(void) {
  return (millis() - s_lastTelemMs) < ESPNOW_TELEM_TIMEOUT_MS;
}

void espnowGetTelemetry(TelemetryPacket* out) {
  if (out) memcpy(out, &s_telemetry, sizeof(TelemetryPacket));
}

void espnowSetPeerMac(const uint8_t mac[6]) {
  if (mac) memcpy(s_peerMac, mac, 6);
}

uint16_t espnowGetPacketRate(void) {
  unsigned long now = millis();
  if (now - s_rateWindowStartMs >= 1000) {
    s_lastRate = s_sendCount;
    s_sendCount = 0;
    s_rateWindowStartMs = now;
  }
  return s_lastRate;
}
