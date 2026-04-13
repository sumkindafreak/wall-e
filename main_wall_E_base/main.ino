// ============================================================
//  PlatformIO: this file is the sketch entry (see platformio.ini).
//  Arduino IDE: open folder "main_wall_E_base/main" and use main.ino
//  there — sketch folder name must match the .ino name ("main").
//  See ARDUINO_IDE_QUICK_START.md
// ============================================================
//  WALL-E Simple WebUI + Tank Drive Controller
//  Platform:   ESP32-S3 Dev Module
//  Motor:      L298N Dual H-Bridge
//  Boot mode:  AP-first (always reachable at 192.168.4.1)
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

// NEW: Autonomy and behavioral brain includes
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
#include "eve_target_assist.h"
#include "memory_manager.h"
#include "relationship_manager.h"
#include "shared_voicebox_manager.h"
#include "autonomy_manager.h"
#include "eve_link_manager.h"
#include "laser_control.h"
#include "sequence_engine.h"

// ============================================================
//  Failsafe
// ============================================================
#define FAILSAFE_TIMEOUT_MS 500UL

unsigned long lastCommandMillis = 0;

// ============================================================
//  setup()
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[WALL-E] Starting...");

  // Motors safe first — always
  motorInit();
  motionLayerInit();
  Serial.println("[Motors] Initialised");

  /* Power bus sense first — chest boot animation reads V/A from real ADC */
  batteryInit();
  Serial.println("[Setup] Post-battery (before chest display)");

  displayInit();

  // I2C bus: servos init first (calls Wire.begin)
  servoInit();
  servoNeutral(SERVO_SLOW_SPEED);

  // IMU shares same I2C bus — init and start auto-calibration (~3 s when stationary)
  beginIMU();
  Serial.println("[IMU] Init complete");

  // Autonomy stack: sensors + behavioural engines (safe if compass/GPS absent)
  if (!sonarInit()) {
    Serial.println(F("[Sonar] WARN: init failed"));
  } else {
    Serial.println(F("[Sonar] Ready"));
  }
  if (!compassInit()) {
    Serial.println(F("[Compass] WARN: not available (continuing)"));
  } else {
    Serial.println(F("[Compass] Ready"));
  }
  if (!gpsInit()) {
    Serial.println(F("[GPS] WARN: init failed"));
  } else {
    Serial.println(F("[GPS] Ready"));
  }
  waypointInit();
  Serial.println(F("[Waypoint] Ready"));
  personalityInit();
  emotionInit();
  interestInit();
  memoryInit();
  returnHomeInit();
  autonomyInit();
  unifiedAutonomyInit();
  Serial.println(F("[Autonomy] Engine ready"));

  displayUpdateBattery();

  // Dock system: sensors, homing. ToF deferred (can block if sensor absent)
  dockSensorsBegin();
  Serial.println("[Setup] Post-dockSensors");

  // WiFi — AP starts immediately
  wifiManagerInit();
  displayUpdateWifi();
  Serial.println("[Setup] Post-wifi");

  // Web server
  webServerInit();
  Serial.println("[Setup] Post-webServer");

  // ESP-NOW receiver (CYD controller)
  espnowReceiverInit();
  Serial.println("[Setup] Post-espnow");
  audioEspNowInit();
  Serial.println("[Setup] Post-audioEspNow");

  memoryManagerInit();
  relationshipInit();
  eveUartBridgeInit();
  eveTargetAssistInit();
  sharedVoiceboxInit();
  autonomyManagerInit();
  eveLinkManagerInit();
  Serial.println("[Setup] Post-living-core");

  nodeHealthInit();
  Serial.println("[Setup] Post-nodeHealth");
  walleEmotionPoseBridgeInit();
  Serial.println("[Setup] Post-emotionPoseBridge");

  // Vision behaviour (servo tracking from camera node ESP-NOW packets)
  visionBehaviourInit();
  Serial.println("[Setup] Post-visionBehaviour");

  // Dock controller (ESP-NOW commands to dock, e.g. REQUEST_CHARGE)
  dockControllerInit();
  Serial.println("[Setup] Post-dockController");

  dockIrTransmittersInit();
  Serial.println("[Setup] Post-dockIrTx");

  // Autonomous docking state machine
  autonomousDockingInit();
  Serial.println("[Setup] Post-autonomousDocking");

  // LDR + MOSFET flashlight (on when dark)
  flashlightInit();
  Serial.println("[Setup] Post-flashlight");

  lastCommandMillis = millis();
  Serial.println("[WALL-E] Ready");
}

// ============================================================
//  loop()
// ============================================================
unsigned long lastTelemSendMs = 0;
#define TELEM_SEND_INTERVAL_MS 100  // 10 Hz telemetry updates

void loop() {
  delay(1);  /* Yield first — prevents TG1WDT before any blocking */
  unsigned long now = millis();
  yield();
  
  // WiFi state polling
  WiFiState prevState = wifiGetState();
  wifiManagerHandle();
  if (wifiGetState() != prevState) displayUpdateWifi();

  // Web requests
  webServerHandle();
  eveUartBridgePoll();
  memoryManagerTick();
  relationshipTick(now);
  sharedVoiceboxTick(now);
  autonomyManagerTick(now);
  eveLinkManagerTick(now);
  sequenceEngineTick(now);

  // Servo velocity interpolation
  servoHandle();
  laserUpdate(now);

  // Vision behaviour (scan/timeouts when no motion; uses packets from ESP-NOW callback)
  visionBehaviourUpdate(now);

  // IMU: update (runs calibration until done, then provides offset-corrected data)
  updateIMU();
  // Behaviour that uses imuGetData() should check isIMUCalibrated() before use

  // Autonomy: sensors + engine (drive applied after docking when not manually overridden)
  sonarUpdate(now);
  compassUpdate(now);
  gpsUpdate(now);
  if (gpsHasFix()) {
    waypointUpdate(gpsGetLatitude(), gpsGetLongitude(), compassGetHeading());
    gpsFeedToMemory(now);
  }
  personalityUpdate(now);

  bool manualFromControl = (motionAuthorityAllowCyd() && espnowIsManualControlActive()) ||
                           (motionAuthorityAllowWeb() && webServerIsManualOverrideActive());
  autonomySetManualOverride(manualFromControl);

  int8_t autoLeft = 0, autoRight = 0;
  autonomyUpdate(now, &autoLeft, &autoRight);

  // Battery polling (rate-limited internally to 10s); refresh TFT when we have a new reading
  if (batteryHandle()) displayUpdateBattery();

  // Dock: ToF (lazy init on first call), sensors
  static bool tofTried = false;
  if (!tofTried) { tofTried = true; tofInit(); }
  tofUpdate(now);
  dockSensorsUpdate();
  dockIrTransmittersUpdate(now);

  bool dockDriving = false;
#if USE_AUTONOMOUS_DOCKING
  // Autonomous docking FSM (IDLE→SEARCH→ALIGN→APPROACH→DOCKED→CHARGING)
  autonomousDockingUpdate(now);
#else
  // Legacy dock homing (RSSI-based)
  dockHomingCheckAutoReturn(now);
  dockHomingUpdate(now);
#endif

  // Unified autonomy brain: one state machine view + safety gate (non-blocking)
  unifiedAutonomyTick(now, manualFromControl);
  const bool safetyBlock = unifiedAutonomySafetyActive();
  if (safetyBlock) {
    motorStop();
    lastCommandMillis = now;
  }

  /* Central motion layer: drive profile, speed caps, turn feel (manual: CYD/WebUI still set profile per command) */
  motionLayerUpdate(now, manualFromControl, safetyBlock);

#if USE_AUTONOMOUS_DOCKING
  if (!safetyBlock && autonomousDockingIsActive()) {
    int16_t left, right;
    if (autonomousDockingGetMotorOutput(&left, &right)) {
      motionLayerApplyMotorTankLimits(&left, &right);
      motorSetLeftRight(left, right);
      lastCommandMillis = now;
      dockDriving = true;
    }
  }
#else
  if (!safetyBlock && dockHomingIsActive()) {
    int16_t left, right;
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

  {
    uint32_t am = 0;
    if (safetyBlock) {
      am |= EVE_ASSIST_MASK_SAFETY;
    }
    if (dockDriving) {
      am |= EVE_ASSIST_MASK_DOCK;
    }
    if (manualFromControl) {
      am |= EVE_ASSIST_MASK_MANUAL;
    }
    eveTargetAssistSetSuppressMask(am);
    eveTargetAssistTick(now);
  }

  // LDR: turn flashlight on when dark
  flashlightHandle();

  // Display redraws (rate-limited internally)
  displayHandle();

  // ESP-NOW telemetry send (10 Hz)
  if ((now - lastTelemSendMs) >= TELEM_SEND_INTERVAL_MS) {
    espnowSendTelemetry();
    lastTelemSendMs = now;
  }

  nodeHealthTick();
  walleEmotionPoseBridgeTick();
  walleEmotionPoseApplyToServosStub();

  // Failsafe: stop drive motors if no command received (includes autonomy + dock keepalive)
  if ((now - lastCommandMillis) > FAILSAFE_TIMEOUT_MS) {
    motorStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = now;
  }

  // L298N: ramp toward targets and drive PWM (after motorSetLeftRight + failsafe motorStop)
  motorHandle();

  delay(1);  /* Yield — prevents TG1WDT_SYS_RST */
  yield();
}
