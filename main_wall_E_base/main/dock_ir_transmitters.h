#pragma once

#include <stdint.h>
#include "base_board_pins.h"

/*******************************************************************************
 * Dock IR transmitters — two ~38 kHz LEDs on WALL-E aimed at the dock's
 * left/right receivers. Alignment feedback returns via the dock radio beacon.
 ******************************************************************************/

#define DOCK_IR_TX_LEFT_PIN   BASE_PIN_DOCK_IR_LEFT
#define DOCK_IR_TX_RIGHT_PIN  BASE_PIN_DOCK_IR_RIGHT

void dockIrTransmittersInit(void);
void dockIrTransmittersSetEnabled(bool on);
void dockIrTransmittersUpdate(uint32_t now);
