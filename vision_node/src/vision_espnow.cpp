/**
 * vision_espnow.cpp - ESP-NOW transmit of VisionPacket
 *
 * WALL-E's Base AP/CYD radio contract uses Wi-Fi channel 11.  The vision
 * node therefore either shares the channel of its active Base Wi-Fi link,
 * or explicitly falls back to channel 11 when the AP is unavailable.
 */
#include "vision_espnow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>

static constexpr uint8_t VISION_ESPNOW_CHANNEL = 11;
static uint8_t s_broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_init = false;

static void onSendCb(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  (void)status;
}

bool visionEspNowInit(void) {
  if (s_init) return true;

  // Keep an existing WALL-E-Control Wi-Fi connection alive.  Disconnecting
  // here would disable the /snapshot server and can move the radio away from
  // the Base Brain's ESP-NOW channel.
  WiFi.mode(WIFI_STA);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[Vision] ESP-NOW sharing WiFi channel %d\n", WiFi.channel());
  } else {
    esp_err_t channelErr = esp_wifi_set_channel(VISION_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (channelErr != ESP_OK) {
      Serial.printf("[Vision] WARN: could not set ESP-NOW channel %u (0x%x)\n",
                    (unsigned)VISION_ESPNOW_CHANNEL, (unsigned)channelErr);
    } else {
      Serial.printf("[Vision] ESP-NOW fallback channel %u\n",
                    (unsigned)VISION_ESPNOW_CHANNEL);
    }
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("[Vision] ESP-NOW init failed");
    return false;
  }

  esp_now_register_send_cb(onSendCb);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_broadcastMac, 6);
  // Channel 0 means use the station interface's current channel.  That keeps
  // ESP-NOW and WALL-E-Control Wi-Fi on the same physical radio channel.
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("[Vision] ESP-NOW broadcast peer failed");
    esp_now_deinit();
    return false;
  }

  s_init = true;
  Serial.println("[Vision] ESP-NOW ready");
  return true;
}

bool visionEspNowSend(const VisionPacket_t* pkt) {
  if (!s_init || !pkt) return false;
  return (esp_now_send(s_broadcastMac, (const uint8_t*)pkt, VISION_PACKET_SIZE) == ESP_OK);
}
