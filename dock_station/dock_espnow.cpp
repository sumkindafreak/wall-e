/*******************************************************************************
 * dock_espnow.cpp
 * ESP-NOW beacon at 10 Hz
 *
 * Docking radio is the primary service and always uses WALL-E channel 11.
 * Home Wi-Fi is optional and is never allowed to move the dock permanently
 * away from the ESP-NOW channel used by WALL-E.
 ******************************************************************************/

#include "dock_espnow.h"
#include "dock_config.h"
#include "dock_state.h"
#include "dock_protocol.h"
#include "node_health_protocol.h"
#include "dock_alignment.h"
#include <esp_wifi.h>
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

static constexpr uint8_t DOCK_ESPNOW_CHANNEL = 11;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000;

static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static esp_now_peer_info_t peer = {};
static uint32_t g_last_send_ms = 0;
static uint32_t g_last_health_ms = 0;
static bool g_last_ok = false;
static uint32_t g_send_ok = 0;
static uint32_t g_send_fail = 0;
static bool g_espnow_inited = false;

#if ENABLE_WIFI
static bool g_wifi_started = false;
static bool g_wifi_announced = false;
static uint32_t g_wifi_started_ms = 0;
static char g_pending_ssid[33] = {0};
static char g_pending_pass[65] = {0};
static volatile bool g_wifi_config_pending = false;
#endif

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len);
#endif

static void onSendDone(const uint8_t *mac, esp_now_send_status_t status) {
  (void)mac;
  g_last_ok = (status == ESP_NOW_SEND_SUCCESS);
  if (g_last_ok) g_send_ok++; else g_send_fail++;
}

static bool forceDockRadioChannel(void) {
  esp_err_t err = esp_wifi_set_channel(DOCK_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (err != ESP_OK) {
    Serial.printf("[DOCK] WARN: channel %u set failed: 0x%x\n",
                  (unsigned)DOCK_ESPNOW_CHANNEL, (unsigned)err);
    return false;
  }
  return true;
}

static uint8_t currentRadioChannel(void) {
  uint8_t primary = 0;
  wifi_second_chan_t secondary = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &secondary) != ESP_OK) return 0;
  return primary;
}

static bool initEspNowTransport(void) {
  if (g_espnow_inited) return true;

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[DOCK] ESP-NOW init failed"));
    return false;
  }

  esp_now_register_send_cb(onSendDone);
  esp_now_register_recv_cb(onRecv);

  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, broadcast_mac, sizeof(broadcast_mac));
  peer.channel = 0;  // Use the station interface's current channel.
  peer.encrypt = false;

  esp_err_t peerResult = esp_now_add_peer(&peer);
  if (peerResult != ESP_OK && peerResult != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("[DOCK] ESP-NOW broadcast peer failed: 0x%x\n",
                  (unsigned)peerResult);
    esp_now_deinit();
    return false;
  }

  g_espnow_inited = true;
  Serial.printf("[DOCK] ESP-NOW ready on channel %u\n",
                (unsigned)currentRadioChannel());
  return true;
}

#if ENABLE_WIFI
static void startOptionalWifi(const char *ssid, const char *pass) {
  if (!ssid || ssid[0] == '\0') return;

  Serial.printf("[DOCK] Optional home WiFi connecting to '%s'\n", ssid);
  WiFi.begin(ssid, pass ? pass : "");
  g_wifi_started = true;
  g_wifi_announced = false;
  g_wifi_started_ms = millis();
}

static void loadOptionalWifi(void) {
  Preferences prefs;
  prefs.begin("dock_wifi", true);
  String nvsSsid = prefs.getString("sta_ssid", "");
  String nvsPass = prefs.getString("sta_pass", "");
  prefs.end();

  if (nvsSsid.length() > 0) {
    Serial.println(F("[DOCK] Optional WiFi credentials loaded from NVS"));
    startOptionalWifi(nvsSsid.c_str(), nvsPass.c_str());
  } else if (strlen(WIFI_HOME_SSID) > 0 && strcmp(WIFI_HOME_SSID, "YourNetwork") != 0) {
    Serial.println(F("[DOCK] Optional WiFi credentials loaded from config"));
    startOptionalWifi(WIFI_HOME_SSID, WIFI_HOME_PASSWORD);
  } else {
    Serial.println(F("[DOCK] Optional home WiFi not configured; ESP-NOW remains active"));
  }
}

static void applyPendingWifiConfig(void) {
  if (!g_wifi_config_pending) return;

  char ssid[sizeof(g_pending_ssid)];
  char pass[sizeof(g_pending_pass)];

  noInterrupts();
  memcpy(ssid, g_pending_ssid, sizeof(ssid));
  memcpy(pass, g_pending_pass, sizeof(pass));
  g_wifi_config_pending = false;
  interrupts();

  ssid[sizeof(ssid) - 1] = '\0';
  pass[sizeof(pass) - 1] = '\0';
  if (ssid[0] == '\0') return;

  Preferences prefs;
  prefs.begin("dock_wifi", false);
  prefs.putString("sta_ssid", String(ssid));
  prefs.putString("sta_pass", String(pass));
  prefs.end();

  WiFi.disconnect(false, false);
  delay(5);
  forceDockRadioChannel();
  startOptionalWifi(ssid, pass);
}

static void maintainOptionalWifi(void) {
  applyPendingWifiConfig();

  if (!g_wifi_started) return;

  if (WiFi.status() == WL_CONNECTED) {
    const uint8_t channel = (uint8_t)WiFi.channel();
    if (channel != DOCK_ESPNOW_CHANNEL) {
      Serial.printf(
        "[DOCK] Home WiFi is channel %u, but docking requires channel %u; disconnecting home WiFi\n",
        (unsigned)channel, (unsigned)DOCK_ESPNOW_CHANNEL);
      WiFi.disconnect(false, false);
      g_wifi_started = false;
      g_wifi_announced = false;
      delay(5);
      forceDockRadioChannel();
      return;
    }

    if (!g_wifi_announced) {
      g_wifi_announced = true;
      Serial.print(F("[DOCK] Optional home WiFi OK: "));
      Serial.println(WiFi.localIP());
      configTime(TIMEZONE_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
    }
    return;
  }

  if (millis() - g_wifi_started_ms >= WIFI_CONNECT_TIMEOUT_MS) {
    Serial.println(F("[DOCK] Home WiFi timeout; restoring dedicated docking channel"));
    WiFi.disconnect(false, false);
    g_wifi_started = false;
    g_wifi_announced = false;
    delay(5);
    forceDockRadioChannel();
  }
}
#endif

bool dockEspNowBegin(void) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  delay(20);

  // ESP-NOW is essential to docking. It comes up whether or not the dock has
  // home-network credentials.
  forceDockRadioChannel();
  if (!initEspNowTransport()) {
    return false;
  }

#if ENABLE_WIFI
  loadOptionalWifi();
#else
  Serial.println(F("[DOCK] Home WiFi/OTA disabled; dedicated ESP-NOW remains enabled"));
#endif

  return true;
}

bool dockEspNowPoll(void) {
#if ENABLE_WIFI
  maintainOptionalWifi();
#endif

  // When no optional STA connection owns the radio, actively repair the
  // channel if anything external moved it.
#if ENABLE_WIFI
  if (WiFi.status() != WL_CONNECTED)
#endif
  {
    const uint8_t channel = currentRadioChannel();
    if (channel != 0 && channel != DOCK_ESPNOW_CHANNEL) {
      Serial.printf("[DOCK] Radio drifted to channel %u; restoring channel %u\n",
                    (unsigned)channel, (unsigned)DOCK_ESPNOW_CHANNEL);
      forceDockRadioChannel();
    }
  }

  return g_espnow_inited;
}

void dockEspNowSendNodeHealth(void) {
  if (!g_espnow_inited) return;
  uint32_t now = millis();
  if (now - g_last_health_ms < 1000) return;
  g_last_health_ms = now;

  DockState st = dockStateGet();
  WalleNodeHealthPacket_t h = {};
  h.magic = WALLE_NODE_HEALTH_MAGIC;
  h.version = WALLE_NODE_HEALTH_VERSION;
  h.node_id = WALLE_NODE_DOCK;
  h.role = WALLE_ROLE_SENSOR;
  h.battery_pct = WALLE_NODE_HEALTH_UNKNOWN_BAT;
  h.temp_c = WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  h.uptime_ms = now;
  h.last_error = (st == STATE_FAULT) ? 1u : 0u;

  uint16_t f = 0;
  if (st == STATE_CHARGING) f |= WALLE_NODE_FLAG_CHARGING;
  if (st >= STATE_DOCKED_IDLE && st != STATE_NOT_DOCKED && st != STATE_BOOT) {
    f |= WALLE_NODE_FLAG_DOCKED;
  }
  if (st == STATE_FAULT) f |= WALLE_NODE_FLAG_FAULT;
  h.flags = f;

  esp_now_send(broadcast_mac, (uint8_t*)&h, sizeof(h));
}

void dockEspNowSendBeacon(const DockBeaconPacket_t *pkt) {
  if (!g_espnow_inited || !pkt) return;

  uint32_t now = millis();
  if (now - g_last_send_ms < ESPNOW_BEACON_INTERVAL_MS) return;
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

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  dockEspNowHandleRecv(data, len);
}
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  dockEspNowHandleRecv(data, len);
}
#endif

bool dockEspNowHandleRecv(const uint8_t *data, int len) {
  if (!data || len < 4) return false;

  uint32_t magic = 0;
  memcpy(&magic, data, sizeof(magic));
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
#if ENABLE_WIFI
      if (p->ssid[0] != '\0') {
        memcpy(g_pending_ssid, p->ssid, sizeof(g_pending_ssid));
        memcpy(g_pending_pass, p->pass, sizeof(g_pending_pass));
        g_pending_ssid[sizeof(g_pending_ssid) - 1] = '\0';
        g_pending_pass[sizeof(g_pending_pass) - 1] = '\0';
        g_wifi_config_pending = true;
        Serial.println(F("[DOCK] WiFi config queued; docking radio remains primary"));
      }
#else
      Serial.println(F("[DOCK] WiFi config received but optional home WiFi is disabled"));
#endif
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
  } else if (p->cmd == DOCK_CMD_REQUEST_CHARGE) {
    dockStateRequestCharge();
    Serial.println(F("[DOCK] CMD: Request charge (from WALL-E)"));
  } else if (p->cmd == DOCK_CMD_DOCKING_ARM) {
    dockAlignmentSetDockingArmed(true);
    Serial.println(F("[DOCK] CMD: Docking ARM - arrow guidance allowed"));
  } else if (p->cmd == DOCK_CMD_DOCKING_DISARM) {
    dockAlignmentSetDockingArmed(false);
    Serial.println(F("[DOCK] CMD: Docking DISARM - arrows off"));
  } else if (p->cmd == DOCK_CMD_APPROACH_STAGE &&
             len >= (int)sizeof(DockApproachStagePacket_t)) {
    const DockApproachStagePacket_t *ap = (const DockApproachStagePacket_t *)data;
    dockAlignmentSetStage(ap->stage);
  }

  return true;
}
