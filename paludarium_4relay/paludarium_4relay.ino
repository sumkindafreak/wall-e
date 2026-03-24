/*
 * Paludarium 4-Relay Controller
 * Standard ESP32 + 4-channel relay board.
 * WiFi (STA + AP fallback), web dashboard, optional DHT11/water sensor.
 * FastLED 30x4 matrix with NTP-based day/night (no RTC, no LDR).
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"
#include "page_html.h"
#if defined(LED_ENABLED) && (LED_ENABLED) == 1
#include "led_fastled.h"
#endif

#if PIN_DHT >= 0
#include <DHT.h>
#define DHT_TYPE DHT11
DHT dht(PIN_DHT, DHT_TYPE);
#endif

WebServer server(80);
Preferences prefs;

// ────────────────────────────── STATE ──────────────────────────────
bool relayState[RELAY_COUNT] = { false };
bool systemAutoMode = true;  // Auto = schedule/logic; Manual = web only
bool wifiConnected = false;
bool wifiAPMode = false;
uint32_t lastSensorRead = 0;
uint32_t lastWiFiCheck = 0;

float sensorTemp = NAN;
float sensorHumid = NAN;
bool sensorWaterHigh = false;

// NTP / time (no RTC)
static bool timeSynced = false;
static uint16_t dayStartMin = 360;   // 06:00
static uint16_t dayEndMin = 1200;    // 20:00
static long gmtOffsetSec = -18000;   // EST = -5h
static uint32_t lastNtpCheck = 0;
#define NTP_CHECK_INTERVAL_MS  60000
#define NTP_SERVER            "pool.ntp.org"

struct Config {
  char wifiSSID[33];
  char wifiPass[65];
};
Config config;

// ────────────────────────────── RELAYS ──────────────────────────────
void setRelay(int idx, bool on) {
  if (idx < 0 || idx >= RELAY_COUNT) return;
  if (relayState[idx] == on) return;
  relayState[idx] = on;
  digitalWrite(RELAY_PINS[idx], on ? RELAY_ON : RELAY_OFF);
  if (DEBUG_SERIAL) Serial.printf("[RELAY] %s -> %s\n", RELAY_LABELS[idx], on ? "ON" : "OFF");
}

void turnOffAllRelays() {
  for (int i = 0; i < RELAY_COUNT; i++) setRelay(i, false);
}

// ────────────────────────────── SENSORS (optional) ──────────────────────────────
void readSensors() {
#if PIN_DHT >= 0
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) sensorTemp = t;
  else if (isnan(sensorTemp)) sensorTemp = 25.0f;
  if (!isnan(h)) sensorHumid = h;
  else if (isnan(sensorHumid)) sensorHumid = 50.0f;
#else
  (void)0;
#endif

#if PIN_WATER >= 0
  sensorWaterHigh = (digitalRead(PIN_WATER) == LOW);
#else
  sensorWaterHigh = false;
#endif

  if (DEBUG_SERIAL) {
    Serial.printf("[SENSOR] T=%.1f H=%.0f%% Water=%s\n",
      sensorTemp, sensorHumid, sensorWaterHigh ? "HIGH" : "OK");
  }
}

// ────────────────────────────── NTP / TIME ──────────────────────────────
static void timeSyncBegin(void) {
  prefs.begin("time", true);
  dayStartMin = (uint16_t)prefs.getUShort("dayStart", 360);
  dayEndMin = (uint16_t)prefs.getUShort("dayEnd", 1200);
  gmtOffsetSec = prefs.getLong("gmtOffset", -18000);
  prefs.end();
  if (wifiConnected) {
    configTime((long)gmtOffsetSec, 0, NTP_SERVER);
    if (DEBUG_SERIAL) Serial.println("[NTP] configTime set");
  }
}

static void timeSyncPoll(void) {
  if (!wifiConnected) return;
  struct tm tm;
  if (getLocalTime(&tm, 100)) {
    if (!timeSynced && DEBUG_SERIAL) Serial.println("[NTP] Time synced");
    timeSynced = true;
  }
  (void)tm;
}

static void getLocalHourMin(uint8_t* hour, uint8_t* minute) {
  struct tm tm;
  if (!getLocalTime(&tm, 50)) {
    *hour = 12; *minute = 0;
    return;
  }
  *hour = (uint8_t)(tm.tm_hour % 24);
  *minute = (uint8_t)(tm.tm_min % 60);
}

// ────────────────────────────── WIFI ──────────────────────────────
void setupWiFi() {
  if (DEBUG_SERIAL) Serial.println("[WIFI] Starting...");

  prefs.begin("wifi", false);
  if (strlen(config.wifiSSID) == 0) {
    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");
    if (s.length() > 0) {
      s.toCharArray(config.wifiSSID, sizeof(config.wifiSSID));
      p.toCharArray(config.wifiPass, sizeof(config.wifiPass));
      if (DEBUG_SERIAL) Serial.printf("[WIFI] Loaded saved: %s\n", config.wifiSSID);
    }
  }
  prefs.end();

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(config.wifiSSID, config.wifiPass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < (WIFI_CONNECT_TIMEOUT_MS / 500)) {
    delay(500);
    if (DEBUG_SERIAL) Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    wifiAPMode = false;
    if (DEBUG_SERIAL) {
      Serial.println(" OK");
      Serial.printf("[WIFI] IP: %s RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    prefs.begin("wifi", false);
    prefs.putString("ssid", config.wifiSSID);
    prefs.putString("pass", config.wifiPass);
    prefs.end();
  } else {
    wifiConnected = false;
    if (DEBUG_SERIAL) Serial.println(" FAILED");
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    wifiAPMode = true;
    if (DEBUG_SERIAL) {
      Serial.printf("[WIFI] AP: %s IP: %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
    }
  }
}

void checkWiFi() {
  static uint32_t lastAPRetry = 0;

  if (wifiAPMode) {
    if (millis() - lastAPRetry > WIFI_RETRY_INTERVAL_MS) {
      lastAPRetry = millis();
      if (DEBUG_SERIAL) Serial.println("[WIFI] Retry STA...");
      setupWiFi();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    if (DEBUG_SERIAL) Serial.println("[WIFI] Lost, reconnecting...");
    WiFi.reconnect();
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) { delay(500); attempts++; }
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      if (DEBUG_SERIAL) Serial.println("[WIFI] Reconnected");
    }
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    if (DEBUG_SERIAL) Serial.println("[WIFI] Restored");
  }
}

// ────────────────────────────── WEB ROUTES ──────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

void handleAPI() {
  DynamicJsonDocument doc(1024);
  doc["tp"] = round(sensorTemp * 10) / 10.0;
  doc["hm"] = round(sensorHumid * 10) / 10.0;
  doc["wh"] = sensorWaterHigh;
  doc["wifi"] = wifiConnected;
  doc["ap"] = wifiAPMode;
  doc["rssi"] = wifiAPMode ? 0 : WiFi.RSSI();
  doc["auto"] = systemAutoMode;
  doc["timeSynced"] = timeSynced;
  struct tm tm;
  if (getLocalTime(&tm, 0)) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", tm.tm_hour, tm.tm_min);
    doc["time"] = buf;
  } else {
    doc["time"] = "--:--";
  }
#if defined(LED_ENABLED) && (LED_ENABLED) == 1
  doc["ledBrightness"] = ledGetBrightness();
  doc["ledMode"] = ledGetMode();
#endif
  JsonArray arr = doc.createNestedArray("relays");
  for (int i = 0; i < RELAY_COUNT; i++) {
    arr.add(relayState[i] ? 1 : 0);
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleRelay() {
  if (!server.hasArg("i") || !server.hasArg("s")) {
    server.send(400, "text/plain", "Bad request");
    return;
  }
  int idx = server.arg("i").toInt();
  int st = server.arg("s").toInt();
  if (idx < 0 || idx >= RELAY_COUNT) {
    server.send(400, "text/plain", "Invalid relay");
    return;
  }
  setRelay(idx, st != 0);
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "Bad JSON");
    return;
  }
  const char* mode = doc["mode"];
  if (!mode) {
    server.send(400, "text/plain", "Missing mode");
    return;
  }
  if (strcmp(mode, "auto") == 0) {
    systemAutoMode = true;
    if (DEBUG_SERIAL) Serial.println("[MODE] Auto");
  } else if (strcmp(mode, "manual") == 0) {
    systemAutoMode = false;
    if (DEBUG_SERIAL) Serial.println("[MODE] Manual");
  } else {
    server.send(400, "text/plain", "Invalid mode");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleWifiScan() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
  WiFi.scanDelete();
}

void handleWifiConnect() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "Bad JSON");
    return;
  }
  const char* ssid = doc["ssid"];
  const char* pass = doc["pass"] ? doc["pass"].as<const char*>() : "";
  if (!ssid || strlen(ssid) == 0) {
    server.send(400, "text/plain", "Missing ssid");
    return;
  }
  strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
  config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  strncpy(config.wifiPass, pass, sizeof(config.wifiPass) - 1);
  config.wifiPass[sizeof(config.wifiPass) - 1] = '\0';

  prefs.begin("wifi", false);
  prefs.putString("ssid", config.wifiSSID);
  prefs.putString("pass", config.wifiPass);
  prefs.end();

  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Restarting\"}");
  delay(500);
  ESP.restart();
}

// ────────────────────────────── SETUP ──────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nPaludarium 4-Relay Controller (ESP32)");
  Serial.println("======================================");

  memset(&config, 0, sizeof(config));
  // WiFi credentials loaded in setupWiFi() from NVS "wifi" namespace

  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], RELAY_OFF);
  }
#if PIN_WATER >= 0
  pinMode(PIN_WATER, INPUT_PULLUP);
#endif
#if PIN_DHT >= 0
  dht.begin();
  if (DEBUG_SERIAL) Serial.println("DHT11 OK");
#endif

  readSensors();
  setupWiFi();
  timeSyncBegin();
#if defined(LED_ENABLED) && (LED_ENABLED) == 1
  ledBegin();
#endif

  server.on("/", handleRoot);
  server.on("/api", HTTP_GET, handleAPI);
  server.on("/r", HTTP_GET, handleRelay);
  server.on("/system/mode", HTTP_POST, handleMode);
  server.on("/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/wifi/connect", HTTP_POST, handleWifiConnect);
#if defined(LED_ENABLED) && (LED_ENABLED) == 1
  server.on("/led/brightness", HTTP_POST, []() {
    if (!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }
    DynamicJsonDocument doc(128);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "Bad JSON"); return; }
    int v = doc["value"] | (int)LED_BRIGHTNESS_DEFAULT;
    v = constrain(v, 0, 255);
    ledSetBrightness((uint8_t)v);
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/led/mode", HTTP_POST, []() {
    if (!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }
    DynamicJsonDocument doc(128);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "Bad JSON"); return; }
    const char* m = doc["mode"];
    if (!m) { server.send(400, "text/plain", "Missing mode"); return; }
    uint8_t mode = LED_AUTO;
    if (strcmp(m, "off") == 0) mode = LED_OFF;
    else if (strcmp(m, "manual") == 0) mode = LED_MANUAL;
    else if (strcmp(m, "auto") != 0) { server.send(400, "text/plain", "Invalid mode"); return; }
    ledSetMode(mode);
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/time/schedule", HTTP_POST, []() {
    if (!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "Bad JSON"); return; }
    if (doc.containsKey("dayStart")) dayStartMin = (uint16_t)doc["dayStart"].as<int>();
    if (doc.containsKey("dayEnd")) dayEndMin = (uint16_t)doc["dayEnd"].as<int>();
    if (doc.containsKey("gmtOffset")) gmtOffsetSec = doc["gmtOffset"].as<long>();
    prefs.begin("time", false);
    prefs.putUShort("dayStart", dayStartMin);
    prefs.putUShort("dayEnd", dayEndMin);
    prefs.putLong("gmtOffset", gmtOffsetSec);
    prefs.end();
    if (wifiConnected) configTime((long)gmtOffsetSec, 0, NTP_SERVER);
    server.send(200, "application/json", "{\"ok\":true}");
  });
#endif
  server.begin();

  if (DEBUG_SERIAL) {
    Serial.println("======================================");
    Serial.printf("Web: http://%s\n", wifiAPMode ? WiFi.softAPIP().toString().c_str() : WiFi.localIP().toString().c_str());
    Serial.println("======================================\n");
  }
}

static uint32_t lastLedUpdate = 0;
#define LED_UPDATE_INTERVAL_MS  50

// ────────────────────────────── LOOP ──────────────────────────────
void loop() {
  server.handleClient();
  uint32_t now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = now;
    readSensors();
  }

  if (now - lastWiFiCheck >= WIFI_CHECK_INTERVAL_MS) {
    lastWiFiCheck = now;
    checkWiFi();
  }

  if (wifiConnected && now - lastNtpCheck >= NTP_CHECK_INTERVAL_MS) {
    lastNtpCheck = now;
    timeSyncPoll();
  }

#if defined(LED_ENABLED) && (LED_ENABLED) == 1
  if (now - lastLedUpdate >= LED_UPDATE_INTERVAL_MS) {
    lastLedUpdate = now;
    uint8_t h, m;
    getLocalHourMin(&h, &m);
    ledUpdate(h, m, timeSynced, dayStartMin, dayEndMin);
  }
#endif
}
