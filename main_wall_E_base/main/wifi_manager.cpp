// ============================================================
// WALL-E WiFi Manager — ESP32-S3 Base Brain
//
// The S3 owns both Wi-Fi and ESP-NOW, so they MUST share one RF channel.
// The CYD/master controller is fixed to channel 11; therefore WALL-E's AP is
// fixed to channel 11 and STA connections are accepted only when the target
// access point is also on channel 11.
// ============================================================

#include "wifi_manager.h"
#include "radio_transport.h"
#include <WiFi.h>
#include <Preferences.h>

#define NVS_NAMESPACE  "walle_wifi"
#define NVS_SSID_KEY   "sta_ssid"
#define NVS_PASS_KEY   "sta_pass"

static WiFiState _state = WS_AP_ONLY;
static unsigned long _connectStartMillis = 0;
static bool _connectPending = false;
static Preferences _prefs;

// ------------------------------------------------------------
// Find a Wi-Fi network and return its channel.
// Returns 0 when the SSID cannot currently be found.
//
// This check prevents WiFi.begin() from retuning the S3 away from the
// ESP-NOW channel used by the CYD/audio/vision/dock network.
// ------------------------------------------------------------
static uint8_t findNetworkChannel(const char* ssid) {
  if (!ssid || ssid[0] == '\0') return 0;

  Serial.printf("[WiFi] Checking channel for '%s'...\n", ssid);
  const int count = WiFi.scanNetworks();
  uint8_t foundChannel = 0;

  for (int i = 0; i < count; ++i) {
    if (WiFi.SSID(i) == ssid) {
      foundChannel = (uint8_t)WiFi.channel(i);
      break;
    }
  }

  WiFi.scanDelete();
  return foundChannel;
}

// ------------------------------------------------------------
// Start a STA connection only if that network is on channel 11.
// ------------------------------------------------------------
static bool beginCompatibleSta(const char* ssid, const char* pass) {
  const uint8_t channel = findNetworkChannel(ssid);

  if (channel == 0) {
    Serial.printf("[WiFi] '%s' not found; staying on WALL-E AP channel %u\n",
                  ssid, (unsigned)WALLE_RADIO_CHANNEL);
    return false;
  }

  if (channel != WALLE_RADIO_CHANNEL) {
    Serial.printf("[WiFi] Refusing STA '%s': channel %u would break ESP-NOW channel %u\n",
                  ssid,
                  (unsigned)channel,
                  (unsigned)WALLE_RADIO_CHANNEL);
    return false;
  }

  Serial.printf("[WiFi] STA '%s' is compatible on channel %u\n",
                ssid, (unsigned)channel);
  WiFi.begin(ssid, pass ? pass : "");
  _connectPending = true;
  _connectStartMillis = millis();
  _state = WS_CONNECTING;
  return true;
}

void wifiManagerInit() {
  WiFi.mode(WIFI_AP_STA);

  // Explicit channel is critical: the CYD/master controller uses ESP-NOW 11.
  const bool apOk = WiFi.softAP(AP_SSID,
                                AP_PASSWORD,
                                WALLE_RADIO_CHANNEL,
                                0,
                                4);
  if (apOk) {
    Serial.printf("[WiFi] AP '%s' ready on channel %u, IP %s\n",
                  AP_SSID,
                  (unsigned)WALLE_RADIO_CHANNEL,
                  WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println(F("[WiFi] ERROR: WALL-E AP failed to start"));
  }

  _state = WS_AP_ONLY;
  _connectPending = false;

  _prefs.begin(NVS_NAMESPACE, true);
  const String savedSsid = _prefs.getString(NVS_SSID_KEY, "");
  const String savedPass = _prefs.getString(NVS_PASS_KEY, "");
  _prefs.end();

  if (savedSsid.length() > 0) {
    if (!beginCompatibleSta(savedSsid.c_str(), savedPass.c_str())) {
      _state = WS_AP_ONLY;
      Serial.println(F("[WiFi] Saved STA skipped; ESP-NOW remains protected"));
    }
  }
}

void wifiManagerHandle() {
  if (_state != WS_CONNECTING) return;

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    _connectPending = false;
    _state = WS_CONNECTED;
    Serial.printf("[WiFi] STA connected: %s, channel %u, IP %s\n",
                  WiFi.SSID().c_str(),
                  (unsigned)WiFi.channel(),
                  WiFi.localIP().toString().c_str());
    return;
  }

  if ((millis() - _connectStartMillis) > WIFI_CONNECT_TIMEOUT_MS) {
    _connectPending = false;

    // Disconnect STA without erasing the AP configuration. Reassert AP mode
    // and channel 11 so ESP-NOW remains deterministic after a failed attempt.
    WiFi.disconnect(false, false);
    delay(20);
    WiFi.softAP(AP_SSID,
                AP_PASSWORD,
                WALLE_RADIO_CHANNEL,
                0,
                4);
    _state = WS_FAILED;
    Serial.println(F("[WiFi] STA timeout; WALL-E AP/ESP-NOW remains on channel 11"));
  }
}

bool wifiConnectSTA(const char* ssid, const char* pass) {
  if (!ssid || ssid[0] == '\0') return false;

  // Check compatibility BEFORE storing or connecting. A network on another
  // channel would retune the S3 and disconnect WALL-E's ESP-NOW nodes.
  const uint8_t channel = findNetworkChannel(ssid);
  if (channel == 0 || channel != WALLE_RADIO_CHANNEL) {
    Serial.printf("[WiFi] STA rejected: '%s' must be on channel %u\n",
                  ssid, (unsigned)WALLE_RADIO_CHANNEL);
    return false;
  }

  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.putString(NVS_SSID_KEY, ssid);
  _prefs.putString(NVS_PASS_KEY, pass ? pass : "");
  _prefs.end();

  return beginCompatibleSta(ssid, pass);
}

void wifiDisconnectSTA() {
  _connectPending = false;
  WiFi.disconnect(false, false);
  delay(20);

  // Reassert the fixed AP/ESP-NOW channel after leaving STA mode.
  WiFi.softAP(AP_SSID,
              AP_PASSWORD,
              WALLE_RADIO_CHANNEL,
              0,
              4);
  _state = WS_AP_ONLY;
  Serial.println(F("[WiFi] STA disconnected; WALL-E AP remains on channel 11"));
}

void wifiClearCredentials() {
  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.remove(NVS_SSID_KEY);
  _prefs.remove(NVS_PASS_KEY);
  _prefs.end();
  wifiDisconnectSTA();
}

WiFiState wifiGetState() {
  return _state;
}

String wifiGetSTA_IP() {
  if (WiFi.status() != WL_CONNECTED) return "";
  return WiFi.localIP().toString();
}

String wifiGetAP_IP() {
  return WiFi.softAPIP().toString();
}

String wifiGetSTA_SSID() {
  if (WiFi.status() != WL_CONNECTED) return "";
  return WiFi.SSID();
}

static void jsonEscape(String& out, const String& value) {
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else out += c;
  }
}

String wifiGetStatusJSON() {
  String out = "{\"state\":";
  out += (int)wifiGetState();
  out += ",\"ap_ssid\":\"";
  jsonEscape(out, String(AP_SSID));
  out += "\",\"ap_ip\":\"";
  out += wifiGetAP_IP();
  out += "\",\"ap_channel\":";
  out += String((int)WALLE_RADIO_CHANNEL);
  out += ",\"ap_clients\":";
  out += String(WiFi.softAPgetStationNum());
  out += ",\"sta_ssid\":\"";
  jsonEscape(out, wifiGetSTA_SSID());
  out += "\",\"sta_ip\":\"";
  out += wifiGetSTA_IP();
  out += ",\"rssi\":";
  if (WiFi.status() == WL_CONNECTED) out += String(WiFi.RSSI());
  else out += "null";
  out += "}";
  return out;
}

String wifiScanJSON() {
  const int count = WiFi.scanNetworks();
  String out = "[";

  for (int i = 0; i < count; ++i) {
    if (i > 0) out += ",";
    out += "{\"ssid\":\"";

    const String ssid = WiFi.SSID(i);
    for (size_t j = 0; j < ssid.length(); ++j) {
      const char c = ssid[j];
      if (c == '\\') out += "\\\\";
      else if (c == '"') out += "\\\"";
      else out += c;
    }

    out += "\",\"rssi\":";
    out += WiFi.RSSI(i);
    out += ",\"channel\":";
    out += WiFi.channel(i);
    out += ",\"espnow_compatible\":";
    out += (WiFi.channel(i) == WALLE_RADIO_CHANNEL) ? "true" : "false";
    out += ",\"secure\":";
    out += (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false";
    out += "}";
  }

  out += "]";
  WiFi.scanDelete();
  return out;
}
