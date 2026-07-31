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
#include "dock_controller.h"
#include "walle_link_packet.h"
#include "eve_uart_bridge.h"
#include <string.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define CONTROL_PACKET_HEADER_BYTES  7
#define CONTROL_PACKET_FULL_BYTES    (int)WALLE_CTRL_OBSOLETE_BYTES

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
#define ACTION_EVE_UART_SERVO   13  /* aux0=head deg 45–135, aux1=right arm 0–180 */

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
static uint32_t s_lastRejectedControlLogMs = 0;

// NEW: Track last received speeds for manual control detection
static int8_t s_lastLeftSpeed = 0;
static int8_t s_lastRightSpeed = 0;
static uint32_t s_lastManualCommandMs = 0;

/* CYD control v2: sequence + CRC (legacy 19 B unchanged for old CYD) */
static uint16_t s_lastAcceptedCydSeq = 0xFFFFu;
static uint8_t  s_telemAckLatch = 0;
static uint32_t s_lastBadCrcLogMs = 0;
static uint32_t s_lastOldSeqLogMs = 0;
static uint32_t s_lastSeqGapLogMs = 0;
static uint32_t s_lastSeqGapEventMs = 0;
static uint32_t s_lastTelemTxFailMs = 0;
static uint8_t  s_telemSendFailStreak = 0;
static bool     s_ctrlPeerReady = false;
static bool     s_pendingCtrlPeerRebind = false;
static uint16_t s_apiCommsLastSeq = 0;
static uint8_t  s_apiCommsLastAck = 0;

#define TELEM_RSV0_STICKY_MS 2000u
#define PEER_REBIND_FAIL_STREAK 4u

static bool hasKnownControllerMac(void) {
  for (int i = 0; i < 6; i++) {
    if (s_controllerMac[i] != 0) {
      return true;
    }
  }
  return false;
}

static bool isControlSourceAllowed(const esp_now_recv_info_t* info) {
  if (!info) {
    return false;
  }
  if (!hasKnownControllerMac()) {
    memcpy(s_controllerMac, info->src_addr, 6);
    Serial.print("[ESP-NOW] Bound controller MAC: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", s_controllerMac[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
    return true;
  }
  if (memcmp(s_controllerMac, info->src_addr, 6) == 0) {
    return true;
  }
  uint32_t now = millis();
  if ((uint32_t)(now - s_lastRejectedControlLogMs) > 1000u) {
    s_lastRejectedControlLogMs = now;
    Serial.println("[ESP-NOW] Rejected control packet from unknown MAC");
  }
  return false;
}

static void onBaseTelemetrySent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  if (status != ESP_NOW_SEND_SUCCESS) {
    s_lastTelemTxFailMs = millis();
    if (s_telemSendFailStreak < 255) {
      s_telemSendFailStreak++;
    }
    if (s_telemSendFailStreak >= PEER_REBIND_FAIL_STREAK) {
      s_pendingCtrlPeerRebind = true;
    }
  } else {
    s_telemSendFailStreak = 0;
  }
}

static void baseRecoverControllerPeerIfNeeded() {
  if (!s_pendingCtrlPeerRebind || !hasKnownControllerMac()) {
    return;
  }
  s_pendingCtrlPeerRebind = false;
  esp_err_t d = esp_now_del_peer(s_controllerMac);
  (void)d;
  s_ctrlPeerReady = false;
  s_telemSendFailStreak = 0;
  Serial.println(F("[ESP-NOW] Rebound CYD peer after telemetry send failures"));
}

// ESP-NOW callback (signature for ESP32 Arduino Core 3.x)
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  audioTelemOnPacket(data, len);

  if (len >= 8) {
    const WalleNodeHealthPacket_t* hp = (const WalleNodeHealthPacket_t*)data;
    if (hp->magic == WALLE_NODE_HEALTH_MAGIC) {
      if ((hp->version == 1 && len >= (int)WALLE_NODE_HEALTH_V1_SIZE) ||
          (hp->version == 2 && len >= (int)WALLE_NODE_HEALTH_V2_SIZE)) {
        nodeHealthOnPacket(data, len);
        return;
      }
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

  if (len < (int)WALLE_CTRL_OBSOLETE_BYTES) return;
  if (!isControlSourceAllowed(info)) return;

  ControlPacket cp = {};
  memcpy(&cp, data, (size_t)len < sizeof(ControlPacket) ? (size_t)len : sizeof(ControlPacket));
  const bool legacy = (len < (int)WALLE_CTRL_PACKET_V2);
  if (!legacy) {
    if (len < (int)WALLE_CTRL_PACKET_V2) return;
    if (walle_crc8_dallas((const uint8_t*)&cp, WALLE_CTRL_CRC_LEN) != cp.crc8) {
      uint32_t t = millis();
      if (t - s_lastBadCrcLogMs > 1000u) {
        s_lastBadCrcLogMs = t;
        Serial.println(F("[CTRL] CRC8 mismatch (dropped)"));
      }
      return;
    }
  }
  {
    const bool estopF = (cp.systemFlags & FLAG_ESTOP) != 0;
    if (!legacy && !estopF) {
      const uint16_t d = (uint16_t)(cp.seq - s_lastAcceptedCydSeq);
      if (s_lastAcceptedCydSeq != 0xFFFF) {
        if (d == 0) return;
        if (d > 0x7FFF) {
          uint32_t t = millis();
          if (t - s_lastOldSeqLogMs > 500u) {
            s_lastOldSeqLogMs = t;
            Serial.println(F("[CTRL] Stale or replay seq (dropped)"));
          }
          return;
        }
        if (d > (uint16_t)WALLE_CTRL_MAX_SEQ_JUMP) {
          s_lastSeqGapEventMs = millis();
          uint32_t t = millis();
          if (t - s_lastSeqGapLogMs > 500u) {
            s_lastSeqGapLogMs = t;
            Serial.println(F("[CTRL] Seq jump too large (dropped, link flag)"));
          }
          return;
        }
      }
    }
    if (!legacy) s_lastAcceptedCydSeq = cp.seq;
  }
  const ControlPacket* p = &cp;
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
    s_telemAckLatch |= (uint8_t)WALLE_ACK_ESTOP;
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
  } else if (p->action == ACTION_AUTONOMY_REMOTE && len >= 19) {
    autonomyApplyRemoteConfig(p->aux0, p->aux1);
  } else if (p->action == ACTION_MOTION_POLICY && len >= 19) {
    if (p->aux0 <= (uint8_t)MOTION_AUTH_WEB_ONLY) {
      motionAuthoritySet((MotionAuthorityMode)p->aux0);
      s_telemAckLatch |= (uint8_t)WALLE_ACK_MOTION_POLICY;
      Serial.printf("[MotionAuth] CYD policy -> %s\n", motionAuthorityModeName((MotionAuthorityMode)p->aux0));
    }
  } else if (p->action == ACTION_EVE_UART_SERVO && len >= 19) {
    (void)eveUartBridgeSendCydServo(p->aux0, p->aux1);
  } else if (p->action != ACTION_NONE) {
    Serial.printf("[Action] %d\n", p->action);
  }

  /* Servo + laser from CYD (0–180° in packet → base uses 0–100 scale) */
  if (cydAllowed && len >= (int)WALLE_CTRL_OBSOLETE_BYTES) {
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
  esp_now_register_send_cb(onBaseTelemetrySent);
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

  baseRecoverControllerPeerIfNeeded();

  if (!s_ctrlPeerReady && hasController) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, s_controllerMac, 6);
    peerInfo.channel = 0;  // Use same channel as current WiFi
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_AP;  // Use AP interface for base

    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
      Serial.print(F("[ESP-NOW] Controller peer added: "));
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", s_controllerMac[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println();
      s_ctrlPeerReady = true;
    } else if (result == ESP_ERR_ESPNOW_EXIST) {
      s_ctrlPeerReady = true;
    } else {
      Serial.print(F("[ESP-NOW] Failed to add peer: "));
      Serial.println((int)result);
      return;
    }
  }

  TelemetryPacket telem = {};
  telem.magic = WALLE_TELEMETRY_MAGIC;
  telem.version = (uint8_t)WALLE_TELEMETRY_VERSION;
  
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

  {
    if (s_lastAcceptedCydSeq == 0xFFFF) {
      telem.last_control_seq_applied = 0;
    } else {
      telem.last_control_seq_applied = s_lastAcceptedCydSeq;
    }
    uint8_t a = s_telemAckLatch;
    a |= dockControllerGetLiveAckMask();
    telem.control_ack_bits = a;
    s_telemAckLatch = 0;
    {
      uint8_t rsv = 0;
      const uint32_t tnow = millis();
      if (s_lastSeqGapEventMs != 0 &&
          (uint32_t)(tnow - s_lastSeqGapEventMs) < TELEM_RSV0_STICKY_MS) {
        rsv = (uint8_t)(rsv | WALLE_TELEM_RSV0_SEQ_GAP);
      }
      if (s_lastTelemTxFailMs != 0 &&
          (uint32_t)(tnow - s_lastTelemTxFailMs) < TELEM_RSV0_STICKY_MS) {
        rsv = (uint8_t)(rsv | WALLE_TELEM_RSV0_TELEM_TX_FAIL);
      }
      if (eveUartBridgeIsLinkUp()) {
        rsv = (uint8_t)(rsv | WALLE_TELEM_RSV0_EVE_UART);
      }
      telem.rsv0 = rsv;
    }
    s_apiCommsLastSeq = telem.last_control_seq_applied;
    s_apiCommsLastAck = telem.control_ack_bits;
  }

  esp_err_t result = esp_now_send(s_controllerMac, (uint8_t*)&telem, sizeof(TelemetryPacket));
  if (result != ESP_OK) {
    s_lastTelemTxFailMs = millis();
    s_pendingCtrlPeerRebind = true;
  }
  
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

void espnowReceiverGetCydCommsForApi(uint16_t* lastSeq, uint8_t* ackBits) {
  if (lastSeq) *lastSeq = s_apiCommsLastSeq;
  if (ackBits) *ackBits = s_apiCommsLastAck;
}
