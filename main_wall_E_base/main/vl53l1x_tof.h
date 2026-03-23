#pragma once

/*******************************************************************************
 * vl53l1x_tof.h
 * VL53L1X Time-of-Flight sensor — forward-facing on WALL-E base
 * Part of docking system: distance to dock bay / obstacle ahead
 * I2C shared with PCA9685, MPU6050. No ToF on dock — base only.
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/* Init (call after Wire.begin). Returns true if sensor found. */
bool tofInit(void);

/* Poll every loop — non-blocking. */
void tofUpdate(uint32_t now);

/* Distance in mm. 0 = invalid/no reading. VL53L1X range ~30–4000mm. */
uint16_t tofGetDistanceMm(void);

/* True if last read was successful. */
bool tofIsValid(void);
