// ============================================================
// WALL-E Base radio receiver / telemetry
//
// The Base no longer owns an ESP-NOW implementation directly.
// radio_transport.cpp provides native ESP-NOW on radio-capable ESP32s
// and a UART radio-gateway transport on ESP32-P4.
// ============================================================

#include "espnow_receiver.h"
#include "radio_transport.h"
#include "motor_control.h"
#include "display_manager.h"
#include "battery_monitor.h"
#include "autonomy_engine.h"
#include "unified_autonomy_engine.h"
#include "waypoint_nav.h"
#include "dock_protocol.h"
#include "dock_homing.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "vision_protocol.h"
#include "vision_behaviour.h"
#include "audio_protocol.h"
#include "node_health_protocol.h"
#include "node_health_registry.h"
#include "motion_authority.h"
#include "audio_espnow.h"
#include "audio_telem.h"
#include "audio_esp_status.h"
#include "servo_manager.h"
#include "laser_control.h"
#include "imu_manager.h"
#include "walle_emotion_pose.h"
#include "../../protocols/walle_control_protocol.h"

#include <Arduino.h>
#include <cmath>
#include <cstring>

#define CONTROL_PACKET_HEADER_BYTES 7
#define CONTROL_PACKET_FULL_BYTES   ((int)sizeof(ControlPacket))

extern unsigned long lastCommandMillis;

static uint8_t s_controllerMac[6] = {0};
static bool s_estopLast = false;
static int8_t s_lastLeftSpeed = 0;
static int8_t s_lastRightSpeed = 0;
static uint32_t s_lastManualCommandMs = 0;

static bool macIsSet(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (mac[i] != 0) return true;
  }
  return false;
}

static int servoDegreesToPercent(uint8_t degrees) {
  return constrain(((int)degrees * 100) / 180, 0, 100);
}

// Translate the 10-slot controller packet into WALL-E's nine physical
// PCA9685 channels. Wire slot 1 is deliberately unused; the master now
// drives physical head tilt through the upper-neck slot (slot 4).
static void applyControllerServoTargets(const ControlPacket* p) {
  if (!p) return;

  servoSet(SERVO_HEAD_PAN,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_HEAD_PAN]), 85);
  servoSet(SERVO_NECK_TOP,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_NECK_TOP]), 85);
  servoSet(SERVO_NECK_BOT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_NECK_BOTTOM]), 85);
  servoSet(SERVO_EYE_LEFT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_EYE_LEFT]), 85);
  servoSet(SERVO_EYE_RIGHT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_EYE_RIGHT]), 85);
  servoSet(SERVO_ARM_LEFT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_LEFT_ARM]), 85);
  servoSet(SERVO_ARM_RIGHT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_RIGHT_ARM]), 85);
  servoSet(SERVO_BROW_RIGHT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_EYEBROW_RIGHT]), 85);
  servoSet(SERVO_BROW_LEFT,
           servoDegreesToPercent(p->servoTargets[WALLE_CTRL_SERVO_EYEBROW_LEFT]), 85);
}

static void onRadioPacket(const uint8_t sourceMac[6],
                          const uint8_t* data,
                          size_t length,
                          int8_t rssi,
                          uint8_t channel) {
  (void)channel;
  if (!data || length == 0) return;
  const int len = (int)length;

  audioTelemOnPacket(data, len);

  if (len >= (int)sizeof(WalleNodeHealthPacket_t)) {
    const WalleNodeHealthPacket_t* hp =
        reinterpret_cast<const WalleNodeHealthPacket_t*>(data);
    if (hp->magic == WALLE_NODE_HEALTH_MAGIC &&
        hp->version == WALLE_NODE_HEALTH_VERSION) {
      nodeHealthOnPacket(data, len);
      return;
    }
  }

  if (len >= (int)sizeof(DockBeaconPacket_t)) {
    const DockBeaconPacket_t* bp = reinterpret_cast<const DockBeaconPacket_t*>(data);
    if (bp->magic == DOCK_BEACON_MAGIC) {
      dockHomingOnBeacon(rssi);
      autonomousDockingOnBeacon(rssi);
      autonomousDockingSetLastDockId(bp->dock_id);
      autonomousDockingOnIrAlign(bp->ir_align_hint);
      nodeHealthOnDockBeacon(bp);
      if (bp->callout_active) autonomousDockingSetRequested(true);
      return;
    }
  }

  if (len >= (int)sizeof(WalleAudioStatusPacket_t)) {
    const WalleAudioStatusPacket_t* ap =
        reinterpret_cast<const WalleAudioStatusPacket_t*>(data);
    if (ap->magic == WALLE_AUDIO_STATUS_MAGIC) {
      audioEspStatusOnPacket(data, len);
      return;
    }
  }

  if (len >= (int)VISION_PACKET_SIZE) {
    const VisionPacket_t* vp = reinterpret_cast<const VisionPacket_t*>(data);
    if (vp->magic == VISION_MAGIC) {
      visionBehaviourOnPacket(vp, length);
      nodeHealthMarkVisionSeen();
      return;
    }
  }

  if (len < CONTROL_PACKET_HEADER_BYTES || !sourceMac) return;
  const ControlPacket* p = reinterpret_cast<const ControlPacket*>(data);

  memcpy(s_controllerMac, sourceMac, 6);
  const bool cydAllowed = motionAuthorityAllowCyd();

  if (cydAllowed) {
    s_lastLeftSpeed = p->leftSpeed;
    s_lastRightSpeed = p->rightSpeed;
    if (abs(p->leftSpeed) > 5 || abs(p->rightSpeed) > 5) {
      s_lastManualCommandMs = millis();
    }
  }

  const bool estop = (p->systemFlags & FLAG_ESTOP) != 0;
  if (estop) {
    motorStop();
    laserOff();
    autonomyEmergencyStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = millis();
    if (!s_estopLast) {
      Serial.println(F("[E-STOP] CYD emergency stop"));
      audioEspNowSendEvent(WALLE_AUDIO_EVT_ESTOP, WALLE_AUDIO_PRIORITY_ESTOP);
    }
    s_estopLast = true;
    return;
  }
  s_estopLast = false;

  autonomySetEnabled((p->systemFlags & FLAG_AUTONOMOUS) != 0);

  switch (p->action) {
    case ACTION_DOCK_GO:
#if USE_AUTONOMOUS_DOCKING
      autonomousDockingSetRequested(true);
#else
      dockHomingSetRequested(true);
#endif
      displayShowToast("Going to dock...");
      break;

    case ACTION_DOCK_CANCEL:
#if USE_AUTONOMOUS_DOCKING
      autonomousDockingSetRequested(false);
#else
      dockHomingSetRequested(false);
#endif
      displayShowToast("Dock cancelled");
      break;

    case ACTION_STOP_ALL:
      motorStop();
      laserOff();
      autonomySetEnabled(false);
      displaySetCommand(CMD_IDLE);
      Serial.println(F("[Action] STOP_ALL"));
      break;

    case ACTION_IMU_CAL:
      forceRecalibration();
      Serial.println(F("[Action] IMU recalibration"));
      break;

    case ACTION_AUTONOMY_REMOTE:
      if (len >= CONTROL_PACKET_FULL_BYTES) {
        autonomyApplyRemoteConfig(p->aux0, p->aux1);
      }
      break;

    case ACTION_NONE:
      break;

    default:
      Serial.printf("[Action] %u\n", (unsigned)p->action);
      break;
  }

  if (cydAllowed && len >= CONTROL_PACKET_FULL_BYTES) {
    laserSetBrightness((p->systemFlags & FLAG_LASER) ? 255 : 0);
    applyControllerServoTargets(p);
  }

#if USE_AUTONOMOUS_DOCKING
  if (autonomousDockingIsActive()) {
#else
  if (dockHomingIsActive()) {
#endif
    lastCommandMillis = millis();
    return;
  }

  int16_t left = (int16_t)((p->leftSpeed * 255) / 100);
  int16_t right = (int16_t)((p->rightSpeed * 255) / 100);
  if (p->driveMode == 1) {
    left /= 2;
    right /= 2;
  }
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  if (p->systemFlags & FLAG_PRECISION) {
    motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "CYD FLAG_PRECISION");
  } else {
    motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "CYD manual");
  }

  if (!cydAllowed) return;

  lastCommandMillis = millis();
  motorSetLeftRight(right, left);

  const bool moving = abs(left) > 10 || abs(right) > 10;
  displaySetCommand(moving ? CMD_DRIVE : CMD_IDLE);
  const float jx = constrain((right - left) / 255.0f, -1.0f, 1.0f);
  const float jy = constrain(-(left + right) / 255.0f, -1.0f, 1.0f);
  displaySetStick(jx, jy);
  displaySetSpeed((uint8_t)((abs(left) + abs(right)) / 2));
}

void espnowReceiverInit() {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  Serial.println(F("[Radio] Initialising ESP32-P4 gateway transport..."));
#else
  Serial.println(F("[Radio] Initialising native ESP-NOW transport..."));
#endif

  radioTransportSetReceiveCallback(onRadioPacket);
  if (!radioTransportInit()) {
    Serial.println(F("[Radio] Transport init failed"));
    return;
  }
  Serial.printf("[Radio] Ready, channel %u\n", (unsigned)radioTransportGetChannel());
}

void espnowReceiverHandle() {
  radioTransportPoll();
}

void espnowSendTelemetry() {
  if (!macIsSet(s_controllerMac) || !radioTransportIsReady()) return;

  TelemetryPacket telem = {};
  const BatteryData& bat = batteryGetData();
  telem.batteryVoltage = bat.voltage;
  telem.currentDraw = bat.currentA;

  const float chipTemp = temperatureRead();
  telem.temperature = std::isfinite(chipTemp) ? chipTemp : 0.0f;
  telem.moodState = (uint8_t)walleEmotionPoseGetState();
  telem.autonomousState = (uint8_t)unifiedAutonomyGetState();
  telem.safetyState = unifiedAutonomySafetyActive() ? 1u : 0u;

  const AutoContext* ctx = autonomyGetContext();
  const LocationState* loc = autonomyGetLocation();
  const NavState* nav = waypointGetNavState();

  telem.autonomyEnabled = autonomyIsEnabled() ? 1u : 0u;
  telem.autonomyState = (uint8_t)autonomyGetState();
  telem.sonarDistanceCm = ctx ? ctx->detectedDistance : 0.0f;
  telem.compassHeading = loc ? loc->heading : 0.0f;
  telem.gpsLatitude = loc ? (float)loc->latitude : 0.0f;
  telem.gpsLongitude = loc ? (float)loc->longitude : 0.0f;
  telem.gpsValid = (loc && loc->gpsValid) ? 1u : 0u;
  telem.waypointMode = autonomyIsWaypointMode() ? 1u : 0u;
  telem.waypointDistanceM = nav ? nav->distanceToWaypoint : 0.0f;
  telem.waypointBearingDeg = nav ? nav->bearingToWaypoint : 0.0f;
  telem.currentWaypoint = nav ? nav->currentWaypointIndex : 0u;
  telem.totalWaypoints = waypointGetCount();

  if (!radioTransportSend(s_controllerMac, &telem, sizeof(telem))) {
    static uint32_t lastWarn = 0;
    if (millis() - lastWarn > 2000u) {
      lastWarn = millis();
      Serial.println(F("[Telemetry] Send failed"));
    }
  }

  static uint8_t sendDebugCount = 0;
  if (++sendDebugCount >= 50) {
    sendDebugCount = 0;
    Serial.printf("[Telemetry] Bat=%.2fV Auto=%s Safety=%u RadioRX=%lu TX=%lu\n",
                  telem.batteryVoltage,
                  telem.autonomyEnabled ? "ON" : "OFF",
                  (unsigned)telem.safetyState,
                  (unsigned long)radioTransportGetRxCount(),
                  (unsigned long)radioTransportGetTxCount());
  }
}

bool espnowIsManualControlActive() {
  if (abs(s_lastLeftSpeed) > 5 || abs(s_lastRightSpeed) > 5) return true;
  return (millis() - s_lastManualCommandMs) < 500u;
}
