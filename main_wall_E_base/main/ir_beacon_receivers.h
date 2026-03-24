/*******************************************************************************
 * ir_beacon_receivers.h
 * WALL-E front IR transmitters (clear 940 nm LEDs) for dock TSOP alignment.
 * Dock receivers decode 38 kHz; this module uses LEDC ~38 kHz on ESP32.
 *
 * Pin config (unchanged GPIO — rewire from old “receiver” to LED + resistor):
 *   IR_BEACON_LEFT_PIN  = 21
 *   IR_BEACON_RIGHT_PIN = 38
 ******************************************************************************/

#ifndef IR_BEACON_RECEIVERS_H
#define IR_BEACON_RECEIVERS_H

#include <stdint.h>
#include <stdbool.h>

#define IR_BEACON_LEFT_PIN   21
#define IR_BEACON_RIGHT_PIN  38

void irBeaconInit(void);

/* Enable both IR LEDs (38 kHz PWM on ESP32). Call off when not docking. */
void irBeaconSetTransmitEnabled(bool on);

/* Call every loop (reserved for future timing). */
void irBeaconUpdate(uint32_t now);

/* Legacy stubs for older code / telemetry (alignment uses ESP-NOW ir_align_hint). */
uint16_t irBeaconGetLeft(void);
uint16_t irBeaconGetRight(void);
int16_t irBeaconGetBalance(void);
bool irBeaconAnyDetected(void);

#endif /* IR_BEACON_RECEIVERS_H */
