// ============================================================
//  WALL-E ESP-NOW Receiver Implementation
//  Receives ControlPacket from CYD controller → motorSetLeftRight
//  Updated for Enhanced CYD Controller protocol
// ============================================================

#include "espnow_receiver.h"
#include "motor_control.h"
#include "display_manager.h"
#include "battery_monitor.h"
#include "autonomy_engine.h"  // NEW
#include "waypoint_nav.h"     // NEW
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Must match wall_e_master_controller/protocol.h ControlPacket
typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;       // -100 to +100
  int8_t   rightSpeed;      // -100 to +100
  uint8_t  driveMode;       // 0=manual, 1=precision
  uint8_t  behaviourMode;   // mood override (0-4)
  uint8_t  action;          // trigger event
  uint16_t systemFlags;     // bitmask (E-STOP, AUTO, etc.)
} ControlPacket;

// Telemetry packet to send back (UPDATED with autonomy data)
typedef struct __attribute__((packed)) {
  float   batteryVoltage;
  float   currentDraw;
  float   temperature;
  uint8_t moodState;
  uint8_t autonomousState;
  uint8_t safetyState;
  
  // Autonomy telemetry
  uint8_t autonomyEnabled;
  uint8_t autonomyState;
  float   sonarDistanceCm;
  float   compassHeading;
  float   gpsLatitude;
  float   gpsLongitude;
  uint8_t gpsValid;
  uint8_t waypointMode;
  float   waypointDistanceM;
  float   waypointBearingDeg;
  uint8_t currentWaypoint;
  uint8_t totalWaypoints;
} TelemetryPacket;

// Action codes
#define ACTION_NONE        0
#define ACTION_SCAN        1
#define ACTION_BEEP        2
#define ACTION_LOOKAROUND  3
#define ACTION_SLEEP       4
#define ACTION_WAKE        5
#define ACTION_IMU_CAL     6
#define ACTION_MOTOR_RESET 7

// System flags
#define FLAG_ESTOP      0x0001
#define FLAG_AUTONOMOUS 0x0002
#define FLAG_PRECISION  0x0004
#define FLAG_SUPERVISED 0x0008
#define FLAG_SLEEP      0x0010

extern unsigned long lastCommandMillis;
static uint8_t s_controllerMac[6] = {0};  // Remember controller MAC for telemetry send
static bool s_estopLast = false;  // Only print [E-STOP] on transition

// NEW: Track last received speeds for manual control detection
static int8_t s_lastLeftSpeed = 0;
static int8_t s_lastRightSpeed = 0;
static uint32_t s_lastManualCommandMs = 0;

// ESP-NOW callback (signature for ESP32 Arduino Core 3.x)
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len < (int)sizeof(ControlPacket)) return;

  const ControlPacket* p = (const ControlPacket*)data;
  
  // Remember controller MAC for telemetry replies (from info struct in 3.x)
  memcpy(s_controllerMac, info->src_addr, 6);

  // NEW: Track manual control activity
  s_lastLeftSpeed = p->leftSpeed;
  s_lastRightSpeed = p->rightSpeed;
  if (abs(p->leftSpeed) > 5 || abs(p->rightSpeed) > 5) {
    s_lastManualCommandMs = millis();
  }

  // E-STOP check (print only on transition to avoid serial spam)
  bool estop = (p->systemFlags & FLAG_ESTOP);
  if (estop) {
    motorStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = millis();
    if (!s_estopLast) Serial.println("[E-STOP]");
    s_estopLast = true;
    return;
  }
  s_estopLast = false;
  
  // NEW: Autonomy flag handling (TEMPORARILY DISABLED - autonomy not initialized)
  // bool autoFlagSet = (p->systemFlags & FLAG_AUTONOMOUS);
  // autonomySetEnabled(autoFlagSet);  // Enable/disable autonomy engine

  // Action handling (optional: implement these in your main)
  if (p->action != ACTION_NONE) {
    Serial.printf("[Action] %d\n", p->action);
    // Future: call servoScan(), beep(), etc.
  }

  // Map -100..+100 → -255..+255
  int16_t left  = (int16_t)((p->leftSpeed  * 255) / 100);
  int16_t right = (int16_t)((p->rightSpeed * 255) / 100);

  // Precision mode: half speed
  if (p->driveMode == 1) {
    left  /= 2;
    right /= 2;
  }

  left  = constrain(left,  -255, 255);
  right = constrain(right, -255, 255);

  lastCommandMillis = millis();
  // SWAP: Physical motors are wired opposite to code
  motorSetLeftRight(right, left);  // Swapped: was (left, right)

  displaySetCommand(CMD_DRIVE);
  float jx = (right - left) / 255.0f;
  float jy = -(left + right) / 255.0f;
  jx = constrain(jx, -1.0f, 1.0f);
  jy = constrain(jy, -1.0f, 1.0f);
  displaySetStick(jx, jy);
  uint8_t spd = (uint8_t)((abs(left) + abs(right)) / 2);
  displaySetSpeed(spd);
}

void espnowReceiverInit() {
  Serial.println("[ESP-NOW] Initializing...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init failed");
    return;
  }
  esp_now_register_recv_cb(onRecv);
  // AP MAC receives on channel 11 — use this when controller needs specific MAC
  Serial.print("[ESP-NOW] Receiver ready. Use this MAC for controller: ");
  Serial.println(WiFi.softAPmacAddress());
  
  // Print current WiFi channel
  uint8_t primaryChan;
  wifi_second_chan_t secondChan;
  esp_wifi_get_channel(&primaryChan, &secondChan);
  Serial.printf("[ESP-NOW] Listening on channel: %d\n", primaryChan);
}

void espnowSendTelemetry() {
  // Only send if we know controller MAC
  bool hasController = false;
  for (int i = 0; i < 6; i++) {
    if (s_controllerMac[i] != 0) {
      hasController = true;
      break;
    }
  }
  if (!hasController) return;

  // Check if controller is registered as a peer, if not add it
  static bool peerAdded = false;
  if (!peerAdded && hasController) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, s_controllerMac, 6);
    peerInfo.channel = 0;  // Use same channel as current WiFi
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_AP;  // Use AP interface for base
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
      Serial.print("[ESP-NOW] Controller peer added: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", s_controllerMac[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
      peerAdded = true;
    } else if (result == ESP_ERR_ESPNOW_EXIST) {
      // Already added, that's fine
      peerAdded = true;
    } else {
      Serial.printf("[ESP-NOW] Failed to add peer: %d\n", result);
      return;
    }
  }

  TelemetryPacket telem = {};
  
  // Battery data (from battery_monitor.h/cpp)
  const BatteryData& bat = batteryGetData();
  telem.batteryVoltage = bat.voltage;
  telem.currentDraw = bat.currentA;
  telem.temperature = 25.0f;  // Placeholder: add temp sensor if available
  telem.moodState = 0;        // Placeholder: 0=curious
  telem.autonomousState = 0;  // 0=manual
  telem.safetyState = 0;      // 0=ok

  // NEW: Autonomy telemetry (TEMPORARILY DISABLED - autonomy not initialized)
  // const AutoContext* ctx = autonomyGetContext();
  // const LocationState* loc = autonomyGetLocation();
  // const NavState* nav = waypointGetNavState();
  
  telem.autonomyEnabled = 0;  // Disabled
  telem.autonomyState = 0;    // Idle
  telem.sonarDistanceCm = 0.0f;
  telem.compassHeading = 0.0f;
  telem.gpsLatitude = 0.0;
  telem.gpsLongitude = 0.0;
  telem.gpsValid = 0;
  telem.waypointMode = 0;
  telem.waypointDistanceM = 0.0f;
  telem.waypointBearingDeg = 0.0f;
  telem.currentWaypoint = 0;
  telem.totalWaypoints = 0;

  esp_err_t result = esp_now_send(s_controllerMac, (uint8_t*)&telem, sizeof(TelemetryPacket));
  
  // Debug output every 50 sends (~5 seconds at 10Hz)
  static int sendCount = 0;
  sendCount++;
  if (sendCount >= 50) {
    Serial.printf("[Telemetry] Bat=%.2fV Auto=DISABLED\n", telem.batteryVoltage);
    sendCount = 0;
  }
}

// NEW: Check if manual control is active
bool espnowIsManualControlActive() {
  // Consider manual control active if any speed command > 5 received in last 500ms
  if (abs(s_lastLeftSpeed) > 5 || abs(s_lastRightSpeed) > 5) {
    return true;
  }
  // Or if recent command
  return (millis() - s_lastManualCommandMs) < 500;
}
