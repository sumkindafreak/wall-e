/*******************************************************************************
 * ir_beacon_receivers.h
 * Left/right IR beacon receivers for dock alignment
 *
 * Dock has IR Beacon LED Left and Right. WALL-E mounts two IR receivers
 * to compare signal strength and steer until aligned (left ≈ right).
 *
 * Pin config (ESP32-S3: avoid 33/34 — Octal PSRAM reserved):
 *   IR_LEFT_PIN  = 21 (or 33 if not using S3 Octal PSRAM)
 *   IR_RIGHT_PIN = 38 (or 32)
 *
 * Analog or digital: If using analog output receivers, we read strength.
 * If digital, we use pulse density / detection rate. This module uses
 * analogRead for simplicity — works with TSOP-style analog output.
 ******************************************************************************/

#ifndef IR_BEACON_RECEIVERS_H
#define IR_BEACON_RECEIVERS_H

#include <stdint.h>

/* Pin assignment — change for your wiring */
#define IR_BEACON_LEFT_PIN   21
#define IR_BEACON_RIGHT_PIN  38

/* Call once in setup */
void irBeaconInit(void);

/* Call every loop — non-blocking */
void irBeaconUpdate(uint32_t now);

/* Raw ADC 0..4095 (12-bit) or equivalent */
uint16_t irBeaconGetLeft(void);
uint16_t irBeaconGetRight(void);

/* Difference: positive = right stronger; negative = left stronger */
int16_t irBeaconGetBalance(void);

/* True if either receiver sees significant signal */
bool irBeaconAnyDetected(void);

#endif /* IR_BEACON_RECEIVERS_H */
