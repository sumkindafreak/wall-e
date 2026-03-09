/*******************************************************************************
 * dock_espnow.cpp
 * ESP-NOW beacon at 10 Hz
 ******************************************************************************/

#include "dock_espnow.h"
#include "dock_config.h"
#include "dock_state.h"
#include "dock_protocol.h"
#include <esp_wifi.h>
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static esp_now_peer_info_t peer = {};
static uint32_t g_last_send_ms = 0;
static bool g_last_ok = false;
static uint32_t g_send_ok = 0;
static uint32_t g_send_fail = 0;

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);

static void onSendDone(const uint8_t *mac, esp_now_send_status_t status) {
  (void)mac;
  g_last_ok = (status == ESP_NOW_SEND_SUCCESS);
  if (g_last_ok) g_send_ok++; else g_send_fail++;
}

bool dockEspNowBegin(void) {
  WiFi.mode(WIFI_STA);
  delay(100);

  /* Connect to home WiFi: NVS (from WALL-E) first, else compile-time */
  Preferences prefs;
  prefs.begin("dock_wifi", true);
  String nvsSsid = prefs.getString("sta_ssid", "");
  String nvsPass = prefs.getString("sta_pass", "");
  prefs.end();

  const char *ssid = NULL;
  const char *pass = NULL;
  if (nvsSsid.length() > 0) {
    ssid = nvsSsid.c_str();
    pass = nvsPass.c_str();
    Serial.print(F("[DOCK] Using WiFi from WALL-E: "));
  } else if (strlen(WIFI_HOME_SSID) > 0) {
    ssid = WIFI_HOME_SSID;
    pass = WIFI_HOME_PASSWORD;
    Serial.print(F("[DOCK] Using WiFi from config: "));
  }

  if (ssid && ssid[0] != '\0') {
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 12000) {
      delay(200);
      Serial.print('.');
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print(F("[DOCK] Home WiFi OK: "));
      Serial.println(WiFi.localIP());
      configTime(TIMEZONE_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
      Serial.println(F("[DOCK] NTP configured"));
    } else {
      Serial.println(F(" (timeout)"));
    }
  } else {
    WiFi.disconnect();
    delay(100);
  }

  if (esp_now_init() != ESP_OK) {
    return false;
  }

  esp_now_register_send_cb(onSendDone);
  esp_now_register_recv_cb(onRecv);

  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, broadcast_mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    return false;
  }

  return true;
}

void dockEspNowSendBeacon(const DockBeaconPacket_t *pkt) {
  uint32_t now = millis();
  if (now - g_last_send_ms < ESPNOW_BEACON_INTERVAL_MS) {
    return;
  }
  g_last_send_ms = now;

  esp_err_t r = esp_now_send(broadcast_mac, (const uint8_t *)pkt, DOCK_BEACON_SIZE);
  if (r != ESP_OK) {
    g_last_ok = false;
    g_send_fail++;
  }
}

bool dockEspNowLastSendOk(void) {
  return g_last_ok;
}

void dockEspNowGetStats(uint32_t *ok, uint32_t *fail) {
  if (ok) *ok = g_send_ok;
  if (fail) *fail = g_send_fail;
}

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  dockEspNowHandleRecv(data, len);
}

bool dockEspNowHandleRecv(const uint8_t *data, int len) {
  if (len < 4) return false;
  uint32_t magic = *(const uint32_t *)data;
  if (magic != DOCK_CMD_MAGIC) return false;

  /* Time sync from WALL-E */
  if (len >= (int)sizeof(DockTimePacket_t)) {
    const DockTimePacket_t *tp = (const DockTimePacket_t *)data;
    if (tp->cmd == DOCK_CMD_TIME && (tp->dock_id == 0 || tp->dock_id == DOCK_ID)) {
      struct timeval tv = { (time_t)tp->unix_time, 0 };
      settimeofday(&tv, NULL);
      Serial.print(F("[DOCK] Time set from WALL-E: "));
      Serial.println((unsigned long)tp->unix_time);
      return true;
    }
  }

  if (len >= (int)sizeof(DockWifiConfigPacket_t)) {
    const DockWifiConfigPacket_t *p = (const DockWifiConfigPacket_t *)data;
    if (p->cmd == DOCK_CMD_WIFI_CONFIG && (p->dock_id == 0 || p->dock_id == DOCK_ID)) {
      if (p->ssid[0] == '\0') return true;
      Preferences prefs;
      prefs.begin("dock_wifi", false);
      prefs.putString("sta_ssid", String(p->ssid));
      prefs.putString("sta_pass", String(p->pass));
      prefs.end();
      Serial.print(F("[DOCK] WiFi config received from WALL-E: "));
      Serial.println(p->ssid);
      WiFi.disconnect(true);
      delay(200);
      WiFi.begin(p->ssid, p->pass);
      Serial.println(F("[DOCK] Reconnecting..."));
      return true;
    }
  }

  if (len < (int)sizeof(DockCommandPacket_t)) return false;
  const DockCommandPacket_t *p = (const DockCommandPacket_t *)data;
  if (p->dock_id != 0 && p->dock_id != DOCK_ID) return false;
  if (p->cmd == DOCK_CMD_FORCE_OFF) {
    dockStateForceOff();
    Serial.println(F("[DOCK] CMD: Force OFF (from WALL-E)"));
  } else if (p->cmd == DOCK_CMD_RESET) {
    dockStateResetFault();
    Serial.println(F("[DOCK] CMD: Reset fault (from WALL-E)"));
  }
  return true;
}

