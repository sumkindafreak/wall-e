// ============================================================
//  WALL-E Master Controller — CYD Direct-Draw UI
//  TFT_eSPI + XPT2046, no LVGL
//  State machine, animations, audio, zero-flicker
//  + Physical Joysticks (ADS1115) + Buttons (SX1509)
// ============================================================
//
// REQUIRED: TFT_eSPI (Setup_CYD_ESP32_2432S028R), XPT2046_Touchscreen
//           Adafruit_ADS1X15, SparkFun SX1509
//
// ============================================================

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_task_wdt.h>

#include "protocol.h"
#include "ui_draw.h"
#include "ui_state.h"
#include "touch_input.h"
#include "animation_system.h"
#include "audio_system.h"
#include "packet_control.h"
#include "espnow_control.h"

// Physical I2C input
#include "i2c_devices.h"
#include "ads1115_input.h"
#include "sx1509_input.h"

// Motion engine
#include "motion_engine.h"

// Profile system
#include "profiles.h"

#define TFT_BL 21
#define WDT_TIMEOUT_SEC 3

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[WALL-E Master] CYD Command Console");

  // Configure watchdog timer (ESP-IDF 5.x API)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SEC * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  Serial.printf("[WDT] Enabled (%ds timeout)\n", WDT_TIMEOUT_SEC);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);

  uiDrawInit(&tft);
  uiStateInit();
  touchInit();
  animInit();
  audioInit();
  packetInit();
  
  // Initialize motion engine
  motionInit();
  
  // Initialize profile system (loads from Preferences)
  profileInit();

  // Initialize I2C hardware
  i2cInit();
  i2cScan();
  
  bool ads_ok = ads1115Init();
  bool sx_ok = sx1509Init();
  
  if (!ads_ok || !sx_ok) {
    Serial.println(F("[WARN] Some I2C devices missing - check wiring"));
  }

  g_needStaticRedraw = true;

  Serial.println("[WALL-E Master] Ready");
}

void loop() {
  unsigned long now = millis();
  
  // Feed watchdog to prevent reset
  esp_task_wdt_reset();

  // Update I2C inputs (joysticks + buttons) with error handling
  static unsigned long lastI2CError = 0;
  static int i2cErrorCount = 0;
  
  try {
    ads1115Update();
    sx1509Update();
  } catch (...) {
    i2cErrorCount++;
    if (now - lastI2CError > 5000) {
      Serial.printf("[I2C] ⚠️  Error count: %d\n", i2cErrorCount);
      lastI2CError = now;
    }
  }

  int pageInt = (int)g_currentPage;
  TouchZone zone = touchUpdate(pageInt);

  DriveState ds;
  
  // Priority: Physical joystick overrides touch
  const JoystickState& joy = getJoystickState();
  bool joystickActive = joy.active[JOY2_X] || joy.active[JOY2_Y];
  
  // HEAD CONTROL (Joy1) - velocity-based - ALWAYS ACTIVE (no deadman required)
  motionSetHeadPanVelocity(joy.processed[JOY1_X]);   // Left stick X → Pan
  motionSetHeadTiltVelocity(joy.processed[JOY1_Y]);  // Left stick Y → Tilt
  
  // DRIVE CONTROL (Joy2) - tank drive
  if (joystickActive) {
    joystickToDriveState(&ds);
    
    // Log drive state when joystick is active
    static unsigned long lastJoyLog = 0;
    if (now - lastJoyLog > 500) {  // Every 500ms
      Serial.printf("[Drive] Joystick: L=%d%% R=%d%% (Deadman: %s)\n", 
        ds.leftSpeed, ds.rightSpeed, isDeadmanButtonHeld() ? "HELD" : "NOT HELD");
      lastJoyLog = now;
    }
  } else {
    touchGetDriveState(&ds);
  }

  // CRITICAL: DEADMAN BUTTON CHECK - Must be held to move TRACKS ONLY!
  // Servos, head, animations work WITHOUT deadman!
  if (!isDeadmanButtonHeld()) {
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
    static unsigned long lastDeadmanWarning = 0;
    if (now - lastDeadmanWarning > 2000 && joystickActive) {
      Serial.println("[DEADMAN] Button not held - TRACKS disabled! (servos/head still work)");
      lastDeadmanWarning = now;
    }
  }

  // Safety lock: no touch for 200ms → STOP (only if joystick also inactive)
  if (!joystickActive && (now - touchLastActivityMs() > PACKET_SAFETY_TIMEOUT_MS)) {
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
  }

  // Zone actions
  if (zone == TOUCH_ZONE_ESTOP) {
    g_estop = true;
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
    g_controlAuthority = CTRL_SAFETY;
    playUISound(SOUND_ESTOP);
  }
  if (zone == TOUCH_ZONE_NAV_BEHAV) {
    g_currentPage = PAGE_BEHAVIOUR;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_SYSTEM) {
    g_currentPage = PAGE_SYSTEM;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_BACK) {
    g_currentPage = PAGE_DRIVE;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_PROFILE) {
    g_currentPage = PAGE_PROFILE;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_SERVO_TEST) {
    g_currentPage = PAGE_SERVO_TEST;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_QUICK_ACTION) {
    g_overlayVisible = !g_overlayVisible;
    g_needStaticRedraw = true;
    playUISound(SOUND_MODE_CHANGE);
  }
  if (zone == TOUCH_ZONE_ADVANCED) {
    g_advancedMode = !g_advancedMode;
    playUISound(SOUND_CONFIRM);
  }
  // NEW: Autonomy navigation
  if (zone == TOUCH_ZONE_NAV_AUTONOMY) {
    g_currentPage = PAGE_AUTONOMY;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.println(F("[Nav] Navigated to Autonomy page"));
  }
  if (zone == TOUCH_ZONE_AUTONOMY_TOGGLE) {
    // TODO: Send command to Base to toggle autonomy
    playUISound(SOUND_MODE_CHANGE);
    Serial.println(F("[Autonomy] Toggle requested (command not implemented yet)"));
    // Future: send ESP-NOW command to Base
  }
  if (zone == TOUCH_ZONE_PROFILE_0) {
    profileSet(0);
    playUISound(SOUND_MODE_CHANGE);
    g_needStaticRedraw = true;  // Force UI redraw
    Serial.println(F("[Profile] Switched to Kid mode"));
  }
  if (zone == TOUCH_ZONE_PROFILE_1) {
    profileSet(1);
    playUISound(SOUND_MODE_CHANGE);
    g_needStaticRedraw = true;  // Force UI redraw
    Serial.println(F("[Profile] Switched to Demo mode"));
  }
  if (zone == TOUCH_ZONE_PROFILE_2) {
    profileSet(2);
    playUISound(SOUND_MODE_CHANGE);
    g_needStaticRedraw = true;  // Force UI redraw
    Serial.println(F("[Profile] Switched to Advanced mode"));
  }
  if (zone == TOUCH_ZONE_PROFILE_EDIT_0 || zone == TOUCH_ZONE_PROFILE_EDIT_1 || zone == TOUCH_ZONE_PROFILE_EDIT_2) {
    // Set profile first before entering editor
    uint8_t profileId = zone - TOUCH_ZONE_PROFILE_EDIT_0;
    profileSet(profileId);
    g_currentPage = PAGE_SERVO_EDITOR;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.printf("[ServoEdit] Editing %s profile\n", profileGet()->name);
  }
  if (zone == TOUCH_ZONE_SERVO_SAVE) {
    profileSave();
    playUISound(SOUND_CONFIRM);
    Serial.println(F("[ServoEdit] Settings saved to flash"));
  }
  if (zone == TOUCH_ZONE_SERVO_RESET) {
    profileApply();  // Re-apply current profile (resets any unsaved changes)
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.println(F("[ServoEdit] Settings reset"));
  }
  // Slider drag handling
  if (zone >= TOUCH_ZONE_SLIDER_0 && zone <= TOUCH_ZONE_SLIDER_4) {
    // Touch on a slider - handle drag
    XPT2046_Touchscreen* ts = touchGetTs();
    if (ts->touched()) {
      TS_Point p = ts->getPoint();
      int screenX = map(p.x, 200, 3700, 0, 319);
      screenX = constrain(screenX, 0, 319);
      
      // Calculate slider value from X position (120-300 = slider area)
      const int sliderX = 120;
      const int sliderW = 180;
      float sliderValue = (float)(screenX - sliderX) / (float)sliderW;
      sliderValue = constrain(sliderValue, 0.0f, 1.0f);
      
      uint8_t sliderIndex = zone - TOUCH_ZONE_SLIDER_0;
      
      switch (sliderIndex) {
        case 0:  // Head sensitivity (0.5-2.0 range)
          {
            float newVal = 0.5f + (sliderValue * 1.5f);
            Profile* p = profileGet();
            profileAdjustHeadSensitivity(newVal - p->headSensitivity);
          }
          break;
        case 1:  // Servo speed (0-1.0)
          profileAdjustServoSpeed(sliderValue - profileGet()->servoSpeedLimit);
          break;
        case 2:  // Deadzone (0-0.5)
          profileAdjustDeadzone((sliderValue * 0.5f) - profileGet()->joystickDeadzone);
          break;
        case 3:  // Expo (0-1.0)
          profileAdjustExpo(sliderValue - profileGet()->joystickExpo);
          break;
        case 4:  // Max speed (0-1.0)
          profileAdjustMaxSpeed(sliderValue - profileGet()->joystickMaxSpeed);
          break;
      }
      
      g_needStaticRedraw = true;  // Redraw to show new slider position
    }
  }
  // Servo test page - individual servo sliders
  if (zone >= TOUCH_ZONE_SERVO_SLIDER_0 && zone <= TOUCH_ZONE_SERVO_SLIDER_9) {
    XPT2046_Touchscreen* ts = touchGetTs();
    if (ts->touched()) {
      TS_Point p = ts->getPoint();
      int screenX = map(p.x, 200, 3700, 0, 319);
      screenX = constrain(screenX, 0, 319);
      
      uint8_t servoIndex = zone - TOUCH_ZONE_SERVO_SLIDER_0;
      
      // Calculate servo position from X (slider is 90px wide, represents 0-180°)
      int col = (servoIndex < 5) ? 10 : 165;
      int sliderX = col + 55;
      int sliderW = 90;
      
      float sliderValue = (float)(screenX - sliderX) / (float)sliderW;
      sliderValue = constrain(sliderValue, 0.0f, 1.0f);
      uint8_t degrees = (uint8_t)(sliderValue * 180.0f);
      
      motionSetServoDirect(servoIndex, degrees);
      g_needStaticRedraw = true;
    }
  }
  if (zone == TOUCH_ZONE_SERVO_SAVE_NEUTRAL) {
    // Get current servo positions and save as neutral
    uint8_t currentPositions[SERVO_COUNT];
    motionGetServoTargets(currentPositions);
    profileSaveNeutralPositions(currentPositions);
    playUISound(SOUND_CONFIRM);
    Serial.println(F("[ServoTest] Current positions saved as neutral"));
  }
  if (zone == TOUCH_ZONE_SERVO_NEUTRAL) {
    motionSetAllNeutral();
    g_needStaticRedraw = true;
    playUISound(SOUND_CONFIRM);
  }
  if (zone == TOUCH_ZONE_SERVO_TEST1) {
    motionTestPose1();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SERVO_TEST2) {
    motionTestPose2();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  // Behaviour page - mood buttons trigger animations
  if (zone >= TOUCH_ZONE_ANIM_0 && zone <= TOUCH_ZONE_ANIM_5) {
    uint8_t animId = zone - TOUCH_ZONE_ANIM_0;
    motionTriggerAnimation(animId);
    playUISound(SOUND_CLICK);
    Serial.printf("[Behaviour] Animation %d triggered\n", animId);
  }
  
  // Behaviour page - LONG PRESS to toggle favorite
  if (zone >= (TOUCH_ZONE_ANIM_0 + 100) && zone <= (TOUCH_ZONE_ANIM_5 + 100)) {
    uint8_t animId = zone - (TOUCH_ZONE_ANIM_0 + 100);
    profileToggleFavoriteAnimation(animId);
    playUISound(SOUND_CONFIRM);
    g_needStaticRedraw = true;  // Redraw to show updated favorites
    Serial.printf("[Behaviour] Toggled favorite for animation %d\n", animId);
  }
  
  // Legacy mood zones (from main screen) - now use profile favorites!
  if (zone >= TOUCH_ZONE_MOOD_CURIOUS && zone <= TOUCH_ZONE_MOOD_EXCITED) {
    Profile* p = profileGet();
    uint8_t moodIndex = zone - TOUCH_ZONE_MOOD_CURIOUS;
    uint8_t animId = p->favoriteAnimations[moodIndex];
    if (animId < 6) {  // Valid animation
      motionTriggerAnimation(animId);
      playUISound(SOUND_CLICK);
      Serial.printf("[MainScreen] Favorite %d (anim %d) triggered\n", moodIndex, animId);
    }
  }

  // Button actions (NO DEADMAN REQUIRED - buttons work independently!)
  const ButtonState& btn = getButtonState();
  
  // Both joystick buttons = E-STOP (stops everything including servos)
  if (isBothJoystickButtonsHeld()) {
    g_estop = true;
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
    g_controlAuthority = CTRL_SAFETY;
    motionEmergencyStop();  // Stop all servos too
  }
  
  // Individual joystick button = Eyebrow raise (NO deadman required)
  if (btn.pressed[BTN_JOY1] && !isBothJoystickButtonsHeld()) {
    motionTriggerAnimation(3);  // Right eyebrow
    Serial.println(F("[Action] Right eyebrow raise (no deadman needed)"));
  }
  
  if (btn.pressed[BTN_JOY2] && !isBothJoystickButtonsHeld()) {
    motionTriggerAnimation(4);  // Left eyebrow
    Serial.println(F("[Action] Left eyebrow raise (no deadman needed)"));
  }
  
  // Extra buttons (2-7) use profile mappings (NO deadman required)
  for (int i = 2; i < 8; i++) {
    if (i == 6) continue;  // Skip deadman button
    if (btn.pressed[i]) {
      profileHandleButtonAction(i - 2);  // Map to profile button 0-5
      Serial.printf("[Action] Button %d pressed (no deadman needed)\n", i);
    }
  }
  
  // Update motion engine
  motionUpdate(now);

  packetUpdate(now, &ds, g_estop);
  audioUpdate(now);

  // Static redraw on page/overlay change
  if (g_needStaticRedraw) {
    uiDrawCurrentPage();
    if (g_overlayVisible) uiDrawQuickActionOverlay();
    g_needStaticRedraw = false;
  }

  // Dynamic update
  TelemetryPacket telem;
  packetGetTelemetry(&telem);
  bool connected = packetTelemetryValid();

  TelemetryStripData strip = {};
  strip.batteryV = connected ? telem.batteryVoltage : 0.0f;
  strip.batteryPct = (strip.batteryV > 0) ? (int)((strip.batteryV - 3.0f) / 1.2f * 100.0f) : 0;
  if (strip.batteryPct < 0) strip.batteryPct = 0;
  if (strip.batteryPct > 100) strip.batteryPct = 100;
  strip.currentA = connected ? telem.currentDraw : 0.0f;
  strip.tempC = connected ? telem.temperature : 0.0f;
  strip.packetRate = espnowGetPacketRate();
  strip.rssi = 0;
  strip.connected = connected;
  strip.modeStr = g_estop ? "E-STOP" : (g_controlAuthority == CTRL_AUTONOMOUS ? "AUTO" :
                  g_controlAuthority == CTRL_SUPERVISED ? "SUPV" : "MANUAL");

  // Debug telemetry every 5 seconds
  static unsigned long lastTelemDebug = 0;
  if (now - lastTelemDebug > 5000) {
    Serial.printf("[Telemetry] Connected=%d BattV=%.2f (%.0f%%) Temp=%.1f°C Current=%.2fA\n",
      connected, strip.batteryV, (float)strip.batteryPct, strip.tempC, strip.currentA);
    lastTelemDebug = now;
  }

  int lx, ly;
  touchGetJoystickDots(&lx, &ly);

  uiDrawUpdateDynamic(&strip, &ds, lx, ly);
  
  // Draw physical joystick indicators (only on Drive page)
  if (g_currentPage == PAGE_DRIVE) {
    const JoystickState& joyVis = getJoystickState();
    uiDrawPhysicalJoystickIndicators(
      joyVis.processed[JOY1_X], joyVis.processed[JOY1_Y],
      joyVis.processed[JOY2_X], joyVis.processed[JOY2_Y]
    );
  }

  // Animated eye (Drive or Behaviour page)
  animUpdate(now);
  if (g_currentPage == PAGE_DRIVE || g_currentPage == PAGE_BEHAVIOUR) {
    animDrawEye(telem.moodState, g_estop, false);
  }

  if (g_advancedMode) {
    uiDrawAdvancedModeOverlay();
  }

  delay(5);
}
