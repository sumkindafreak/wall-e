// ============================================================
//  CYD laser UI — beam on/off only (no screen aim; head follows robot)
//  FLAG_LASER via ESP-NOW when armed
// ============================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

/* Single toggle — left pane, clear of midX divider (160) */
#define CYD_LASER_BTN_X   8
#define CYD_LASER_BTN_Y   138
#define CYD_LASER_BTN_W   110
#define CYD_LASER_BTN_H   38

void cydLaserUiInit(void);

void cydLaserUiSetArmed(bool on);
bool cydLaserUiGetArmed(void);
void cydLaserUiToggleArmed(void);

/** OR into outgoing ControlPacket.systemFlags */
uint16_t cydLaserUiGetExtraFlags(void);
