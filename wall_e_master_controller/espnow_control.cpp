// ============================================================
//  WALL-E Master Controller — ESP-NOW Control Implementation
// ============================================================

#include "espnow_control.h"
#include "audio_protocol.h"
#include "node_health_protocol.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_idf_version.h>
#include <Arduino.h>
#include <string.h>

// ============================================================
//  PEER MAC ADDRESS CONFIGURATION
//
//  Broadcast: Works with any Base, AP-only or home WiFi connected
//  Specific MAC: For lockdown — use Base Serial "Use this MAC for controller"
// ============================================================

static constexpr uint8_t ESPNOW_CHANNEL = 11;
static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static TelemetryPacket s_telemetry;
static unsigned long s_lastSendMs = 0;
static unsigned long s_lastTelemMs = 0;
static bool s_sendOk = false;
static uint16_t s_sendCount = 0;
static unsigned long s_rateWindowStartMs = 0;
static uint16_t s_lastRate = 0;
static uint16_t s_sendFailCount = 0;
static uint32_t s_diagnosticSendCount = 0;
static unsigned long s_lastDiagnosticMs = 0;
static unsigned long s_lastNodeHealthMs = 0;

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  s_sendOk = (status == ESP_NOW_SEND_SUCCESS);
  if (!s_sendOk) {
    s_sendFailCount++;
  }
}

static void handleReceivedPacket(const uint8_t* data, int len) {
  if (!data || len <= 0) return;

  if (len >= (int)sizeof(TelemetryPacket)) {
    memcpy(&s_telemetry, data, sizeof(TelemetryPacket));
    s_lastTelemMs = millis();
  }
}

// Arduino-ESP32 2.x / ESP-IDF 4.x uses the legacy callback signature.
// Arduino-ESP32 3.x / ESP-IDF 5.x supplies esp_now_recv_info_t instead.
#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  handleReceivedPacket(data, len);
}
#else
static void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  handleReceivedPacket(data, len);
}
#endif

void espnowInit(void) {
  s_rateWindowStartMs = millis();
  s_sendCount = 0;
  s_lastRate = 0;
  s_sendFailCount = 0;
  s_diagnosticSendCount = 0;
  s_lastDiagnosticMs = millis();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // CRITICAL: match the WALL-E Base Brain AP/ESP-NOW channel.
  esp_wifi_set_promiscuous(true);
  const esp_err_t channelResult = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  if (channelResult != ESP_OK) {
    Serial.printf("[ESP-NOW] WARN: channel %u set failed: %d\n",
                  (unsigned)ESPNOW_CHANNEL, (int)channelResult);
  }

  // Boost WiFi power for better operator-controller range.
  esp_wifi_set_max_tx_power(84);

  Serial.print("[ESP-NOW] Controller MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("[ESP-NOW] WiFi Channel: %u\n", (unsigned)ESPNOW_CHANNEL);
  Serial.println("[ESP-NOW] TX Power: MAX (84/84)");
  Serial.print("[ESP-NOW] Target Brain MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", s_peerMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  if (s_peerMac[0] == 0xFF) {
    Serial.println("[ESP-NOW] Broadcast mode - works with WALL-E Base on channel 11");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init FAILED");
    return;
  }

  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_peerMac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;

  const esp_err_t result = esp_now_add_peer(&peer);
  if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) {
    Serial.println("[ESP-NOW] Peer ready");
  } else {
    Serial.printf("[ESP-NOW] Failed to add peer: %d\n", result);
  }

  memset(&s_telemetry, 0, sizeof(s_telemetry));
  s_lastTelemMs = 0;
}

void espnowSend(const ControlPacket* pkt) {
  if (!pkt) return;

  const esp_err_t result = esp_now_send(s_peerMac, (const uint8_t*)pkt, sizeof(ControlPacket));
  const unsigned long now = millis();
  s_lastSendMs = now;
  s_diagnosticSendCount++;

  if (result != ESP_OK) {
    s_sendOk = false;
    s_sendFailCount++;
  }

  if (now - s_rateWindowStartMs >= 1000) {
    s_lastRate = s_sendCount;
    s_sendCount = 0;
    s_rateWindowStartMs = now;
  }
  s_sendCount++;
}

void espnowUpdate(void) {
  const unsigned long now = millis();

  // Connection diagnostics every 5 seconds. Keep numerator and denominator
  // in the same diagnostic window so packet-loss percentages stay meaningful.
  if (now - s_lastDiagnosticMs > 5000) {
    const bool telemValid = espnowTelemetryValid();
    const float failRate = (s_diagnosticSendCount > 0)
      ? (s_sendFailCount * 100.0f / s_diagnosticSendCount)
      : 0.0f;

    Serial.printf("[ESP-NOW] Status: Send=%u/s Fail=%u (%.1f%%) Telem=%s LastRx=%lums\n",
                  (unsigned)s_lastRate,
                  (unsigned)s_sendFailCount,
                  failRate,
                  telemValid ? "OK" : "LOST",
                  s_lastTelemMs == 0 ? 0UL : now - s_lastTelemMs);

    if (failRate > 50.0f) {
      Serial.println("[ESP-NOW] HIGH PACKET LOSS - check Base power, channel 11 and range");
    }

    if (!telemValid && (s_lastTelemMs == 0 || now - s_lastTelemMs > 10000)) {
      Serial.println("[ESP-NOW] NO TELEMETRY - WALL-E Base may not be replying");
    }

    s_sendFailCount = 0;
    s_diagnosticSendCount = 0;
    s_lastDiagnosticMs = now;
  }

  // Clear stale telemetry.
  if (!espnowTelemetryValid()) {
    s_telemetry.batteryVoltage = 0;
    s_telemetry.safetyState = 0xFF;
  }

  if (now - s_lastNodeHealthMs >= 1000) {
    s_lastNodeHealthMs = now;
    WalleNodeHealthPacket_t h = {};
    h.magic = WALLE_NODE_HEALTH_MAGIC;
    h.version = WALLE_NODE_HEALTH_VERSION;
    h.node_id = WALLE_NODE_MASTER;
    h.role = WALLE_ROLE_OPERATOR;
    h.battery_pct = WALLE_NODE_HEALTH_UNKNOWN_BAT;
    h.temp_c = WALLE_NODE_HEALTH_UNKNOWN_TEMP;
    h.uptime_ms = (uint32_t)now;
    h.last_error = 0;
    h.flags = 0;
    esp_now_send(s_peerMac, (const uint8_t*)&h, sizeof(h));
  }
}

bool espnowIsConnected(void) {
  return s_sendOk;
}

bool espnowTelemetryValid(void) {
  return s_lastTelemMs != 0 && (millis() - s_lastTelemMs) < ESPNOW_TELEM_TIMEOUT_MS;
}

void espnowGetTelemetry(TelemetryPacket* out) {
  if (out) memcpy(out, &s_telemetry, sizeof(TelemetryPacket));
}

void espnowSetPeerMac(const uint8_t mac[6]) {
  if (mac) memcpy(s_peerMac, mac, 6);
}

void espnowBroadcastAudioEstopEdge(bool estop_pressed) {
  static bool prev = false;
  if (estop_pressed && !prev) {
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    WalleAudioCommandPacket_t pkt = {
      .magic = WALLE_AUDIO_MAGIC,
      .cmd = WALLE_AU_CMD_PLAY_EVENT,
      .param = WALLE_AUDIO_EVT_ESTOP,
      .reserved = 0,
      .priority = WALLE_AUDIO_PRIORITY_ESTOP,
    };
    esp_now_send(bcast, (const uint8_t*)&pkt, sizeof(pkt));
  }
  prev = estop_pressed;
}

uint16_t espnowGetPacketRate(void) {
  const unsigned long now = millis();
  if (now - s_rateWindowStartMs >= 1000) {
    s_lastRate = s_sendCount;
    s_sendCount = 0;
    s_rateWindowStartMs = now;
  }
  return s_lastRate;
}
