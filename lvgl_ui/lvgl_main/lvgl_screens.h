#pragma once

#include <lvgl.h>
#include "../../wall_e_master_controller/protocol.h"
#include "../../wall_e_master_controller/ads1115_input.h"

typedef struct {
  float batteryV;
  float currentA;
  bool linkOk;
  /** From Base telemetry: EVE companion UART link (v2). */
  bool eveUartOk;
  uint8_t mode;
  DriveState drive;
  JoystickState joystick;
} LvglRuntimeData;

typedef enum {
  LVGL_NAV_HOME = 0,
  LVGL_NAV_DRIVE,
  LVGL_NAV_BEHAVIOR,
  LVGL_NAV_AUDIO,
  LVGL_NAV_SETTINGS,
  LVGL_NAV_SD,
  LVGL_NAV_SYSTEM,
  LVGL_NAV_DOCK,
  LVGL_NAV_EVE = LVGL_NAV_DOCK
} LvglNavPage;

void lvglScreensInit(void);
void lvglScreensTick(const LvglRuntimeData* data);
void lvglScreensSetPage(LvglNavPage page);

// Backend actions (implemented in lvgl_ui.cpp)
void lvglUiActionSetUiDrive(int8_t left, int8_t right);
void lvglUiActionStopAll(void);
void lvglUiActionTriggerAnimation(uint8_t animId);
void lvglUiActionSetHeadTiltPct(int16_t pct);
void lvglUiActionSetHeadPanPct(int16_t pct);
void lvglUiActionSetEyebrowPct(int16_t pct);
void lvglUiActionSetServoSpeedPct(int16_t pct);
void lvglUiActionSavePosition(void);
void lvglUiActionLoadPosition(void);
void lvglUiActionSetVolumePct(int16_t pct);
void lvglUiActionDockGo(void);
void lvglUiActionDockCancel(void);
void lvglUiActionProfileSet(uint8_t id);
void lvglUiActionBrightnessSet(uint8_t level255);
void lvglUiActionToggleJoystick(bool enabled);
void lvglUiActionSdRefresh(void);
void lvglUiActionSdUp(void);
void lvglUiActionSdOpenSelected(uint16_t idx);
void lvglUiActionEveSendServo(int16_t headPanDeg, int16_t rightArmDeg);
