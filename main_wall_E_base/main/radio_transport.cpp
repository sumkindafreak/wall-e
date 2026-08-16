#include "radio_transport.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>
#include <cstring>

// ============================================================
// WALL-E ESP32-S3 native ESP-NOW transport
// ============================================================

static WalleRadioReceiveCallback s_receiveCallback = nullptr;
static bool s_started = false;
static uint32_t s_lastRxMs = 0;
static uint32_t s_rxCount = 0;
static uint32_t s_txCount = 0;
static uint32_t s_txFailures = 0;
static uint8_t s_channel = WALLE_RADIO_CHANNEL;

void radioTransportSetReceiveCallback(WalleRadioReceiveCallback callback) {
  s_receiveCallback = callback;
}

uint8_t radioTransportGetChannel(void) { return s_channel; }
uint32_t radioTransportGetLastRxMs(void) { return s_lastRxMs; }
uint32_t radioTransportGetRxCount(void) { return s_rxCount; }
uint32_t radioTransportGetTxCount(void) { return s_txCount; }
uint32_t radioTransportGetTxFailureCount(void) { return s_txFailures; }

static wifi_interface_t activeInterface(void) {
  const wifi_mode_t mode = WiFi.getMode();
  return (mode == WIFI_AP || mode == WIFI_AP_STA) ? WIFI_IF_AP : WIFI_IF_STA;
}

static bool ensurePeer(const uint8_t mac[6]) {
  if (!mac) return false;
  if (esp_now_is_peer_exist(mac)) return true;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = activeInterface();

  const esp_err_t result = esp_now_add_peer(&peer);
  return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

static void dispatchPacket(const uint8_t sourceMac[6],
                           const uint8_t* data,
                           int length,
                           int8_t rssi,
                           uint8_t channel) {
  if (!sourceMac || !data || length <= 0) return;

  s_lastRxMs = millis();
  ++s_rxCount;
  if (channel != 0) s_channel = channel;

  if (s_receiveCallback) {
    s_receiveCallback(sourceMac,
                      data,
                      (size_t)length,
                      rssi,
                      s_channel);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void nativeReceive(const esp_now_recv_info_t* info,
                          const uint8_t* data,
                          int length) {
  if (!info) return;
  const int8_t rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : -127;
  const uint8_t channel = info->rx_ctrl ? (uint8_t)info->rx_ctrl->channel
                                        : s_channel;
  dispatchPacket(info->src_addr, data, length, rssi, channel);
}
#else
static void nativeReceive(const uint8_t* sourceMac,
                          const uint8_t* data,
                          int length) {
  // Arduino-ESP32 2.x does not expose RSSI/channel metadata here.
  dispatchPacket(sourceMac, data, length, -127, s_channel);
}
#endif

bool radioTransportInit(void) {
  if (s_started) return true;

  // wifi_manager normally establishes AP/STA mode before this function is
  // called. If Wi-Fi has not been configured yet, create a STA interface so
  // ESP-NOW still has a valid radio interface.
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
    delay(20);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[Radio/S3] ESP-NOW init failed"));
    return false;
  }

  esp_now_register_recv_cb(nativeReceive);

  uint8_t primary = 0;
  wifi_second_chan_t secondary = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &secondary) == ESP_OK && primary != 0) {
    s_channel = primary;
  }

  s_started = true;
  Serial.printf("[Radio/S3] Native ESP-NOW ready on channel %u\n",
                (unsigned)s_channel);
  return true;
}

void radioTransportPoll(void) {
  // Native ESP-NOW receive is callback-driven.
}

bool radioTransportSend(const uint8_t destinationMac[6],
                        const void* data,
                        size_t length) {
  if (!s_started || !destinationMac || !data || length == 0 || length > 250u) {
    ++s_txFailures;
    return false;
  }

  if (!ensurePeer(destinationMac)) {
    ++s_txFailures;
    return false;
  }

  const esp_err_t result = esp_now_send(
      destinationMac,
      reinterpret_cast<const uint8_t*>(data),
      length);
  if (result != ESP_OK) {
    ++s_txFailures;
    return false;
  }

  ++s_txCount;
  return true;
}

bool radioTransportBroadcast(const void* data, size_t length) {
  static const uint8_t broadcastMac[6] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };
  return radioTransportSend(broadcastMac, data, length);
}

bool radioTransportIsReady(void) {
  return s_started;
}
