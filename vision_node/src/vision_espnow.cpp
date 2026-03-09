/**
 * vision_espnow.cpp - ESP-NOW transmit of VisionPacket
 */
#include "vision_espnow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

static uint8_t s_broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_init = false;

static void onSendCb(const uint8_t* mac, esp_now_send_status_t status) { (void)mac; (void)status; }

bool visionEspNowInit(void) {
  if (s_init) return true;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onSendCb);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_broadcastMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) return false;
  s_init = true;
  Serial.println("[Vision] ESP-NOW ready");
  return true;
}

bool visionEspNowSend(const VisionPacket_t* pkt) {
  if (!s_init || !pkt) return false;
  return (esp_now_send(s_broadcastMac, (const uint8_t*)pkt, VISION_PACKET_SIZE) == ESP_OK);
}
