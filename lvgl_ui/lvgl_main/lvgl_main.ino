// ============================================================
// SAFE LVGL MIGRATION ENTRYPOINT (non-destructive clone)
// This file does NOT modify your existing direct-draw project.
// ============================================================

#include <Arduino.h>
#include <stdio.h>

#include "../../wall_e_master_controller/protocol.h"
#include "../../wall_e_master_controller/packet_control.h"
#include "../../wall_e_master_controller/motion_engine.h"
#include "../../wall_e_master_controller/audio_system.h"
#include "../../wall_e_master_controller/profiles.h"
#include "../../wall_e_master_controller/command_input.h"
#include "../../wall_e_master_controller/i2c_devices.h"
#include "../../wall_e_master_controller/ads1115_input.h"
#include "../../wall_e_master_controller/sx1509_input.h"
#include "../../wall_e_master_controller/sd_manager.h"
#include "../../wall_e_master_controller/sd_browser.h"
#include "../../wall_e_master_controller/system_status.h"
// --- ADDED: emotional memory + flashback (RAM + SD /wall_e/memory/events.log)
#include "../../wall_e_master_controller/memory_system.h"

#include "lvgl_init.h"
#include "lvgl_input.h"
#include "lvgl_ui.h"

static bool s_estop = false;
static DriveState s_drive = {};
static bool s_sdReady = false;

// --- ADDED: flashback toast on LVGL UI (head nudge inside memoryPoll)
static void lvglMemoryFlashback(float w, const char* d) {
  char m[52];
  snprintf(m, sizeof(m), "Flashback %.1f: %s", w, d && d[0] ? d : "?");
  lvglUiShowToast(m, 700u);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  yield();
  Serial.println("[LVGL-UI] WALL-E CYD clone UI boot");

  // Keep existing backend bring-up paths.
  s_sdReady = sdInit();
  yield();
  lvglUiSetSdReady(s_sdReady);
  if (s_sdReady) {
    sdLogInit();
    sdBrowserOnEnterPage();
  }
  yield();
  memoryInit();
  memoryRegisterFlashbackHandler(lvglMemoryFlashback);
  i2cInit();
  yield();
  ads1115Init();
  sx1509Init();
  profileInit();
  yield();
  motionInit();
  audioInit();
  packetInit();
  commandInputInit();
  systemStatusInit();
  sdBrowserOnEnterPage();
  yield();

  lvglPlatformInit();
  lvglUiInit();
  yield();
  Serial.println("[LVGL-UI] Ready");
}

void loop() {
  yield();
  const uint32_t now = millis();

  // --- ADDED: decay, SD append, delayed flashback — non-blocking
  memoryPoll(now);
  yield();

  /* Reset chord before LVGL / heavy UI so it always runs first */
  sx1509Update();
  sx1509PollEspResetChord();
  {
    static uint8_t s_prevResetCd = 0;
    const uint8_t cd = sx1509GetResetCountdownDigit();
    if (cd != s_prevResetCd) {
      if (cd >= 1u && cd <= 3u) {
        char m[40];
        snprintf(m, sizeof(m), "Restarting in %u...", (unsigned)cd);
        lvglUiShowToast(m, 1200u);
      }
      s_prevResetCd = cd;
    }
  }

  ads1115Update();
  commandInputPollSerial();
  systemStatusTick(now);
  audioUpdate(now);
  if (s_sdReady) sdUpdate();
  yield();

  // Priority: physical joystick / deadman always win.
  lvglInputUpdateDriveFromHardware(&s_drive);

  const JoystickState& joy = getJoystickState();
  if (lvglInputVirtualJoy1Active()) {
    motionSetHeadPanVelocity(lvglInputVirtualJoy1X());
    motionSetHeadTiltVelocity(lvglInputVirtualJoy1Y());
  } else {
    motionSetHeadPanVelocity(joy.processed[JOY1_X]);
    motionSetHeadTiltVelocity(joy.processed[JOY1_Y]);
  }

  if (lvglInputConsumeEstopEdge()) {
    s_estop = !s_estop;
    if (s_estop) {
      motionEmergencyStop();
      packetSetPendingAction(ACTION_STOP_ALL);
    }
  }

  motionUpdate(now);
  // --- ADDED: tread bias from memory before TX
  memoryApplyDriveInfluence(&s_drive);
  packetUpdate(now, &s_drive, s_estop);
  yield();

  TelemetryPacket tm = {};
  bool telemValid = packetTelemetryValid();
  if (telemValid) packetGetTelemetry(&tm);

  lvglUiSetTelemetry(&tm, telemValid);
  lvglUiSetJoystick(&joy);
  lvglUiSetDriveState(&s_drive);
  lvglUiTick(now);
  yield();

  lvglPlatformTask();
  yield();
  delay(1);
}
