/*******************************************************************************
 * dock_sensors.h
 * Four obstacle sensors on WALL-E Base.
 ******************************************************************************/

#ifndef DOCK_SENSORS_H
#define DOCK_SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "base_board_pins.h"

#define PIN_OBS_FRONT_L  BASE_PIN_OBS_FRONT_L
#define PIN_OBS_FRONT_R  BASE_PIN_OBS_FRONT_R
#define PIN_OBS_REAR_L   BASE_PIN_OBS_REAR_L
#define PIN_OBS_REAR_R   BASE_PIN_OBS_REAR_R

#define INVERT_OBS_FRONT_L   0
#define INVERT_OBS_FRONT_R   0
#define INVERT_OBS_REAR_L    0
#define INVERT_OBS_REAR_R    0

#define DOCK_DEBOUNCE_MS  50

extern bool gDockingEnabled;

void dockSensorsBegin(void);
void dockSensorsUpdate(void);

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
