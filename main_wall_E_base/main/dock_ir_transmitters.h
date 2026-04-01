#pragma once

#include <stdint.h>

/*******************************************************************************
 * Dock IR transmitters — two ~38 kHz LEDs on WALL-E aimed at the dock’s two
 * TSOP-style receivers. Alignment feedback: dock → ESP-NOW ir_align_hint.
 * Edit pins here if they clash with I2C or other wiring.
 ******************************************************************************/

#define DOCK_IR_TX_LEFT_PIN   21
#define DOCK_IR_TX_RIGHT_PIN  38

void dockIrTransmittersInit(void);
void dockIrTransmittersSetEnabled(bool on);
void dockIrTransmittersUpdate(uint32_t now);
