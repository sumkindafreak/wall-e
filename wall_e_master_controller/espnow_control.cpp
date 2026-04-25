// ============================================================
//  WALL-E Master Controller — ESP-NOW Control Implementation
// ============================================================

#include "espnow_control.h"
#include "audio_protocol.h"
#include "node_health_protocol.h"
#include "sd_manager.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>

// ============================================================
//  PEER MAC ADDRESS CONFIGURATION
//
//  Broadcast: Works with any Base, AP-only or home WiFi connected
//  Specific MAC: For lockdown — use Base Serial "Use this MAC for controller"
// ============================================================

// Broadcast — works even when Base is connected to home WiFi
static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Specific MAC (uncomment and set from Base Serial output):
// static uint8_t s_peerMac[6] = {0x22, 0x6E, 0xF1, 0x98, 0xB5, 0x38};  // Base AP MAC
static TelemetryPacket s_telemetry;
static unsigned long s_lastSendMs = 0;
static unsigned long s_lastTelemMs = 0;
static bool s_sendOk = false;
static uint16_t s_sendCount = 0;
static unsigned long s_rateWindowStartMs = 0;
static uint16_t s_lastRate = 0;
static uint16_t s_sendFailCount = 0;
static unsigned long s_lastDiagnosticMs = 0;
static unsigned long s_lastNodeHealthMs = 0;

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  s_sendOk = (status == ESP_NOW_SEND_SUCCESS);
  if (!s_sendOk) {
    s_sendFailCount++;
  }
}

static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len < (int)WALLE_TELEMETRY_SIZE_V1) return;
  TelemetryPacket pkt = {};
  size_t cpy = (size_t)len < sizeof(TelemetryPacket) ? (size_t)len : sizeof(TelemetryPacket);
  memcpy(&pkt, data, cpy);
  if (pkt.magic != WALLE_TELEMETRY_MAGIC) return;
  if (pkt.version < 1) return;
  if (cpy < sizeof(TelemetryPacket) || pkt.version < WALLE_TELEMETRY_VERSION) {
    pkt.last_control_seq_applied = 0;
    pkt.control_ack_bits = 0;
    pkt.rsv0 = 0;
  }
  s_telemetry = pkt;
  s_lastTelemMs = millis();
}

void espnowInit(void) {
  s_rateWindowStartMs = millis();
  s_sendCount = 0;
  s_lastRate = 0;
  s_sendFailCount = 0;
  s_lastDiagnosticMs = millis();
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // CRITICAL: Set WiFi channel to match Brain's AP (Brain is on channel 11)
  int channel = 11;  // Changed from 1 to 11
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  // Boost WiFi power for better range
  esp_wifi_set_max_tx_power(84); // Max power (20.5 dBm)
  
  Serial.print("[ESP-NOW] Controller MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("[ESP-NOW] WiFi Channel: %d\n", channel);
  Serial.printf("[ESP-NOW] TX Power: MAX (84/84)\n");
  Serial.print("[ESP-NOW] Target Brain MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", s_peerMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  if (s_peerMac[0] == 0xFF) {
    Serial.println("[ESP-NOW] Broadcast mode — works with Base on AP or home WiFi");
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
  peer.ifidx = WIFI_IF_STA;
  
  esp_err_t result = esp_now_add_peer(&peer);
  if (result == ESP_OK) {
    Serial.println("[ESP-NOW] ✓ Peer added successfully");
  } else {
    Serial.printf("[ESP-NOW] ❌ Failed to add peer: %d\n", result);
  }

  memset(&s_telemetry, 0, sizeof(s_telemetry));
  s_telemetry.magic = WALLE_TELEMETRY_MAGIC;
  s_telemetry.version = WALLE_TELEMETRY_VERSION;
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
  
  // Connection diagnostics every 5 seconds
  if (now - s_lastDiagnosticMs > 5000) {
    bool telemValid = espnowTelemetryValid();
    float failRate = (s_sendCount > 0) ? (s_sendFailCount * 100.0f / s_sendCount) : 0;
    
    Serial.printf("[ESP-NOW] Status: Send=%d/s Fail=%d (%.1f%%) Telem=%s LastRx=%lums\n",
      s_lastRate, s_sendFailCount, failRate, 
      telemValid ? "OK" : "LOST", now - s_lastTelemMs);
    
    if (failRate > 50.0f) {
      Serial.println("[ESP-NOW] ⚠️  HIGH PACKET LOSS - Check:");
      Serial.println("  1. Brain is powered on and running");
      Serial.println("  2. Brain MAC address matches (line 31)");
      Serial.println("  3. Brain is on WiFi channel 11");
      Serial.println("  4. Distance < 50m, no metal obstacles");
    }

    if (s_sendFailCount > 20 && s_peerMac[0] != 0xFF) {
      esp_err_t d = esp_now_del_peer(s_peerMac);
      esp_now_peer_info_t p = {};
      memcpy(p.peer_addr, s_peerMac, 6);
      p.channel = 11;
      p.encrypt = false;
      p.ifidx = WIFI_IF_STA;
      esp_err_t a = esp_now_add_peer(&p);
      Serial.printf("[ESP-NOW] Peer refresh (fails=%u) del=%d add=%d\n", (unsigned)s_sendFailCount, (int)d, (int)a);
    }
    
    if (!telemValid && (now - s_lastTelemMs > 10000)) {
      Serial.println("[ESP-NOW] ⚠️  NO TELEMETRY - Brain may not be sending back");
    }
    
    s_sendFailCount = 0;
    s_lastDiagnosticMs = now;
  }
  
  // Clear stale telemetry
  if (now - s_lastTelemMs > ESPNOW_TELEM_TIMEOUT_MS) {
    s_telemetry.batteryVoltage = 0;
    s_telemetry.safetyState = 0xFF;
    s_telemetry.magic = WALLE_TELEMETRY_MAGIC;
    s_telemetry.version = WALLE_TELEMETRY_VERSION;
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
    h.free_heap = ESP.getFreeHeap();
    h.loop_p95_ms = 0;
    h.wifi_rssi = (int8_t)WiFi.RSSI();
    h.sd_ok = sdIsAvailable() ? 1u : 0u;
    if (espnowTelemetryValid()) {
      h.last_peer_rx_age_ms = (uint32_t)(now - s_lastTelemMs);
    } else {
      h.last_peer_rx_age_ms = 0xFFFFFFFFu;
    }
    esp_now_send(s_peerMac, (uint8_t*)&h, sizeof(h));
  }
}

bool espnowIsConnected(void) {
  return s_sendOk;
}

bool espnowTelemetryValid(void) {
  return (millis() - s_lastTelemMs) < ESPNOW_TELEM_TIMEOUT_MS;
}

bool espnowTelemetryLinkDegraded(void) {
  if (!espnowTelemetryValid()) {
    return false;
  }
  return (s_telemetry.rsv0 & (WALLE_TELEM_RSV0_SEQ_GAP | WALLE_TELEM_RSV0_TELEM_TX_FAIL)) != 0;
}

void espnowGetTelemetry(TelemetryPacket* out) {
  if (out) memcpy(out, &s_telemetry, sizeof(TelemetryPacket));
}

void espnowSetPeerMac(const uint8_t mac[6]) {
  if (mac) memcpy(s_peerMac, mac, 6);
}

void espnowBroadcastAudioEstopEdge(bool estop_pressed)
{
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
    esp_now_send(bcast, (uint8_t *)&pkt, sizeof(pkt));
  }
  prev = estop_pressed;
}

void espnowBroadcastAudioPlayTrack(uint8_t track) {
  if (track == 0) {
    return;
  }
  uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  WalleAudioCommandPacket_t pkt = {
      .magic = WALLE_AUDIO_MAGIC,
      .cmd = WALLE_AU_CMD_PLAY_TRACK,
      .param = track,
      .reserved = 0,
      .priority = WALLE_AUDIO_PRIORITY_NORMAL,
  };
  esp_now_send(bcast, (uint8_t*)&pkt, sizeof(pkt));
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
