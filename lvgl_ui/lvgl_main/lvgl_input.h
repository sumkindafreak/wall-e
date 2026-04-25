#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "../../wall_e_master_controller/protocol.h"

// Touch -> LVGL
void lvglInputInit(void);
void lvglInputRead(lv_indev_t* indev, lv_indev_data_t* data);

// Hardware priority bridge
void lvglInputSetUiDrive(int8_t left, int8_t right);
void lvglInputSetUiDriveEnabled(bool en);
void lvglInputUpdateDriveFromHardware(DriveState* out);
bool lvglInputConsumeEstopEdge(void);

// Optional joystick enable/disable from settings page.
void lvglInputSetJoystickEnabled(bool en);
bool lvglInputJoystickEnabled(void);

/** On-screen joysticks (DRIVE tab): normalized -1..1 inside unit circle; active while finger down. */
void lvglInputSetVirtualJoy1(float x, float y, bool active);
void lvglInputSetVirtualJoy2(float x, float y, bool active);
bool lvglInputVirtualJoy1Active(void);
bool lvglInputVirtualJoy2Active(void);
float lvglInputVirtualJoy1X(void);
float lvglInputVirtualJoy1Y(void);
float lvglInputVirtualJoy2X(void);
float lvglInputVirtualJoy2Y(void);
