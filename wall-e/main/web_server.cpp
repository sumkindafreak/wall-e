#include "web_server.h"
#include "web_page.h"
#include "motor_control.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "servo_manager.h"
#include "imu_manager.h"
#include "battery_monitor.h"
#include <WebServer.h>
#include <Arduino.h>
#include <Preferences.h>

// ============================================================
//  WALL-E Web Server Implementation
// ============================================================

WebServer server(80);

static Preferences _settingsPrefs;
static uint8_t _maxSpeed = 255;
#define SETTINGS_NAMESPACE  "walle_cfg"
#define SETTINGS_KEY_MAXSP "max_sp"

// Updated every time a valid command arrives — used by failsafe
extern unsigned long lastCommandMillis;

// Helper: add CORS headers so browser fetch() works across IPs
static void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
}

// --- Drive Route Handlers ---

static void handleRoot() {
  server.send_P(200, "text/html", WALLE_PAGE);
}

static void handleForward() {
  lastCommandMillis = millis();
  motorForward(motorGetSpeed());
  displaySetCommand(CMD_FORWARD);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleReverse() {
  lastCommandMillis = millis();
  motorReverse(motorGetSpeed());
  displaySetCommand(CMD_REVERSE);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLeft() {
  lastCommandMillis = millis();
  motorLeft(motorGetSpeed());
  displaySetCommand(CMD_LEFT);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleRight() {
  lastCommandMillis = millis();
  motorRight(motorGetSpeed());
  displaySetCommand(CMD_RIGHT);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleStop() {
  lastCommandMillis = millis();
  motorStop();
  displaySetCommand(CMD_STOP);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleSpeed() {
  if (server.hasArg("value")) {
    int val = server.arg("value").toInt();
    val = constrain(val, 0, (int)_maxSpeed);
    motorSetSpeed((uint8_t)val);
    displaySetSpeed((uint8_t)val);
    addCORS();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

// Tank drive: left and right in -255..255. Smoother diagonals and curves.
static void handleDrive() {
  if (!server.hasArg("left") || !server.hasArg("right")) {
    server.send(400, "text/plain", "Missing left or right");
    return;
  }
  lastCommandMillis = millis();
  int left  = server.arg("left").toInt();
  int right = server.arg("right").toInt();
  left  = constrain(left,  -255, 255);
  right = constrain(right, -255, 255);
  motorSetLeftRight((int16_t)left, (int16_t)right);
  float jx = (right - left) / 255.0f;
  float jy = -(left + right) / 255.0f;
  jx = constrain(jx, -1.0f, 1.0f);
  jy = constrain(jy, -1.0f, 1.0f);
  displaySetStick(jx, jy);
  uint8_t spd = (uint8_t)((abs(left) + abs(right)) / 2);
  displaySetSpeed(spd);
  addCORS();
  server.send(200, "text/plain", "OK");
}

// --- Settings (max speed etc.) ---
static void handleSettingsGet() {
  String s = "{\"max_speed\":";
  s += (int)_maxSpeed;
  s += "}";
  addCORS();
  server.send(200, "application/json", s);
}

static void handleSettingsSet() {
  if (server.hasArg("max_speed")) {
    int v = server.arg("max_speed").toInt();
    _maxSpeed = (uint8_t)constrain(v, 1, 255);
    _settingsPrefs.begin(SETTINGS_NAMESPACE, false);
    _settingsPrefs.putUChar(SETTINGS_KEY_MAXSP, _maxSpeed);
    _settingsPrefs.end();
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

// --- WiFi Route Handlers ---

static void handleWifiStatus() {
  addCORS();
  server.send(200, "application/json", wifiGetStatusJSON());
}

static void handleWifiScan() {
  // Scan can take 2-4 seconds — browser will wait
  addCORS();
  server.send(200, "application/json", wifiScanJSON());
}

static void handleWifiConnect() {
  if (!server.hasArg("ssid") || !server.hasArg("password")) {
    server.send(400, "application/json", "{\"error\":\"Missing ssid or password\"}");
    return;
  }
  String ssid = server.arg("ssid");
  String pass = server.arg("password");

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID cannot be empty\"}");
    return;
  }

  wifiConnectSTA(ssid.c_str(), pass.c_str());
  displayUpdateWifi();
  addCORS();
  server.send(200, "application/json", "{\"status\":\"connecting\",\"ssid\":\"" + ssid + "\"}");
}

static void handleWifiDisconnect() {
  wifiDisconnectSTA();
  displayUpdateWifi();
  addCORS();
  server.send(200, "application/json", "{\"status\":\"disconnected\"}");
}

static void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// --- Servo Handlers ---

static void handleServoSet() {
  if (!server.hasArg("ch") || !server.hasArg("pos")) {
    server.send(400, "application/json", "{\"error\":\"Missing ch or pos\"}");
    return;
  }
  int ch    = server.arg("ch").toInt();
  int pos   = server.arg("pos").toInt();
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : SERVO_DEFAULT_SPEED;
  ch    = constrain(ch, 0, SERVO_COUNT - 1);
  pos   = constrain(pos, 0, 100);
  speed = constrain(speed, 1, 100);
  servoSet(ch, pos, speed);
  addCORS();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleServoNeutral() {
  servoNeutral(SERVO_SLOW_SPEED);
  addCORS();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleServoStatus() {
  addCORS();
  server.send(200, "application/json", servoGetStatusJSON());
}

// --- IMU Handlers ---

static void handleImuStatus() {
  addCORS();
  server.send(200, "application/json", imuGetStatusJSON());
}

static void handleImuRecalibrate() {
  forceRecalibration();
  addCORS();
  server.send(200, "application/json", "{\"status\":\"calibrating\"}");
}

// --- Battery Handler ---

static void handleBatteryStatus() {
  addCORS();
  server.send(200, "application/json", batteryGetStatusJSON());
}

// --- Public Functions ---

void webServerInit() {
  _settingsPrefs.begin(SETTINGS_NAMESPACE, true);
  _maxSpeed = _settingsPrefs.getUChar(SETTINGS_KEY_MAXSP, 255);
  _settingsPrefs.end();
  if (_maxSpeed < 1) _maxSpeed = 255;

  // Drive endpoints
  server.on("/",                  HTTP_GET, handleRoot);
  server.on("/forward",           HTTP_GET, handleForward);
  server.on("/reverse",           HTTP_GET, handleReverse);
  server.on("/left",              HTTP_GET, handleLeft);
  server.on("/right",             HTTP_GET, handleRight);
  server.on("/stop",              HTTP_GET, handleStop);
  server.on("/speed",             HTTP_GET, handleSpeed);
  server.on("/drive",             HTTP_GET, handleDrive);

  server.on("/settings",          HTTP_GET, handleSettingsGet);
  server.on("/settings/set",      HTTP_GET, handleSettingsSet);

  // WiFi management endpoints
  server.on("/wifi/status",       HTTP_GET, handleWifiStatus);
  server.on("/wifi/scan",         HTTP_GET, handleWifiScan);
  server.on("/wifi/connect",      HTTP_GET, handleWifiConnect);
  server.on("/wifi/disconnect",   HTTP_GET, handleWifiDisconnect);

  // Servo endpoints
  server.on("/servo/set",         HTTP_GET, handleServoSet);
  server.on("/servo/neutral",     HTTP_GET, handleServoNeutral);
  server.on("/servo/status",      HTTP_GET, handleServoStatus);

  // Sensor endpoints
  server.on("/imu/status",        HTTP_GET, handleImuStatus);
  server.on("/imu/recalibrate",  HTTP_GET, handleImuRecalibrate);
  server.on("/battery/status",    HTTP_GET, handleBatteryStatus);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[WebServer] Started on port 80");
}

void webServerHandle() {
  server.handleClient();
}
