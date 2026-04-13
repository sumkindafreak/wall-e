#include "web_server.h"
#include "web_page_lros.h"
#include "motor_control.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "servo_manager.h"
#include "laser_control.h"
#include "imu_manager.h"
#include "battery_monitor.h"
#include "autonomy_engine.h"
#include "unified_autonomy_engine.h"
#include "emotion_engine.h"
#include "walle_emotion_pose.h"
#include "interest_engine.h"
#include "personality_engine.h"
#include "memory_engine.h"
#include "return_home_engine.h"
#include "compass_sensor.h"
#include "vision_behaviour.h"
#include "audio_espnow.h"
#include "audio_telem.h"
#include "audio_protocol.h"
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "sonar_sensor.h"
#include "gps_module.h"
#include "navigation_api.h"
#include "sequence_api.h"
#include "motion_layer.h"
#include "motion_authority.h"
#include "api_security.h"
#include "eve_uart_bridge.h"
#include "telemetry_manager.h"
#include "websocket_manager.h"
#include "espnow_receiver.h"
#include "dock_homing.h"
#include <WebServer.h>
#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>

// ============================================================
//  WALL-E Web Server Implementation
// ============================================================

WebServer server(80);

static Preferences _settingsPrefs;
static uint8_t _maxSpeed = 255;
static int16_t _webDriveLeft = 0;
static int16_t _webDriveRight = 0;
static bool _webManualSticky = false;
#define SETTINGS_NAMESPACE  "walle_cfg"
#define SETTINGS_KEY_MAXSP "max_sp"

// Updated every time a valid command arrives — used by failsafe
extern unsigned long lastCommandMillis;
#ifndef FAILSAFE_TIMEOUT_MS
#define FAILSAFE_TIMEOUT_MS 500UL
#endif

static void addCORS(void);

static bool webMotionAllowedOr403(void) {
  if (motionAuthorityAllowWeb()) return true;
  addCORS();
  server.send(403, "application/json", "{\"ok\":false,\"error\":\"motion_policy_denies_web\"}");
  return false;
}

static const char* mapMotionUiFromLayer(MotionLayerMode m) {
  switch (m) {
    case MOTION_MODE_EMERGENCY: return "E_STOP";
    case MOTION_MODE_IDLE_STANDBY: return "IDLE";
    case MOTION_MODE_MANUAL: return "MANUAL";
    case MOTION_MODE_AUTONOMY_ROAM: return "AUTONOMOUS";
    case MOTION_MODE_OBSTACLE_AVOID: return "AUTONOMOUS";
    case MOTION_MODE_EXPRESSIVE_REACT: return "EXPRESSIVE";
    case MOTION_MODE_DOCK_SEARCH: return "HOMING";
    case MOTION_MODE_DOCK_ALIGN: return "DOCK_ALIGN";
    case MOTION_MODE_DOCK_APPROACH: return "DOCK_APPROACH";
    case MOTION_MODE_DOCK_CHARGING: return "IDLE";
    case MOTION_MODE_DOCK_LEGACY_HOMING: return "HOMING";
    default: return "UNKNOWN";
  }
}

/** LROS WebUI: global authority + motion + drive profile (HTTP link is always LIVE from server POV). */
static void handleMotionOperatorApi() {
  const MotionLayerSnapshot* snap = motionLayerGetSnapshot();
  const char* motionUi = mapMotionUiFromLayer(snap->mode);
  if (unifiedAutonomySafetyActive()) {
    motionUi = "E_STOP";
  } else if (unifiedAutonomyGetState() == UA_ERROR) {
    motionUi = "FAULT";
  }

  bool dockBusy = false;
#if USE_AUTONOMOUS_DOCKING
  dockBusy = autonomousDockingIsActive();
#else
  dockBusy = dockHomingIsActive();
#endif

  const char* authority = "UNKNOWN";
  if (unifiedAutonomySafetyActive()) {
    authority = "SAFETY";
  } else if (motionAuthorityGet() == MOTION_AUTH_WEB_ONLY && espnowIsManualControlActive()) {
    authority = "POLICY";
  } else if (espnowIsManualControlActive() && motionAuthorityAllowCyd()) {
    authority = "CYD";
  } else if (webServerIsManualOverrideActive() && motionAuthorityAllowWeb()) {
    authority = "WEBUI";
  } else if (dockBusy) {
    authority = "DOCKING";
  } else if (autonomyIsEnabled()) {
    authority = "AI";
  }

  unsigned long now = millis();
  uint32_t age = (now >= lastCommandMillis) ? (uint32_t)(now - lastCommandMillis) : 0;
  bool stale = (age > FAILSAFE_TIMEOUT_MS);

  String lock = "";
  bool driveLocked = false;
  if (unifiedAutonomySafetyActive()) {
    lock = "Safety stop latched — drive disabled";
    driveLocked = true;
  } else if (!motionAuthorityAllowWeb() && webServerIsManualOverrideActive()) {
    lock = "Motion policy: CYD only — browser drive disabled";
    driveLocked = true;
  } else if (!motionAuthorityAllowCyd() && espnowIsManualControlActive()) {
    lock = "Motion policy: browser only — CYD drive ignored";
    driveLocked = true;
  } else if (espnowIsManualControlActive() && motionAuthorityAllowCyd()) {
    lock = "CYD touchscreen has control";
    driveLocked = true;
  } else if (dockBusy) {
    lock = "Docking controller owns drive";
    driveLocked = true;
  } else if (autonomyIsEnabled() && !webServerIsManualOverrideActive() && !espnowIsManualControlActive()) {
    lock = "AI assist active — manual override available";
    driveLocked = true;
  } else if (stale) {
    lock = "Connection stale — commands may be ignored";
  }

  const char* prof = motorGetActiveDriveProfileName();

  String json = "{";
  json += "\"authority\":\""; json += authority; json += "\"";
  json += ",\"motion\":\""; json += motionUi; json += "\"";
  json += ",\"motion_raw\":\""; json += motionLayerGetModeNameCurrent(); json += "\"";
  json += ",\"drive_profile\":\""; json += prof; json += "\"";
  json += ",\"link\":\"HTTP\"";
  json += ",\"motion_policy\":\""; json += motionAuthorityModeName(motionAuthorityGet()); json += "\"";
  json += ",\"motion_policy_allows_cyd\":"; json += motionAuthorityAllowCyd() ? "true" : "false";
  json += ",\"motion_policy_allows_web\":"; json += motionAuthorityAllowWeb() ? "true" : "false";
  bool denyCyd = (motionAuthorityGet() == MOTION_AUTH_WEB_ONLY && espnowIsManualControlActive());
  json += ",\"policy_deny_cyd\":"; json += denyCyd ? "true" : "false";
  json += ",\"last_command_age_ms\":"; json += String(age);
  json += ",\"failsafe_timeout_ms\":"; json += String((uint32_t)FAILSAFE_TIMEOUT_MS);
  json += ",\"command_stale\":"; json += stale ? "true" : "false";
  json += ",\"drive_locked\":"; json += driveLocked ? "true" : "false";
  json += ",\"lock_reason\":\""; json += lock; json += "\"";
  json += ",\"unifiedState\":\""; json += unifiedAutonomyGetStateName(); json += "\"";
  json += ",\"unifiedSafety\":"; json += unifiedAutonomySafetyActive() ? "true" : "false";
  json += "}";

  addCORS();
  server.send(200, "application/json", json);
}

// Helper: add CORS headers so browser fetch() works across IPs
static void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
}

// --- Drive Route Handlers ---

static void handleRoot() {
  server.send_P(200, "text/html", WALLE_PAGE_LROS);
}

static void handleForward() {
  if (!webMotionAllowedOr403()) return;
  lastCommandMillis = millis();
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "WebUI forward");
  motorForward(motorGetSpeed());
  displaySetCommand(CMD_FORWARD);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleReverse() {
  if (!webMotionAllowedOr403()) return;
  lastCommandMillis = millis();
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "WebUI reverse");
  motorReverse(motorGetSpeed());
  displaySetCommand(CMD_REVERSE);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLeft() {
  if (!webMotionAllowedOr403()) return;
  lastCommandMillis = millis();
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "WebUI left");
  motorLeft(motorGetSpeed());
  displaySetCommand(CMD_LEFT);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleRight() {
  if (!webMotionAllowedOr403()) return;
  lastCommandMillis = millis();
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "WebUI right");
  motorRight(motorGetSpeed());
  displaySetCommand(CMD_RIGHT);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleStop() {
  lastCommandMillis = millis();
  _webDriveLeft = _webDriveRight = 0;
  _webManualSticky = false;
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
// When both zero, call motorStop() so robot always stops when joystick/sliders return to centre.
static void handleDrive() {
  if (!webMotionAllowedOr403()) return;
  if (!server.hasArg("left") || !server.hasArg("right")) {
    server.send(400, "text/plain", "Missing left or right");
    return;
  }
  lastCommandMillis = millis();
  int left  = server.arg("left").toInt();
  int right = server.arg("right").toInt();
  left  = constrain(left,  -255, 255);
  right = constrain(right, -255, 255);
  _webDriveLeft = (int16_t)left;
  _webDriveRight = (int16_t)right;
  if (left == 0 && right == 0) {
    motorStop();
    displaySetCommand(CMD_STOP);
  } else {
    motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "WebUI tank drive");
    motorSetLeftRight((int16_t)left, (int16_t)right);
  }
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

static void handleWifiClear() {
  wifiClearCredentials();
  displayUpdateWifi();
  addCORS();
  server.send(200, "application/json", "{\"status\":\"credentials_cleared\"}");
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

// --- Autonomy / Behavioural Brain API ---

static void handleAutonomyStatus() {
  const AutoContext* ctx = autonomyGetContext();
  const LocationState* loc = autonomyGetLocation();
  const Personality* p = personalityGet();

  String json = "{\"enabled\":";
  json += autonomyIsEnabled() ? "true" : "false";
  json += ",\"state\":\""; json += autonomyGetStateName();
  json += "\",\"sonar\":"; json += String(ctx->detectedDistance, 1);
  json += ",\"heading\":"; json += String(loc->heading, 1);
  json += ",\"targetHeading\":"; json += String(ctx->targetHeading, 1);
  json += ",\"interest\":"; json += String(interestGetLevel(), 1);
  json += ",\"heightLevel\":"; json += String(ctx->investigationLevel);
  json += ",\"emotion\":\""; json += emotionGetName();
  json += "\",\"poseEmotion\":\""; json += walleEmotionPoseGetName();
  json += "\",\"personality\":{\"curiosity\":"; json += String(p->curiosity, 2);
  json += ",\"bravery\":"; json += String(p->bravery, 2);
  json += ",\"energy\":"; json += String(p->energy, 2);
  json += ",\"randomness\":"; json += String(p->randomness, 2);
  json += "},\"lat\":"; json += String(loc->latitude, 6);
  json += ",\"lon\":"; json += String(loc->longitude, 6);
  json += ",\"gpsValid\":"; json += loc->gpsValid ? "true" : "false";
  json += ",\"objectDetected\":"; json += ctx->objectDetected ? "true" : "false";
  json += ",\"rthActive\":"; json += returnHomeIsActive() ? "true" : "false";
  json += ",\"rthState\":\""; json += returnHomeGetStateName();
  json += "\",\"rthDistance\":"; json += String(returnHomeGetDistance(), 1);
  json += ",\"manualOverride\":"; json += autonomyIsManualOverride() ? "true" : "false";
  json += ",\"unifiedState\":\""; json += unifiedAutonomyGetStateName();
  json += "\",\"unifiedSafety\":"; json += unifiedAutonomySafetyActive() ? "true" : "false";
  json += "}";

  addCORS();
  server.send(200, "application/json", json);
}

static void handleAutonomyEnable() {
  if (server.hasArg("enable")) {
    bool en = (server.arg("enable") == "1" || server.arg("enable") == "true");
    autonomySetEnabled(en);
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

/** Sticky manual override until /drive, /stop, or active=0 (AI Assist "take over"). */
static void handleAutonomyManual() {
  if (server.hasArg("active")) {
    _webManualSticky = (server.arg("active") == "1" || server.arg("active") == "true");
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleMemorySetHome() {
  if (gpsHasFix()) {
    memorySetHome(gpsGetLatitude(), gpsGetLongitude());
    memorySave();
  }
  addCORS();
  server.send(200, "application/json", "{\"ok\":true}");
}

// --- Laser (GPIO on base, aim via PCA9685 pan/tilt) ---
static void handleLaserOn() {
  laserOn();
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserOff() {
  laserOff();
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserBrightness() {
  if (server.hasArg("value")) {
    int v = server.arg("value").toInt();
    v = constrain(v, 0, 255);
    laserSetBrightness((uint8_t)v);
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserSet() {
  int pan = server.hasArg("pan") ? server.arg("pan").toInt() : 50;
  int tilt = server.hasArg("tilt") ? server.arg("tilt").toInt() : 50;
  laserAim(pan, tilt);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserFire() {
  int pan = server.hasArg("pan") ? server.arg("pan").toInt() : 50;
  int tilt = server.hasArg("tilt") ? server.arg("tilt").toInt() : 50;
  uint32_t ms = server.hasArg("time") ? (uint32_t)server.arg("time").toInt() : 1000;
  if (ms > 10000) ms = 10000;
  laserFire(pan, tilt, ms);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserStatus() {
  addCORS();
  server.send(200, "application/json", laserGetStatusJSON());
}

static void handleLaserScan() {
  bool on = false;
  if (server.hasArg("enable")) {
    on = (server.arg("enable") == "1" || server.arg("enable") == "true");
  }
  laserScanSetEnabled(on);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserMood() {
  if (server.hasArg("mood")) {
    laserSetMoodMode((int8_t)server.arg("mood").toInt());
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleLaserSmooth() {
  int pan = server.hasArg("pan") ? server.arg("pan").toInt() : 50;
  int tilt = server.hasArg("tilt") ? server.arg("tilt").toInt() : 50;
  uint16_t per = server.hasArg("delay") ? (uint16_t)server.arg("delay").toInt() : 30;
  laserSmoothSetTarget(pan, tilt, per);
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleVisionStatus() {
  String json = visionGetStatusJSON();
  addCORS();
  server.send(200, "application/json", json);
}

static void handleVisionSnapshotUrl() {
  String json = visionGetStatusJSON();
  int ipIdx = json.indexOf("\"visionNodeIp\":\"");
  if (ipIdx < 0) {
    addCORS();
    server.send(200, "text/plain", "");
    return;
  }
  ipIdx += 15;
  int end = json.indexOf('"', ipIdx);
  if (end < 0) {
    addCORS();
    server.send(200, "text/plain", "");
    return;
  }
  String ip = json.substring(ipIdx, end);
  String url = "http://" + ip + "/snapshot";
  addCORS();
  server.send(200, "text/plain", url);
}

static void handleAudioPlay() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "Missing id");
    return;
  }
  int id = server.arg("id").toInt();
  if (id < 1 || id > 255) {
    server.send(400, "text/plain", "id 1-255");
    return;
  }
  bool ok = audioEspNowPlayTrack((uint8_t)id, WALLE_AUDIO_PRIORITY_WEB);
  addCORS();
  server.send(ok ? 200 : 503, "text/plain", ok ? "OK" : "FAIL");
}

static void handleAudioVolume() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing value");
    return;
  }
  int v = server.arg("value").toInt();
  v = constrain(v, 0, 255);
  uint8_t df = (uint8_t)((v * 30 + 127) / 255);
  bool ok = audioEspNowSetVolume(df);
  addCORS();
  server.send(ok ? 200 : 503, "text/plain", ok ? "OK" : "FAIL");
}

static void handleSystemHealth() {
  addCORS();
  server.send(200, "application/json", nodeHealthGetJSON());
}

static void handleNodesApi() {
  addCORS();
  server.send(200, "application/json", nodeHealthGetJSON());
}

static void handleEmotionApi() {
  WalleEmotionPose p = walleEmotionPoseGetPose(walleEmotionPoseGetState());
  String json = "{";
  json += "\"behavior\":\""; json += emotionGetName(); json += "\"";
  json += ",\"pose\":\""; json += walleEmotionPoseGetName(); json += "\"";
  json += ",\"pose_id\":"; json += (int)walleEmotionPoseGetState();
  json += ",\"pose_axes\":{\"eyeTilt\":"; json += (int)p.eyeTilt;
  json += ",\"neckRotate\":"; json += (int)p.neckRotate;
  json += ",\"neckLift\":"; json += (int)p.neckLift;
  json += ",\"neckHeight\":"; json += (int)p.neckHeight;
  json += "}}";
  addCORS();
  server.send(200, "application/json", json);
}

static void handleEmotionSetApi() {
  if (server.hasArg("clear") && server.arg("clear") == "1") {
    walleEmotionPoseSetManualOverride(-1);
  } else if (server.hasArg("id")) {
    int v = server.arg("id").toInt();
    if (v >= 0 && v <= (int)WALLE_EMOTION_TIRED) {
      walleEmotionPoseSetManualOverride((int8_t)v);
    }
  }
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleAudioTestApi() {
  bool ok = audioEspNowPlayTrack(1, WALLE_AUDIO_PRIORITY_WEB);
  addCORS();
  server.send(ok ? 200 : 503, "text/plain", ok ? "OK" : "FAIL");
}

static void handleDockStartApi() {
#if USE_AUTONOMOUS_DOCKING
  autonomousDockingSetRequested(true);
#endif
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleDockStatusApi() {
  String j = "{";
  j += "\"fsm\":\""; j += autonomousDockingGetStateName(); j += "\"";
#if USE_AUTONOMOUS_DOCKING
  j += ",\"active\":"; j += autonomousDockingIsActive() ? "true" : "false";
#else
  j += ",\"active\":false";
#endif
  j += ",\"dock_node_online\":"; j += nodeHealthIsOnline(WALLE_NODE_DOCK) ? "true" : "false";
  j += ",\"dock_flags\":"; j += (uint32_t)nodeHealthGetFlags(WALLE_NODE_DOCK);
  j += "}";
  addCORS();
  server.send(200, "application/json", j);
}

static void handleAudioMic() {
  uint16_t el = 0, er = 0;
  uint8_t vad = 0;
  uint32_t age = 0;
  bool fresh = audioTelemGet(&el, &er, &vad, &age);
  char buf[160];
  snprintf(buf, sizeof(buf),
           "{\"ok\":%s,\"ear_l\":%u,\"ear_r\":%u,\"voice\":%u,\"age_ms\":%lu}",
           fresh ? "true" : "false", (unsigned)el, (unsigned)er, (unsigned)vad,
           (unsigned long)age);
  addCORS();
  server.send(200, "application/json", buf);
}

/** Cancel autonomous docking + return-home (matches webui /api/dock/cancel) */
static void handleDockCancelApi() {
#if USE_AUTONOMOUS_DOCKING
  autonomousDockingSetRequested(false);
#endif
  returnHomeCancel();
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleSleepApi() {
  addCORS();
  server.send(200, "application/json", "{\"ok\":true,\"note\":\"not_implemented\"}");
}

static void handleFilesListApi() {
  addCORS();
  server.send(200, "application/json", "{\"files\":[]}");
}

static void handleVoiceCommandApi() {
  addCORS();
  server.send(200, "application/json", "{\"ok\":true,\"note\":\"not_implemented\"}");
}

static void handleAiChatApi() {
  addCORS();
  server.send(200, "text/plain", "OK");
}

static void handleMotionAuthorityGet(void) {
  addCORS();
  String j = "{\"ok\":true,\"mode\":\"";
  j += motionAuthorityModeName(motionAuthorityGet());
  j += "\"}";
  server.send(200, "application/json", j);
}

static void handleMotionAuthoritySet(void) {
  if (apiSecurityRejectIfBadToken()) return;
  if (!server.hasArg("mode")) {
    addCORS();
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing mode\"}");
    return;
  }
  String m = server.arg("mode");
  MotionAuthorityMode mm = MOTION_AUTH_ANY;
  if (m == "cyd_only") mm = MOTION_AUTH_CYD_ONLY;
  else if (m == "web_only") mm = MOTION_AUTH_WEB_ONLY;
  else if (m != "any") {
    addCORS();
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad mode\"}");
    return;
  }
  motionAuthoritySet(mm);
  apiSecurityAudit("[motion] policy set");
  addCORS();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleEveStatusApi(void) {
  addCORS();
  server.send(200, "application/json", eveUartBridgeGetJSON());
}

static void handleLivingTelemetryApi(void) {
  addCORS();
  server.send(200, "application/json", telemetryManagerGetJSON());
}

static void handleVisionEventsApi(void) {
  addCORS();
  String j = "{\"ok\":true,\"events\":";
  j += visionGetEventsJSON();
  j += "}";
  server.send(200, "application/json", j);
}

static void handleDashboardApi(void) {
  addCORS();
  server.send(200, "application/json", apiSecurityGetDashboardJSON());
}

static void handleAuditApi(void) {
  if (apiSecurityRejectIfBadToken()) return;
  addCORS();
  server.send(200, "application/json", apiSecurityGetAuditJSON());
}

static void handleApiTokenPost(void) {
  if (!server.hasArg("plain")) {
    addCORS();
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"expected JSON body\"}");
    return;
  }
  String body = server.arg("plain");
  char err[48];
  if (!apiSecurityApplyTokenBody(body.c_str(), body.length(), err, sizeof(err))) {
    addCORS();
    String j = "{\"ok\":false,\"error\":\"";
    j += err;
    j += "\"}";
    server.send(400, "application/json", j);
    return;
  }
  addCORS();
  server.send(200, "application/json", "{\"ok\":true}");
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
  server.on("/wifi/clear",        HTTP_GET, handleWifiClear);

  // Servo endpoints
  server.on("/servo/set",         HTTP_GET, handleServoSet);
  server.on("/servo/neutral",     HTTP_GET, handleServoNeutral);
  server.on("/servo/status",      HTTP_GET, handleServoStatus);

  // Sensor endpoints
  server.on("/imu/status",        HTTP_GET, handleImuStatus);
  server.on("/api/vision/status", HTTP_GET, handleVisionStatus);
  server.on("/api/vision/snapshot_url", HTTP_GET, handleVisionSnapshotUrl);
  server.on("/imu/recalibrate",  HTTP_GET, handleImuRecalibrate);
  server.on("/battery/status",    HTTP_GET, handleBatteryStatus);
  server.on("/api/system/health", HTTP_GET, handleSystemHealth);
  server.on("/api/nodes", HTTP_GET, handleNodesApi);
  server.on("/api/emotion", HTTP_GET, handleEmotionApi);
  server.on("/api/emotion/set", HTTP_GET, handleEmotionSetApi);
  server.on("/api/audio/test", HTTP_GET, handleAudioTestApi);
  server.on("/api/dock/start", HTTP_GET, handleDockStartApi);
  server.on("/api/dock/status", HTTP_GET, handleDockStatusApi);
  server.on("/api/dock/cancel", HTTP_GET, handleDockCancelApi);

  server.on("/api/sleep", HTTP_GET, handleSleepApi);
  server.on("/api/files/list", HTTP_GET, handleFilesListApi);
  server.on("/api/voice/command", HTTP_GET, handleVoiceCommandApi);
  server.on("/api/ai/chat", HTTP_GET, handleAiChatApi);

  server.on("/api/autonomy",      HTTP_GET, handleAutonomyStatus);
  server.on("/api/autonomy/enable", HTTP_GET, handleAutonomyEnable);
  server.on("/api/autonomy/manual", HTTP_GET, handleAutonomyManual);
  server.on("/api/autonomy/set_home", HTTP_GET, handleMemorySetHome);
  server.on("/api/motion/operator", HTTP_GET, handleMotionOperatorApi);
  server.on("/api/motion/authority", HTTP_GET, handleMotionAuthorityGet);
  server.on("/api/motion/authority/set", HTTP_GET, handleMotionAuthoritySet);

  server.on("/api/eve/status", HTTP_GET, handleEveStatusApi);
  server.on("/api/living/telemetry", HTTP_GET, handleLivingTelemetryApi);
  server.on("/api/vision/events", HTTP_GET, handleVisionEventsApi);
  server.on("/api/dashboard", HTTP_GET, handleDashboardApi);
  server.on("/api/audit", HTTP_GET, handleAuditApi);
  server.on("/api/security/token", HTTP_POST, handleApiTokenPost);

  server.on("/api/navigation/route", HTTP_POST, navigationHandleRoutePost);
  server.on("/api/navigation/status", HTTP_GET, navigationHandleStatusGet);
  server.on("/api/navigation/stop", HTTP_GET, navigationHandleStopGet);
  sequenceRegisterWebRoutes();

  server.on("/api/laser/on",        HTTP_GET, handleLaserOn);
  server.on("/api/laser/off",       HTTP_GET, handleLaserOff);
  server.on("/api/laser/brightness", HTTP_GET, handleLaserBrightness);
  server.on("/api/laser/set",       HTTP_GET, handleLaserSet);
  server.on("/api/laser/fire",      HTTP_GET, handleLaserFire);
  server.on("/api/laser/status",    HTTP_GET, handleLaserStatus);
  server.on("/api/laser/scan",      HTTP_GET, handleLaserScan);
  server.on("/api/laser/mood",      HTTP_GET, handleLaserMood);
  server.on("/api/laser/smooth",    HTTP_GET, handleLaserSmooth);

  server.on("/api/audio/play",    HTTP_GET, handleAudioPlay);
  server.on("/api/audio/volume",  HTTP_GET, handleAudioVolume);
  server.on("/api/audio/mic",     HTTP_GET, handleAudioMic);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[WebServer] Started on port 80");
  websocketManagerBegin();
}

void webServerHandle() {
  server.handleClient();
  websocketManagerLoop();
}

bool webServerIsManualOverrideActive() {
  return (_webDriveLeft != 0 || _webDriveRight != 0) || _webManualSticky;
}
