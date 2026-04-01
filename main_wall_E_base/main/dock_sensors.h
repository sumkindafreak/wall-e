/*******************************************************************************
 * dock_sensors.h
 * Obstacle bumpers on WALL-E base (onboard IR break-beam removed — use ToF + dock ESP-NOW).
 ******************************************************************************/

#ifndef DOCK_SENSORS_H
#define DOCK_SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/*
 * PIN PLAN (ESP32-S3 — avoid 33/34 Octal PSRAM)
 *   PIN_OBS_FRONT_L  (22), PIN_OBS_FRONT_R  (23)
 *   PIN_OBS_REAR_L     (20), PIN_OBS_REAR_R   (47)
 */

#define PIN_OBS_FRONT_L    22
#define PIN_OBS_FRONT_R    23
#define PIN_OBS_REAR_L     20
#define PIN_OBS_REAR_R     47

#define INVERT_OBS_FRONT_L     0
#define INVERT_OBS_FRONT_R     0
#define INVERT_OBS_REAR_L      0
#define INVERT_OBS_REAR_R      0

#define DOCK_DEBOUNCE_MS   50

extern bool gDockingEnabled;

void dockSensorsBegin(void);
void dockSensorsUpdate(void);

/** Legacy API — always false (no local IR break-beam). */
bool dockBeamPresent(void);

bool obstacleFrontBlocked(void);
bool dockObstacleFL(void);
bool dockObstacleFR(void);
bool dockObstacleRL(void);
bool dockObstacleRR(void);
bool obstacleRearBlocked(void);
bool anyObstacleBlocked(void);

void dockSensorsDebugPrint(uint32_t intervalMs);

#endif
