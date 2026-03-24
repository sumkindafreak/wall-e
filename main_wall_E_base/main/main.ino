// ============================================================
//  Arduino IDE: You opened the correct sketch (folder "main" + main.ino).
//  See ../ARDUINO_IDE_QUICK_START.md — ignore platformio.ini for daily use.
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
#include "ir_beacon_receivers.h"
#include "dock_controller.h"
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
  Serial.println("[Motors] Initialised");

  // Display up early so user gets boot feedback
  displayInit();

  // I2C bus: servos init first (calls Wire.begin)
  servoInit();
  servoNeutral(SERVO_SLOW_SPEED);

  // IMU shares same I2C bus — init and start auto-calibration (~3 s when stationary)
  beginIMU();
  Serial.println("[IMU] Init complete");

  // NEW: Initialize autonomy sensors (TEMPORARILY DISABLED FOR TESTING)
  Serial.println("[Autonomy] SKIPPING initialization for boot test...");
  
  // TODO: Re-enable after confirming boot works
  // if (!sonarInit()) {
  //   Serial.println("[Sonar] ⚠️  Not available - continuing without");
  // } else {
  //   Serial.println("[Sonar] ✓ Ready");
  // }
  // 
  // if (!compassInit()) {
  //   Serial.println("[Compass] ⚠️  Not available - continuing without");
  // } else {
  //   Serial.println("[Compass] ✓ Ready");
  // }
  // 
  // if (!gpsInit()) {
  //   Serial.println("[GPS] ⚠️  Not available - continuing without");
  // } else {
  //   Serial.println("[GPS] ✓ Ready");
  // }
  // 
  // waypointInit();
  // Serial.println("[Waypoint] ✓ Ready");
  // 
  // // NEW: Initialize behavioral brain engines (must be before autonomyInit)
  // Serial.println("[Autonomy] Initializing behavioral engines...");
  // 
  // personalityInit();
  // Serial.println("[Personality] ✓ Ready");
  // 
  // emotionInit();
  // Serial.println("[Emotion] ✓ Ready");
  // 
  // interestInit();
  // Serial.println("[Interest] ✓ Ready");
  // 
  // memoryInit();
  // Serial.println("[Memory] ✓ Ready");
  // 
  // returnHomeInit();
  // Serial.println("[ReturnHome] ✓ Ready");
  // 
  // // Initialize main autonomy engine (uses all above engines)
  // autonomyInit();
  // Serial.println("[Autonomy] ✓ Engine ready");

  // Battery monitor
  batteryInit();
  Serial.println("[Setup] Post-battery");
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

  // IR beacon receivers for dock alignment
  irBeaconInit();
  Serial.println("[Setup] Post-irBeacon");

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

  // Servo velocity interpolation
  servoHandle();

  // Vision behaviour (scan/timeouts when no motion; uses packets from ESP-NOW callback)
  visionBehaviourUpdate(now);

  // IMU: update (runs calibration until done, then provides offset-corrected data)
  updateIMU();
  // Behaviour that uses imuGetData() should check isIMUCalibrated() before use

  // NEW: Update autonomy sensors (TEMPORARILY DISABLED FOR TESTING)
  // TODO: Re-enable after confirming boot works
  // sonarUpdate(now);
  // compassUpdate(now);
  // gpsUpdate(now);
  // 
  // // Update waypoint navigation and feed GPS to memory when we have fix
  // if (gpsHasFix()) {
  //   waypointUpdate(gpsGetLatitude(), gpsGetLongitude(), compassGetHeading());
  //   gpsFeedToMemory(now);
  // }
  // 
  // // Personality update (lightweight)
  // personalityUpdate(now);
  // 
  // // Get autonomy drive commands
  // int8_t autoLeft = 0, autoRight = 0;
  // autonomyUpdate(now, &autoLeft, &autoRight);
  // 
  // // Check if manual control is active (from ESP-NOW)
  // bool manualActive = espnowIsManualControlActive();
  // autonomySetManualOverride(manualActive);
  // 
  // // Apply motor commands (autonomy or manual - manual always wins)
  // if (autonomyIsEnabled() && !manualActive) {
  //   motorSetLeftRight(autoLeft, autoRight);
  //   lastCommandMillis = now;  // Keep alive while autonomy active
  // }
  // // else: motors controlled by ESP-NOW receiver callbacks

  // Battery polling (rate-limited internally to 10s); refresh TFT when we have a new reading
  if (batteryHandle()) displayUpdateBattery();

  // Dock: ToF (lazy init on first call), sensors
  static bool tofTried = false;
  if (!tofTried) { tofTried = true; tofInit(); }
  tofUpdate(now);
  dockSensorsUpdate();
  irBeaconUpdate(now);

#if USE_AUTONOMOUS_DOCKING
  // Autonomous docking FSM (IDLE→SEARCH→ALIGN→APPROACH→DOCKED→CHARGING)
  autonomousDockingUpdate(now);
  if (autonomousDockingIsActive()) {
    int16_t left, right;
    if (autonomousDockingGetMotorOutput(&left, &right)) {
      motorSetLeftRight(left, right);
      lastCommandMillis = now;
    }
  }
#else
  // Legacy dock homing (RSSI-based)
  dockHomingCheckAutoReturn(now);
  dockHomingUpdate(now);
  if (dockHomingIsActive()) {
    int16_t left, right;
    if (dockHomingGetMotorOutput(&left, &right)) {
      motorSetLeftRight(left, right);
      lastCommandMillis = now;
    }
  }
#endif

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

  // Failsafe: stop drive motors if no command received (AUTONOMY TEMPORARILY DISABLED)
  if ((now - lastCommandMillis) > FAILSAFE_TIMEOUT_MS) {
    motorStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = now;
  }

  delay(1);  /* Yield — prevents TG1WDT_SYS_RST */
  yield();
}
