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
#include "dock_protocol.h"
#include "dock_homing.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "vision_protocol.h"
#include "vision_behaviour.h"
#include "audio_protocol.h"
#include "menu_protocol.h"
#include "audio_ui_telemetry.h"
#include "shared_voicebox_manager.h"
#include "node_health_protocol.h"
#include "node_health_registry.h"
#include "motion_authority.h"
#include "audio_espnow.h"
#include "audio_telem.h"
#include "audio_esp_status.h"
#include "servo_manager.h"
#include "laser_control.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Must match wall_e_master_controller/protocol.h ControlPacket (packed)
typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;
  int8_t   rightSpeed;
  uint8_t  driveMode;
  uint8_t  behaviourMode;
  uint8_t  action;
  uint16_t systemFlags;
  uint8_t  servoTargets[10];
  uint8_t  aux0;
  uint8_t  aux1;
} ControlPacket;

#define CONTROL_PACKET_HEADER_BYTES  7
#define CONTROL_PACKET_FULL_BYTES    (int)sizeof(ControlPacket)

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
  uint8_t motionPolicy;
  uint8_t policyDenyCyd;
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
#define ACTION_DOCK_GO     8
#define ACTION_DOCK_CANCEL 9
#define ACTION_STOP_ALL    10
#define ACTION_AUTONOMY_REMOTE  11
#define ACTION_MOTION_POLICY    12

// System flags
#define FLAG_ESTOP      0x0001
#define FLAG_AUTONOMOUS 0x0002
#define FLAG_PRECISION  0x0004
#define FLAG_SUPERVISED 0x0008
#define FLAG_SLEEP      0x0010
#define FLAG_LASER      0x0020

extern unsigned long lastCommandMillis;
static uint8_t s_controllerMac[6] = {0};  // Remember controller MAC for telemetry send
static bool s_estopLast = false;  // Only print [E-STOP] on transition

// NEW: Track last received speeds for manual control detection
static int8_t s_lastLeftSpeed = 0;
static int8_t s_lastRightSpeed = 0;
static uint32_t s_lastManualCommandMs = 0;

// ESP-NOW callback (signature for ESP32 Arduino Core 3.x)
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  audioTelemOnPacket(data, len);

  if (len >= (int)sizeof(WalleNodeHealthPacket_t)) {
    const WalleNodeHealthPacket_t* hp = (const WalleNodeHealthPacket_t*)data;
    if (hp->magic == WALLE_NODE_HEALTH_MAGIC && hp->version == WALLE_NODE_HEALTH_VERSION) {
      nodeHealthOnPacket(data, len);
      return;
    }
  }

  /* Dock beacon: homing target. Feed RSSI and dock_id to both systems. */
  if (len >= (int)sizeof(DockBeaconPacket_t)) {
    const DockBeaconPacket_t* bp = (const DockBeaconPacket_t*)data;
    if (bp->magic == DOCK_BEACON_MAGIC) {
      int8_t rssi = -80;
      if (info && info->rx_ctrl) rssi = (int8_t)info->rx_ctrl->rssi;
      dockHomingOnBeacon(rssi);
      autonomousDockingOnBeacon(rssi);
      autonomousDockingSetLastDockId(bp->dock_id);
      autonomousDockingOnIrAlign(bp->ir_align_hint);
      nodeHealthOnDockBeacon(bp);
      return;
    }
  }

  /* Audio ESP UI telem (buttons, menu, voicebox echo, pair requests) */
  if (len >= (int)sizeof(WalleAudioUiTelemPacket_t)) {
    const WalleAudioUiTelemPacket_t* up = (const WalleAudioUiTelemPacket_t*)data;
    if (up->magic == WALLE_AUDIO_UI_MAGIC) {
      audioUiTelemOnPacket(data, len);
      sharedVoiceboxOnAudioUi(up);
      return;
    }
  }

  /* Audio ESP status: mic dir, dock IR, voice cmd, mode, fault */
  if (len >= (int)sizeof(WalleAudioStatusPacket_t)) {
    const WalleAudioStatusPacket_t* ap = (const WalleAudioStatusPacket_t*)data;
    if (ap->magic == WALLE_AUDIO_STATUS_MAGIC) {
      audioEspStatusOnPacket(data, len);
      return;
    }
  }

  /* Vision packet: motion/centroid from camera node → vision behaviour (servo tracking). */
  if (len >= (int)VISION_PACKET_SIZE) {
    const VisionPacket_t* vp = (const VisionPacket_t*)data;
    if (vp->magic == VISION_MAGIC) {
      visionBehaviourOnPacket(vp, (size_t)len);
      return;
    }
  }

  if (len < CONTROL_PACKET_HEADER_BYTES) return;

  const ControlPacket* p = (const ControlPacket*)data;
  
  // Remember controller MAC for telemetry replies (from info struct in 3.x)
  memcpy(s_controllerMac, info->src_addr, 6);

  const bool cydAllowed = motionAuthorityAllowCyd();

  /* Always track CYD stick intent for /api/motion/operator POLICY + telemetry policyDenyCyd */
  s_lastLeftSpeed = p->leftSpeed;
  s_lastRightSpeed = p->rightSpeed;
  if (abs(p->leftSpeed) > 5 || abs(p->rightSpeed) > 5) {
    s_lastManualCommandMs = millis();
  }

  // E-STOP check (print only on transition to avoid serial spam)
  bool estop = (p->systemFlags & FLAG_ESTOP);
  if (estop) {
    motorStop();
    laserOff();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = millis();
    if (!s_estopLast) {
      Serial.println("[E-STOP]");
      audioEspNowSendEvent(WALLE_AUDIO_EVT_ESTOP, WALLE_AUDIO_PRIORITY_ESTOP);
    }
    s_estopLast = true;
    return;
  }
  s_estopLast = false;
  
  bool autoFlagSet = (p->systemFlags & FLAG_AUTONOMOUS);
  autonomySetEnabled(autoFlagSet);

  // Action handling (dock, etc.)
  if (p->action == ACTION_DOCK_GO) {
#if USE_AUTONOMOUS_DOCKING
    autonomousDockingSetRequested(true);
#else
    dockHomingSetRequested(true);
#endif
    displayShowToast("Going to dock...");
  } else if (p->action == ACTION_DOCK_CANCEL) {
#if USE_AUTONOMOUS_DOCKING
    autonomousDockingSetRequested(false);
#else
    dockHomingSetRequested(false);
#endif
    displayShowToast("Dock cancelled");
  } else if (p->action == ACTION_STOP_ALL) {
    motorStop();
    laserOff();
    displaySetCommand(CMD_IDLE);
    Serial.println(F("[Action] STOP_ALL"));
  } else if (p->action == ACTION_AUTONOMY_REMOTE && len >= (int)sizeof(ControlPacket)) {
    autonomyApplyRemoteConfig(p->aux0, p->aux1);
  } else if (p->action == ACTION_MOTION_POLICY && len >= (int)sizeof(ControlPacket)) {
    if (p->aux0 <= (uint8_t)MOTION_AUTH_WEB_ONLY) {
      motionAuthoritySet((MotionAuthorityMode)p->aux0);
      Serial.printf("[MotionAuth] CYD policy -> %s\n", motionAuthorityModeName((MotionAuthorityMode)p->aux0));
    }
  } else if (p->action != ACTION_NONE) {
    Serial.printf("[Action] %d\n", p->action);
  }

  /* Servo + laser from CYD (0–180° in packet → base uses 0–100 scale) */
  if (cydAllowed && len >= CONTROL_PACKET_FULL_BYTES) {
    if (p->systemFlags & FLAG_LASER) {
      laserSetBrightness(255);
    } else {
      laserSetBrightness(0);
    }
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
      int pos = ((int)p->servoTargets[i] * 100) / 180;
      if (pos < 0) pos = 0;
      if (pos > 100) pos = 100;
      servoSet(i, pos, 85);
    }
  }

  /* When dock homing or autonomous docking active, don't apply drive commands */
#if USE_AUTONOMOUS_DOCKING
  if (autonomousDockingIsActive()) {
#else
  if (dockHomingIsActive()) {
#endif
    lastCommandMillis = millis();
    return;
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

  if (!cydAllowed) {
    return;
  }

  /* Drive profile: only when CYD may command motors (avoid web_only profile fighting HTTP) */
  if (p->systemFlags & FLAG_PRECISION) {
    motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "CYD FLAG_PRECISION");
  } else {
    motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "CYD manual");
  }

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
  telem.moodState = 0;
  telem.autonomousState = autonomyIsEnabled() ? 1 : 0;
  telem.safetyState = 0;

  const AutoContext* ctx = autonomyGetContext();
  const LocationState* loc = autonomyGetLocation();
  const NavState* nav = waypointGetNavState();

  telem.autonomyEnabled = autonomyIsEnabled() ? 1 : 0;
  telem.autonomyState = (uint8_t)autonomyGetState();
  telem.sonarDistanceCm = ctx ? ctx->detectedDistance : 0.0f;
  telem.compassHeading = loc ? loc->heading : 0.0f;
  telem.gpsLatitude = loc ? (float)loc->latitude : 0.0f;
  telem.gpsLongitude = loc ? (float)loc->longitude : 0.0f;
  telem.gpsValid = (loc && loc->gpsValid) ? 1 : 0;
  telem.waypointMode = autonomyIsWaypointMode() ? 1 : 0;
  telem.waypointDistanceM = nav ? nav->distanceToWaypoint : 0.0f;
  telem.waypointBearingDeg = nav ? nav->bearingToWaypoint : 0.0f;
  telem.currentWaypoint = nav ? nav->currentWaypointIndex : 0;
  telem.totalWaypoints = waypointGetCount();

  telem.motionPolicy = (uint8_t)motionAuthorityGet();
  {
    bool sticksIntent = (abs(s_lastLeftSpeed) > 5 || abs(s_lastRightSpeed) > 5);
    telem.policyDenyCyd =
        (telem.motionPolicy == (uint8_t)MOTION_AUTH_WEB_ONLY && sticksIntent) ? 1 : 0;
  }

  esp_err_t result = esp_now_send(s_controllerMac, (uint8_t*)&telem, sizeof(TelemetryPacket));
  
  // Debug output every 50 sends (~5 seconds at 10Hz)
  static int sendCount = 0;
  sendCount++;
  if (sendCount >= 50) {
    Serial.printf("[Telemetry] Bat=%.2fV Auto=%s\n", telem.batteryVoltage,
                  telem.autonomyEnabled ? "ON" : "OFF");
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
