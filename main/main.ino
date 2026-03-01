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

// NEW: Autonomy system includes
#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
#include "waypoint_nav.h"
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

  // NEW: Initialize autonomy sensors
  sonarInit();
  compassInit();
  gpsInit();
  waypointInit();
  autonomyInit();

  // Battery monitor
  batteryInit();
  displayUpdateBattery();

  // WiFi — AP starts immediately
  wifiManagerInit();
  displayUpdateWifi();

  // Web server
  webServerInit();

  // ESP-NOW receiver (CYD controller)
  espnowReceiverInit();

  // LDR + MOSFET flashlight (on when dark)
  flashlightInit();

  lastCommandMillis = millis();
  Serial.println("[WALL-E] Ready");
}

// ============================================================
//  loop()
// ============================================================
unsigned long lastTelemSendMs = 0;
#define TELEM_SEND_INTERVAL_MS 100  // 10 Hz telemetry updates

void loop() {
  unsigned long now = millis();
  
  // WiFi state polling
  WiFiState prevState = wifiGetState();
  wifiManagerHandle();
  if (wifiGetState() != prevState) displayUpdateWifi();

  // Web requests
  webServerHandle();

  // Servo velocity interpolation
  servoHandle();

  // IMU: update (runs calibration until done, then provides offset-corrected data)
  updateIMU();
  // Behaviour that uses imuGetData() should check isIMUCalibrated() before use

  // NEW: Update autonomy sensors
  sonarUpdate(now);
  compassUpdate(now);
  gpsUpdate(now);
  
  // Update waypoint navigation if GPS has fix
  if (gpsHasFix()) {
    waypointUpdate(gpsGetLatitude(), gpsGetLongitude(), compassGetHeading());
  }
  
  // Get autonomy drive commands
  int8_t autoLeft = 0, autoRight = 0;
  autonomyUpdate(now, &autoLeft, &autoRight);
  
  // Check if manual control is active (from ESP-NOW)
  bool manualActive = espnowIsManualControlActive();
  autonomySetManualOverride(manualActive);
  
  // Apply motor commands (autonomy or manual - manual always wins)
  if (autonomyIsEnabled() && !manualActive) {
    motorSet(autoLeft, autoRight);
    lastCommandMillis = now;  // Keep alive while autonomy active
  }
  // else: motors controlled by ESP-NOW receiver callbacks

  // Battery polling (rate-limited internally to 10s); refresh TFT when we have a new reading
  if (batteryHandle()) displayUpdateBattery();

  // LDR: turn flashlight on when dark
  flashlightHandle();

  // Display redraws (rate-limited internally)
  displayHandle();

  // ESP-NOW telemetry send (10 Hz)
  if ((now - lastTelemSendMs) >= TELEM_SEND_INTERVAL_MS) {
    espnowSendTelemetry();
    lastTelemSendMs = now;
  }

  // Failsafe: stop drive motors if no command received AND autonomy disabled
  if (!autonomyIsEnabled() && (now - lastCommandMillis) > FAILSAFE_TIMEOUT_MS) {
    motorStop();
    displaySetCommand(CMD_IDLE);
    lastCommandMillis = now;
  }
}
