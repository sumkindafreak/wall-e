// ============================================================
//  WALL-E WiFi Manager Implementation
//  AP always on; STA async connect with NVS credentials
// ============================================================

#include "wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>

// NVS namespace for STA credentials
#define NVS_NAMESPACE  "walle_wifi"
#define NVS_SSID_KEY   "sta_ssid"
#define NVS_PASS_KEY   "sta_pass"

static WiFiState _state = WS_AP_ONLY;
static unsigned long _connectStartMillis = 0;
static bool _connectPending = false;
static Preferences _prefs;

void wifiManagerInit() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  _state = WS_AP_ONLY;

  _prefs.begin(NVS_NAMESPACE, true);
  String savedSsid = _prefs.getString(NVS_SSID_KEY, "");
  String savedPass = _prefs.getString(NVS_PASS_KEY, "");
  _prefs.end();

  if (savedSsid.length() > 0) {
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    _connectPending = true;
    _connectStartMillis = millis();
    _state = WS_CONNECTING;
  }
}

void wifiManagerHandle() {
  // Always sync state when we're "connecting" so main loop can detect transition and refresh display
  if (_state == WS_CONNECTING) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      _connectPending = false;
      _state = WS_CONNECTED;
      return;
    }
    if ((millis() - _connectStartMillis) > WIFI_CONNECT_TIMEOUT_MS) {
      _connectPending = false;
      WiFi.disconnect(true);
      _state = WS_FAILED;
    }
  }
}

bool wifiConnectSTA(const char* ssid, const char* pass) {
  if (ssid == nullptr || strlen(ssid) == 0) return false;

  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.putString(NVS_SSID_KEY, ssid);
  _prefs.putString(NVS_PASS_KEY, pass == nullptr ? "" : pass);
  _prefs.end();

  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, pass);
  _connectPending = true;
  _connectStartMillis = millis();
  _state = WS_CONNECTING;
  return true;
}

void wifiDisconnectSTA() {
  _connectPending = false;
  WiFi.disconnect(true);
  _state = WS_AP_ONLY;
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

String wifiGetStatusJSON() {
  WiFiState s = wifiGetState();
  String out = "{\"state\":";
  out += (int)s;
  out += ",\"ap_ip\":\"";
  out += wifiGetAP_IP();
  out += "\",\"sta_ip\":\"";
  out += wifiGetSTA_IP();
  out += "\",\"ssid\":\"";
  // Escape SSID for JSON (backslash and quote)
  String ssid = wifiGetSTA_SSID();
  for (size_t i = 0; i < ssid.length(); i++) {
    char c = ssid[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else out += c;
  }
  out += "\"}";
  return out;
}

String wifiScanJSON() {
  int n = WiFi.scanNetworks();
  String out = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) out += ",";
    out += "{\"ssid\":\"";
    String s = WiFi.SSID(i);
    for (size_t j = 0; j < s.length(); j++) {
      char c = s[j];
      if (c == '\\') out += "\\\\";
      else if (c == '"') out += "\\\"";
      else out += c;
    }
    out += "\",\"rssi\":";
    out += WiFi.RSSI(i);
    out += ",\"secure\":";
    out += (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "true" : "false";
    out += "}";
  }
  out += "]";
  WiFi.scanDelete();
  return out;
}
