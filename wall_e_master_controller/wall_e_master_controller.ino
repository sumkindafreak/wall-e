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
#include <esp_err.h>
#include <esp_task_wdt.h>
#include <math.h>

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

// Emotion engine (poses + triggers; no servo output until wired)
#include "emotion_engine.h"

// Profile system
#include "profiles.h"
#include "animation_data.h"

#include "command_queue.h"
#include "cyd_laser_ui.h"
#include "system_status.h"
#include "touch_handling.h"
#include "ui_rendering.h"
#include "command_input.h"

// SD card (SPI: CS 5, MOSI 23, MISO 19, SCK 18) + macro persistence
#include "sd_manager.h"
#include "macro_system.h"
#include "dev_console.h"
#include "ui_sd_explorer.h"
#include "sd_browser.h"

#define TFT_BL 21
/* Full-screen redraw, SD, anim can exceed a few seconds on SPI; 3s WDT caused resets. */
#define WDT_TIMEOUT_SEC 10

TFT_eSPI tft = TFT_eSPI();

/** Only call esp_task_wdt_reset when loop task is actually on the TWDT (avoids IDF error spam). */
static bool s_taskWdtFeedOk = false;

static void walleTaskWdtInit(void) {
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT_SEC * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_err_t e = esp_task_wdt_init(&wdt_config);
  if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
    Serial.printf("[WDT] init failed (%d) — TWDT disabled, no feed\n", (int)e);
    s_taskWdtFeedOk = false;
    return;
  }
  e = esp_task_wdt_add(NULL);
  if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
    Serial.printf("[WDT] esp_task_wdt_add failed (%d) — not feeding\n", (int)e);
    s_taskWdtFeedOk = false;
    return;
  }
  e = esp_task_wdt_reset();
  s_taskWdtFeedOk = (e == ESP_OK);
  if (s_taskWdtFeedOk) {
    Serial.printf("[WDT] loop task OK (%ds)\n", WDT_TIMEOUT_SEC);
  } else {
    Serial.printf("[WDT] reset test failed (%d) — disabling feed\n", (int)e);
  }
}

static inline void walleTaskWdtFeed(void) {
  if (s_taskWdtFeedOk) {
    (void)esp_task_wdt_reset();
  }
}

// USER-CUSTOMIZABLE: Button-press toast durations (ms).
static const uint32_t BTN_TOAST_OK_MS = 1400u;
static const uint32_t BTN_TOAST_WARN_MS = 1200u;
static const uint32_t BTN_HIGHLIGHT_MS = 220u;
static const uint32_t TOUCH_TILE_HIGHLIGHT_MS = 240u;

static void triggerButtonMappedAnimation(uint8_t slot, const char* buttonName) {
  Profile* p = profileGet();
  if (!p || slot >= 6) return;
  uint8_t animId = p->favoriteAnimations[slot];
  if (animId >= ANIMATION_COUNT) {
    char msg[48];
    snprintf(msg, sizeof(msg), "%s: no animation assigned", buttonName ? buttonName : "Button");
    uiShowToast(msg, BTN_TOAST_WARN_MS);
    playUISound(SOUND_ERROR);
    return;
  }
  motionTriggerAnimation(animId);
  uiSetFavoriteButtonHighlight(slot, BTN_HIGHLIGHT_MS);
  const char* nm = animationLibrary[animId].name;
  char msg[56];
  snprintf(msg, sizeof(msg), "%s -> %s (#%u)", buttonName ? buttonName : "Button",
           nm ? nm : "Anim", (unsigned)animId);
  uiShowToast(msg, BTN_TOAST_OK_MS);
  Serial.printf("[BtnMap] %s -> animation %u (%s)\n", buttonName ? buttonName : "Button",
                (unsigned)animId, nm ? nm : "Anim");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[WALL-E Master] CYD Command Console");

  walleTaskWdtInit();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);

  // SD card: after TFT init (separate SPI bus). Macros + logs use SD when present.
  if (sdInit()) {
    sdLogInit();
    sdLog("WALL-E CYD Master boot");
    sdLogFlush();
  } else {
    sdLogInit();
  }
  macroInit();

  uiDrawInit(&tft);
  uiRenderingInit(&tft);
  uiStateInit();
  touchHandlingInit();
  commandQueueInit();
  cydLaserUiInit();
  systemStatusInit();
  commandInputInit();
  animInit();
  audioInit();
  packetInit();
  
  // Initialize motion engine
  motionInit();
  emotionInit();

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
  const uint32_t loopStartUs = micros();
  unsigned long now = millis();
  
  walleTaskWdtFeed();

  ads1115Update();
  sx1509Update();

  bool screenTouch = false;
  uint16_t sx = 0, sy = 0;
  {
    XPT2046_Touchscreen* ts = touchGetTs();
    /* Match touch_input: IRQ low + read + min pressure (avoids extra SPI, stuck reads). */
    if (ts->tirqTouched() && ts->touched()) {
      TS_Point tp = ts->getPoint();
      if (tp.z > TOUCH_MIN_PRESSURE) {
        sx = (uint16_t)constrain(map(tp.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, 319), 0, 319);
        sy = (uint16_t)constrain(map(tp.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 239), 0, 239);
        screenTouch = true;
      }
    }
  }
  devConsoleProcessTouch(sx, sy, screenTouch);

  /* Right ~48px of banner only — avoids dev-console unlock hold (top-right x≈250–271) */
  const bool bannerTapZone =
      screenTouch && sy < (uint16_t)uiBannerTotalHeight() && sx >= (uint16_t)(SCREEN_W - 48);
  static bool s_prevBannerTapZone = false;
  if (!devConsoleIsUnlocked() && bannerTapZone && !s_prevBannerTapZone) {
    g_topBannerCollapsed = !g_topBannerCollapsed;
    uiBannerInvalidateTelemetryCache();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  s_prevBannerTapZone = bannerTapZone;

  if (devConsoleIsUnlocked()) {
    commandQueueDrainAll();
    DriveState dsZ = {};
    motionSetHeadPanVelocity(0);
    motionSetHeadTiltVelocity(0);

    macroCheckJoystickOverride(false);
    float pbL, pbR, pbSv[9];
    if (macroGetPlaybackData(&pbL, &pbR, pbSv)) {
      dsZ.leftSpeed = (int8_t)constrain((int)lroundf(pbL), -100, 100);
      dsZ.rightSpeed = (int8_t)constrain((int)lroundf(pbR), -100, 100);
      for (int i = 0; i < 9; i++) {
        uint8_t deg = (uint8_t)constrain((int)lroundf(pbSv[i] * 180.0f / 100.0f), 0, 180);
        motionSetServoDirect(i, deg);
      }
    }
    if (!isDeadmanButtonHeld()) {
      dsZ.leftSpeed = 0;
      dsZ.rightSpeed = 0;
    }

    motionUpdate(now);

    uint8_t macTgt[10];
    motionGetServoTargets(macTgt);
    float macSv[9];
    for (int i = 0; i < 9; i++) {
      macSv[i] = macTgt[i] * (100.0f / 180.0f);
    }
    macroSetCurrentData((float)dsZ.leftSpeed, (float)dsZ.rightSpeed, macSv);
    macroUpdate(now);

    uint8_t st[10];
    motionGetServoTargets(st);
    float sv[9];
    for (int i = 0; i < 9; i++) {
      sv[i] = st[i] * (100.0f / 180.0f);
    }
    devConsoleFeedServoData(sv);
    TelemetryPacket tm;
    packetGetTelemetry(&tm);
    devConsoleFeedSensorData(tm.sonarDistanceCm, tm.compassHeading, tm.gpsValid != 0);
    devConsoleFeedPacketTiming(PACKET_SEND_INTERVAL_MS * 1000u, (uint32_t)(micros() - loopStartUs));
    devConsoleUpdate(now);
    devConsoleDraw(&tft);
    walleTaskWdtFeed();
    packetUpdate(now, &dsZ, g_estop);
    systemStatusTick(now);
    audioUpdate(now);
    sdUpdate();
    walleTaskWdtFeed();
    delay(1);
    return;
  }

  commandQueueDrainAll();

  int pageInt = (int)g_currentPage;
  TouchZone zone;
  if (bannerTapZone) {
    zone = TOUCH_ZONE_NONE;
  } else {
    zone = touchHandlingUpdate(pageInt);
  }

  static TouchZone s_prevTouchZone = TOUCH_ZONE_NONE;
  static uint32_t s_lastLaserToggleMs = 0;
  if (zone == TOUCH_ZONE_LASER_TOGGLE && s_prevTouchZone != TOUCH_ZONE_LASER_TOGGLE) {
    if ((uint32_t)(now - s_lastLaserToggleMs) >= 400u) {
      cydLaserUiToggleArmed();
      s_lastLaserToggleMs = now;
      playUISound(SOUND_CLICK);
      Serial.println(F("[UI] Laser beam toggle"));
    }
  }
  s_prevTouchZone = zone;

  DriveState ds;
  
  // Priority: Physical joystick overrides touch
  const JoystickState& joy = getJoystickState();
  bool joystickActive = joy.active[JOY2_X] || joy.active[JOY2_Y];

  motionSetHeadPanVelocity(joy.processed[JOY1_X]);
  motionSetHeadTiltVelocity(joy.processed[JOY1_Y]);
  
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

  commandQueueApplyDriveOverride(&ds, joystickActive);

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

  // Zone actions — E-STOP: tap to trigger, tap again to clear
  if (zone == TOUCH_ZONE_ESTOP) {
    g_estop = !g_estop;
    if (g_estop) {
      ds.leftSpeed = 0;
      ds.rightSpeed = 0;
      g_controlAuthority = CTRL_SAFETY;
      playUISound(SOUND_ESTOP);
    } else {
      g_controlAuthority = CTRL_LOCAL;
      playUISound(SOUND_CLICK);
    }
  }
  if (zone == TOUCH_ZONE_NAV_BEHAV) {
    g_currentPage = PAGE_BEHAVIOUR;
    g_behaviourAnimPage = 0;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_BEHAV_PAGE) {
    g_behaviourAnimPage = (uint8_t)((g_behaviourAnimPage + 1u) % 4u);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_SYSTEM) {
    g_currentPage = PAGE_SYSTEM;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_BACK) {
    if (g_currentPage == PAGE_HELP && g_helpSection > 0) {
      g_helpSection = 0;
    } else if (g_currentPage == PAGE_HELP) {
      g_currentPage = PAGE_SYSTEM;
      g_helpSection = 0;
    } else {
      g_currentPage = PAGE_DRIVE;
    }
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
  if (zone == TOUCH_ZONE_SYS_MOTION_POLICY) {
    uint8_t cur = g_motionPolicyFromBrain;
    if (cur > 2) cur = 0;
    uint8_t next = (uint8_t)((cur + 1) % 3);
    packetSetMotionPolicy(next);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.printf("[Nav] Motion policy -> %u (any/cyd/web)\n", (unsigned)next);
  }
  if (zone == TOUCH_ZONE_DOCK_GO) {
    packetSetPendingAction(ACTION_DOCK_GO);
    playUISound(SOUND_MODE_CHANGE);
    Serial.println(F("[Dock] Go to dock"));
  }
  if (zone == TOUCH_ZONE_DOCK_CANCEL) {
    packetSetPendingAction(ACTION_DOCK_CANCEL);
    playUISound(SOUND_CLICK);
    Serial.println(F("[Dock] Cancel"));
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
    g_autonomyUiTab = 0;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.println(F("[Nav] Navigated to Autonomy page"));
  }
  if (zone == TOUCH_ZONE_NAV_HELP) {
    g_currentPage = PAGE_HELP;
    g_helpSection = 0;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.println(F("[Nav] Help"));
  }
  if (zone >= TOUCH_ZONE_HELP_TOPIC_0 && zone <= TOUCH_ZONE_HELP_TOPIC_3) {
    g_helpSection = (uint8_t)(1 + (zone - TOUCH_ZONE_HELP_TOPIC_0));
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.printf("[Nav] Help topic %u\n", (unsigned)g_helpSection);
  }
  if (zone == TOUCH_ZONE_NAV_SD) {
    uiSdExplorerClosePreview();
    uiSdExplorerCloseConfirm();
    uiSdExplorerCloseRename();
    sdBrowserOnEnterPage();
    g_currentPage = PAGE_SD_EXPLORER;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
    Serial.println(F("[Nav] SD explorer"));
  }
  if (zone >= TOUCH_ZONE_MC_SC_0 && zone <= TOUCH_ZONE_MC_SC_5) {
    static const char* const kMemCorePath[] = {
      SD_MEMORY_DIR,
      SD_LOGS,
      SD_CONFIG_DIR,
      SD_MISSIONS_DIR,
      SD_DIAG_DIR,
      SD_EVENTS_DIR
    };
    int si = (int)(zone - TOUCH_ZONE_MC_SC_0);
    if (sdBrowserNavigateToAbsolute(kMemCorePath[si])) {
      Serial.printf("[MemCore] Shortcut -> %s\n", kMemCorePath[si]);
    } else {
      Serial.printf("[MemCore] Shortcut failed: %s\n", kMemCorePath[si]);
    }
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone >= TOUCH_ZONE_SD_LIST_0 && zone <= TOUCH_ZONE_SD_LIST_3) {
    int row = (int)(zone - TOUCH_ZONE_SD_LIST_0);
    uint16_t idx = (uint16_t)(sdBrowserGetScroll() + (uint16_t)row);
    sdBrowserSetSelected((int16_t)idx);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_UP) {
    sdBrowserGoUp();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_OPEN) {
    if (sdBrowserGetSelected() >= 0) {
      const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sdBrowserGetSelected());
      if (e && e->isDir) {
        sdBrowserEnterSelected();
      } else if (e) {
        uiSdExplorerTryOpenPreview();
      }
    }
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_REFRESH) {
    sdBrowserRefresh();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_PG_PREV) {
    uint16_t sc = sdBrowserGetScroll();
    if (sc >= SD_BROWSER_VISIBLE_ROWS) {
      sdBrowserSetScroll((uint16_t)(sc - SD_BROWSER_VISIBLE_ROWS));
    } else {
      sdBrowserSetScroll(0);
    }
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_PG_NEXT) {
    uint16_t sc = sdBrowserGetScroll();
    uint16_t cnt = sdBrowserGetEntryCount();
    uint16_t maxScr = (cnt > SD_BROWSER_VISIBLE_ROWS) ? (uint16_t)(cnt - SD_BROWSER_VISIBLE_ROWS) : 0;
    uint16_t nsc = (uint16_t)(sc + SD_BROWSER_VISIBLE_ROWS);
    if (nsc > maxScr) nsc = maxScr;
    sdBrowserSetScroll(nsc);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_DELETE) {
    uiSdExplorerRequestDeleteConfirm();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_CONFIRM_YES) {
    if (sdBrowserDeleteSelected()) {
      uiSdExplorerCloseConfirm();
    }
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_CONFIRM_NO) {
    uiSdExplorerCloseConfirm();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME) {
    uiSdExplorerRequestRename();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_CH_DEC) {
    uiSdExplorerRenameChrDec();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_CH_INC) {
    uiSdExplorerRenameChrInc();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_BKSP) {
    uiSdExplorerRenameBksp();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_CUR_L) {
    uiSdExplorerRenameCurLeft();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_CUR_R) {
    uiSdExplorerRenameCurRight();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_CANCEL) {
    uiSdExplorerCloseRename();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_SD_RENAME_OK) {
    uiSdExplorerRenameApplyOk();
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_TAB_LIVE) {
    g_autonomyUiTab = 0;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_TAB_TUNE) {
    g_autonomyUiTab = 1;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_ARM) {
    g_remoteAutonomyArm = !g_remoteAutonomyArm;
    g_needStaticRedraw = true;
    playUISound(SOUND_MODE_CHANGE);
    Serial.printf("[Autonomy] ARM TX %s\n", g_remoteAutonomyArm ? "ON" : "off");
  }
  if (zone == TOUCH_ZONE_AUTONOMY_M_CLOSE) {
    if (g_auCloseCm > 10) g_auCloseCm -= 5;
    if (g_auInterestCm < (uint8_t)(g_auCloseCm + 5)) g_auInterestCm = g_auCloseCm + 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_DETECT_CLOSE_CM, g_auCloseCm);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_P_CLOSE) {
    if (g_auCloseCm < 150) g_auCloseCm += 5;
    if (g_auInterestCm < (uint8_t)(g_auCloseCm + 5)) g_auInterestCm = g_auCloseCm + 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_DETECT_CLOSE_CM, g_auCloseCm);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_M_INT) {
    uint8_t lo = g_auCloseCm + 5;
    if (g_auInterestCm > lo) g_auInterestCm -= 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_DETECT_INTEREST_CM, g_auInterestCm);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_P_INT) {
    if (g_auInterestCm < 200) g_auInterestCm += 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_DETECT_INTEREST_CM, g_auInterestCm);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_M_CUR) {
    if (g_auCuriosityPct > 0) g_auCuriosityPct -= 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_CURIOSITY, g_auCuriosityPct);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_P_CUR) {
    if (g_auCuriosityPct < 100) g_auCuriosityPct += 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_CURIOSITY, g_auCuriosityPct);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_M_BRV) {
    if (g_auBraveryPct > 0) g_auBraveryPct -= 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_BRAVERY, g_auBraveryPct);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_P_BRV) {
    if (g_auBraveryPct < 100) g_auBraveryPct += 5;
    packetSetAutonomyConfig(AUTONOMY_KEY_BRAVERY, g_auBraveryPct);
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_WAYPOINT) {
    g_auWaypointFollow = !g_auWaypointFollow;
    packetSetAutonomyConfig(AUTONOMY_KEY_WAYPOINT_MODE, g_auWaypointFollow ? 1u : 0u);
    g_needStaticRedraw = true;
    playUISound(SOUND_MODE_CHANGE);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_PRESET_0) {
    packetSetAutonomyConfig(AUTONOMY_KEY_PRESET, 0);
    g_needStaticRedraw = true;
    playUISound(SOUND_CONFIRM);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_PRESET_1) {
    packetSetAutonomyConfig(AUTONOMY_KEY_PRESET, 1);
    g_needStaticRedraw = true;
    playUISound(SOUND_CONFIRM);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_PRESET_2) {
    packetSetAutonomyConfig(AUTONOMY_KEY_PRESET, 2);
    g_needStaticRedraw = true;
    playUISound(SOUND_CONFIRM);
  }
  if (zone == TOUCH_ZONE_AUTONOMY_PRESET_3) {
    packetSetAutonomyConfig(AUTONOMY_KEY_PRESET, 3);
    g_needStaticRedraw = true;
    playUISound(SOUND_CONFIRM);
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
  if (zone >= TOUCH_ZONE_ANIM_0 && zone <= TOUCH_ZONE_ANIM_23) {
    uint8_t animId = zone - TOUCH_ZONE_ANIM_0;
    uiSetAnimationTileHighlight(animId, TOUCH_TILE_HIGHLIGHT_MS);
    motionTriggerAnimation(animId);
    playUISound(SOUND_CLICK);
    Serial.printf("[Behaviour] Animation %d triggered\n", animId);
  }

  // Behaviour page - LONG PRESS to toggle favorite
  if (zone >= (TOUCH_ZONE_ANIM_0 + 100) && zone <= (TOUCH_ZONE_ANIM_23 + 100)) {
    uint8_t animId = zone - (TOUCH_ZONE_ANIM_0 + 100);
    profileToggleFavoriteAnimation(animId);
    {
      const char* nm = (animId < ANIMATION_COUNT) ? animationLibrary[animId].name : "Anim";
      char msg[56];
      snprintf(msg, sizeof(msg), "Fav toggled: %s (#%u)", nm ? nm : "Anim", (unsigned)animId);
      uiShowToast(msg, 1200);
    }
    playUISound(SOUND_CONFIRM);
    g_needStaticRedraw = true;
    Serial.printf("[Behaviour] Toggled favorite for animation %d\n", animId);
  }

  // Legacy mood zones (from main screen) - use profile favorites
  if (zone >= TOUCH_ZONE_MOOD_CURIOUS && zone <= TOUCH_ZONE_MOOD_FAV6) {
    Profile* p = profileGet();
    uint8_t moodIndex = zone - TOUCH_ZONE_MOOD_CURIOUS;
    uint8_t animId = p->favoriteAnimations[moodIndex];
    if (animId < ANIMATION_COUNT) {
      motionTriggerAnimation(animId);
      playUISound(SOUND_CLICK);
      Serial.printf("[MainScreen] Favorite %d (anim %d) triggered\n", moodIndex, animId);
    }
  }

  // Button actions (NO DEADMAN REQUIRED - buttons work independently!)
  const ButtonState& btn = getButtonState();
  
  // Both joystick buttons = E-STOP toggle (tap again to resume)
  if (isBothJoystickButtonsHeld()) {
    static unsigned long lastBothHeld = 0;
    if (now - lastBothHeld > 300) {  // Debounce 300ms
      g_estop = !g_estop;
      lastBothHeld = now;
      if (g_estop) {
        ds.leftSpeed = 0;
        ds.rightSpeed = 0;
        g_controlAuthority = CTRL_SAFETY;
        motionEmergencyStop();
        playUISound(SOUND_ESTOP);
      } else {
        g_controlAuthority = CTRL_LOCAL;
        playUISound(SOUND_CLICK);
      }
    }
  }
  
  // USER-CUSTOMIZABLE: hardware button -> favorite slot mapping.
  // Deadman behavior must remain safety-only.
  // Individual joystick buttons = programmable favorites slot 0/1 (NO deadman required)
  if (btn.pressed[BTN_JOY1] && !isBothJoystickButtonsHeld()) {
    triggerButtonMappedAnimation(0, "JOY1");
  }
  
  if (btn.pressed[BTN_JOY2] && !isBothJoystickButtonsHeld()) {
    triggerButtonMappedAnimation(1, "JOY2");
  }
  
  // Extra buttons: programmable favorites slot 2..5 (deadman remains dedicated safety hold)
#if USE_CMD_BUTTON_MACROS
  commandInputPollButtonMacros(btn);
#else
  if (btn.pressed[BTN_EXTRA1]) triggerButtonMappedAnimation(2, "EXTRA1");
  if (btn.pressed[BTN_EXTRA2]) triggerButtonMappedAnimation(3, "EXTRA2");
  if (btn.pressed[BTN_EXTRA3]) triggerButtonMappedAnimation(4, "EXTRA3");
  if (btn.pressed[BTN_EXTRA4]) triggerButtonMappedAnimation(5, "EXTRA4");
#endif
  
  macroCheckJoystickOverride(joystickActive);

  float pbL, pbR, pbSv[9];
  if (macroGetPlaybackData(&pbL, &pbR, pbSv)) {
    ds.leftSpeed = (int8_t)constrain((int)lroundf(pbL), -100, 100);
    ds.rightSpeed = (int8_t)constrain((int)lroundf(pbR), -100, 100);
    for (int i = 0; i < 9; i++) {
      uint8_t deg = (uint8_t)constrain((int)lroundf(pbSv[i] * 180.0f / 100.0f), 0, 180);
      motionSetServoDirect(i, deg);
    }
  }
  if (!isDeadmanButtonHeld()) {
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
  }

  motionUpdate(now);

  {
    uint8_t macTgt[10];
    motionGetServoTargets(macTgt);
    float macSv[9];
    for (int i = 0; i < 9; i++) {
      macSv[i] = macTgt[i] * (100.0f / 180.0f);
    }
    macroSetCurrentData((float)ds.leftSpeed, (float)ds.rightSpeed, macSv);
  }
  macroUpdate(now);

  packetUpdate(now, &ds, g_estop);
  systemStatusTick(now);
  audioUpdate(now);

  // Buffered SD log flush (periodic inside sdUpdate)
  sdUpdate();

  // Static redraw on page/overlay change
  if (g_needStaticRedraw) {
    walleTaskWdtFeed();
    uiDrawCurrentPage();
    if (g_overlayVisible) uiDrawQuickActionOverlay();
    g_needStaticRedraw = false;
    walleTaskWdtFeed();
  }

  // Dynamic update
  TelemetryPacket telem;
  packetGetTelemetry(&telem);
  bool connected = packetTelemetryValid();

  g_motionPolicyFromBrain = connected ? telem.motionPolicy : 0;
  g_policyDenyCyd = connected && (telem.policyDenyCyd != 0);

  emotionRefreshFromTelemetry(&telem, connected);
  emotionApplyPoseToMotionEngine();

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
  strip.modeStr = g_estop ? "E-STOP" : (!connected ? "OFFLINE" :
                  (g_controlAuthority == CTRL_AUTONOMOUS ? "AUTO" :
                  g_controlAuthority == CTRL_SUPERVISED ? "SUPV" : "MANUAL"));
  strip.emotionStr = emotionGetName();

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

  uiDrawThinkingStrip(&telem, connected);
  uiUpdateBehaviourTileHighlights();
  uiDrawToast();

  if (g_advancedMode) {
    uiDrawAdvancedModeOverlay();
  }

  /* Laser button must be painted after all other Drive overlays (joystick dots, eye, toasts). */
  uiRenderingDrawDriveLaserOverlayIfNeeded();

  walleTaskWdtFeed();
  delay(1);
}
