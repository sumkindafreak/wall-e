// ============================================================
// Arduino IDE: open this folder ("main") and main.ino.
// WALL-E Base Brain — ESP32-S3 production target
// Motor: L298N Dual H-Bridge
// Radio: native Wi-Fi / ESP-NOW on the Base S3
// ============================================================

#include <Arduino.h>
#include "motor_control.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "display_manager.h"
#include "servo_manager.h"
#include "imu_manager.h"
#include "battery_monitor.h"
#include "flashlight_control.h"
#include "espnow_receiver.h"
#include "vl53l1x_tof.h"
#include "dock_sensors.h"
#include "dock_homing.h"
#include "autonomous_docking.h"
#include "dock_controller.h"
#include "dock_ir_transmitters.h"
#include "dock_config.h"
#include "vision_behaviour.h"
#include "audio_espnow.h"
#include "node_health_registry.h"
#include "walle_emotion_pose_bridge.h"
#include "walle_emotion_pose.h"

#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
#include "waypoint_nav.h"
#include "personality_engine.h"
#include "emotion_engine.h"
#include "interest_engine.h"
#include "memory_engine.h"
#include "return_home_engine.h"
#include "autonomy_engine.h"
#include "unified_autonomy_engine.h"
#include "motion_layer.h"
#include "motion_authority.h"
#include "api_security.h"
#include "eve_uart_bridge.h"
#include "laser_control.h"
#include "sequence_engine.h"

#define FAILSAFE_TIMEOUT_MS 500UL

unsigned long lastCommandMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[WALL-E/S3] Starting Base Brain...");

  // Motors safe first — always.
  motorInit();
  motionLayerInit();
  Serial.println("[Motors] Initialised");

  displayInit();

  // Shared I2C bus: PCA9685 starts the bus; IMU follows.
  servoInit();
  servoNeutral(SERVO_SLOW_SPEED);
  beginIMU();
  Serial.println("[IMU] Init complete");

  // Effects start in a deterministic safe state before any remote command can
  // arrive. laserInit() attaches PWM and forces the output OFF.
  laserInit();

  // Autonomy stack. Optional sensors fail gracefully.
  if (!sonarInit()) Serial.println(F("[Sonar] WARN: init failed"));
  else Serial.println(F("[Sonar] Ready"));

  if (!compassInit()) Serial.println(F("[Compass] WARN: not available (continuing)"));
  else Serial.println(F("[Compass] Ready"));

  if (!gpsInit()) Serial.println(F("[GPS] WARN: init failed"));
  else Serial.println(F("[GPS] Ready"));

  waypointInit();
  personalityInit();
  emotionInit();
  interestInit();
  memoryInit();
  returnHomeInit();
  autonomyInit();
  unifiedAutonomyInit();
  Serial.println(F("[Autonomy] Engine ready"));

  batteryInit();
  displayUpdateBattery();

  dockSensorsBegin();

  // The Base S3 owns Wi-Fi directly. ESP-NOW uses the same radio hardware,
  // while radio_transport keeps packet ownership in one place.
  wifiManagerInit();
  displayUpdateWifi();

  motionAuthorityInit();
  apiSecurityInit();
  eveUartBridgeInit();

  // Sequence persistence/state exists independently of the HTTP API, so
  // initialize the engine before registering WebUI routes.
  sequenceEngineInit();
  webServerInit();

  // Native ESP-NOW receive/send abstraction on the Base S3.
  espnowReceiverInit();
  audioEspNowInit();
  nodeHealthInit();
  walleEmotionPoseBridgeInit();

  visionBehaviourInit();
  dockControllerInit();
  dockIrTransmittersInit();
  autonomousDockingInit();
  flashlightInit();

  lastCommandMillis = millis();
  Serial.println("[WALL-E/S3] Ready");
}

unsigned long lastTelemSendMs = 0;
#define TELEM_SEND_INTERVAL_MS 100

void loop() {
  delay(1);
  const unsigned long now = millis();
  yield();

  // Native ESP-NOW is callback-driven; this remains as the transport service
  // hook so higher-level code does not depend on the radio implementation.
  espnowReceiverHandle();

  WiFiState prevState = wifiGetState();
  wifiManagerHandle();
  if (wifiGetState() != prevState) displayUpdateWifi();

  webServerHandle();
  eveUartBridgePoll();
  sequenceEngineTick(now);

  servoHandle();
  laserUpdate(now);
  visionBehaviourUpdate(now);

  updateIMU();

  sonarUpdate(now);
  compassUpdate(now);
  gpsUpdate(now);
  if (gpsHasFix()) {
    waypointUpdate(gpsGetLatitude(), gpsGetLongitude(), compassGetHeading());
    gpsFeedToMemory(now);
  }
  personalityUpdate(now);

  const bool manualFromControl =
      (motionAuthorityAllowCyd() && espnowIsManualControlActive()) ||
      (motionAuthorityAllowWeb() && webServerIsManualOverrideActive());
  autonomySetManualOverride(manualFromControl);

  int8_t autoLeft = 0;
  int8_t autoRight = 0;
  autonomyUpdate(now, &autoLeft, &autoRight);

  if (batteryHandle()) displayUpdateBattery();

  static bool tofTried = false;
  if (!tofTried) {
    tofTried = true;
    tofInit();
  }
  tofUpdate(now);
  dockSensorsUpdate();
  dockIrTransmittersUpdate(now);

  bool dockDriving = false;
#if USE_AUTONOMOUS_DOCKING
  autonomousDockingUpdate(now);
#else
  dockHomingCheckAutoReturn(now);
  dockHomingUpdate(now);
#endif

  unifiedAutonomyTick(now, manualFromControl);
  const bool safetyBlock = unifiedAutonomySafetyActive();
  if (safetyBlock) {
    motorStop();
    lastCommandMillis = now;
  }

  motionLayerUpdate(now, manualFromControl, safetyBlock);

#if USE_AUTONOMOUS_DOCKING
  if (!safetyBlock && autonomousDockingIsActive()) {
    int16_t left = 0;
    int16_t right = 0;
    if (autonomousDockingGetMotorOutput(&left, &right)) {
      motionLayerApplyMotorTankLimits(&left, &right);
      motorSetLeftRight(left, right);
      lastCommandMillis = now;
      dockDriving = true;
    }
  }
#else
  if (!safetyBlock && dockHomingIsActive()) {
    int16_t left = 0;
    int16_t right = 0;
    if (dockHomingGetMotorOutput(&left, &right)) {
      motionLayerApplyMotorTankLimits(&left, &right);
      motorSetLeftRight(left, right);
      lastCommandMillis = now;
      dockDriving = true;
    }
  }
#endif

  if (!safetyBlock && !dockDriving && autonomyIsEnabled() && !manualFromControl) {
    int16_t ml = (int16_t)((int32_t)autoLeft * 255 / 100);
    int16_t mr = (int16_t)((int32_t)autoRight * 255 / 100);
    ml = (int16_t)constrain((int)ml, -255, 255);
    mr = (int16_t)constrain((int)mr, -255, 255);
    motionLayerApplyMotorTankLimits(&ml, &mr);
    motorSetLeftRight(ml, mr);
    lastCommandMillis = now;
  }

  flashlightHandle();
  displayHandle();

  if ((now - lastTelemSendMs) >= TELEM_SEND_INTERVAL_MS) {
    espnowSendTelemetry();
    lastTelemSendMs = now;
  }

  nodeHealthTick();
  walleEmotionPoseBridgeTick();
  walleEmotionPoseApplyToServosStub();

  if ((now - lastCommandMillis) > FAILSAFE_TIMEOUT_MS) {
    motorStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = now;
  }

  motorHandle();

  delay(1);
  yield();
}
