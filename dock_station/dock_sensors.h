/*******************************************************************************
 * dock_sensors.h
 * ACS712 current sensor, IR beam, and obstacle sensors for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_SENSORS_H
#define DOCK_SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/* Include dock_config.h before this header for ACS712_* and USE_OBSTACLE_SENSORS */

/*=============================================================================
 * PUBLIC API
 *===========================================================================*/

/* Call once at boot. Calibrates ACS712 zero point. Disable charging first! */
void dockSensorsBegin(void);

/* Call every loop iteration. Updates all sensor readings. Non-blocking. */
void dockSensorsUpdate(void);

/* --- IR beam --- */
/* true = beam broken = robot present */
bool dockBeamPresent(void);

/* --- Obstacles (mouth blocked) --- */
/* true = any obstacle sensor active */
bool dockMouthBlocked(void);

/* --- Current --- */
/* Raw ADC value (0-4095 on ESP32) */
int dockCurrentRaw(void);

/* Zero point from calibration (ADC units) */
int dockCurrentZero(void);

/* Filtered current in Amps (positive = charging) */
float dockCurrentAmps(void);

/* Last raw voltage at ADC pin (mV) */
uint32_t dockCurrentVoltageMv(void);

#endif /* DOCK_SENSORS_H */
