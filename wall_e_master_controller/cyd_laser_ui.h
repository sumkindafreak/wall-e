// ============================================================
//  CYD laser UI state — no extra GPIO; beam + aim via ESP-NOW
//  (FLAG_LASER + servoTargets for head pan/tilt on base)
// ============================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

/* Touch regions — left pane below "Battery" label, clear of midX divider (160) */
#define CYD_LASER_PAD_X   8
#define CYD_LASER_PAD_Y   134
#define CYD_LASER_PAD_W   110
#define CYD_LASER_PAD_H   58

#define CYD_LASER_FIRE_X  (CYD_LASER_PAD_X + CYD_LASER_PAD_W - 40)
#define CYD_LASER_FIRE_Y  (CYD_LASER_PAD_Y + 4)
#define CYD_LASER_FIRE_W  36
#define CYD_LASER_FIRE_H  18

void cydLaserUiInit(void);

void cydLaserUiSetArmed(bool on);
bool cydLaserUiGetArmed(void);
void cydLaserUiToggleArmed(void);

void cydLaserUiBeginFrame(void);
void cydLaserUiCancelDrag(void);

/** Call while finger in laser pad (screen coords). */
void cydLaserUiDragFromScreen(int screenX, int screenY);

bool cydLaserUiIsDraggingHead(void);

/** Apply joystick-style overrides for head when dragging (call before motionUpdate). */
void cydLaserUiApplyMotion(void);

/** OR into outgoing ControlPacket.systemFlags */
uint16_t cydLaserUiGetExtraFlags(void);

/** Last aim 0–100 for crosshair draw */
void cydLaserUiGetAim(uint8_t* panOut, uint8_t* tiltOut);
