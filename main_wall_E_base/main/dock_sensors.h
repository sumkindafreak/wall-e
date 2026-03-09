/*******************************************************************************
 * dock_sensors.h
 * Docking and obstacle sensors for WALL-E base (not yet installed)
 * Modular add-on — safe to run with sensors unplugged (INPUT_PULLUP + invert flags)
 ******************************************************************************/

#ifndef DOCK_SENSORS_H
#define DOCK_SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/*=============================================================================
 * PIN PLAN (ESP32-S3 — chosen to avoid clashes with display, sonar, motor, etc.)
 *=============================================================================
 * IR BEAM: Mount low on rear bumper (reverse-in dock) or inside dock bay gate
 *          so robot body breaks beam when fully seated.
 * OBSTACLES:
 *   Front-L/R: low, near track edges for curb/obstacle detection.
 *   Rear-L/R:  low, near track edges for reversing-into-dock protection.
 *
 * WIRING CHECKLIST:
 *   [ ] PIN_DOCK_IR_BEAM (18): VCC, GND, OUT -> GPIO 18 (3-pin module)
 *   [ ] PIN_OBS_FRONT_L  (22): VCC, GND, OUT -> GPIO 22
 *   [ ] PIN_OBS_FRONT_R  (23): VCC, GND, OUT -> GPIO 23
 *   [ ] PIN_OBS_REAR_L   (33): VCC, GND, OUT -> GPIO 33
 *   [ ] PIN_OBS_REAR_R   (34): VCC, GND, OUT -> GPIO 34
 *   [ ] Set INVERT_* flags if module is active-LOW instead of active-HIGH
 *===========================================================================*/

#define PIN_DOCK_IR_BEAM   18   /* Digital: dock presence / alignment confirm */
#define PIN_OBS_FRONT_L    22
#define PIN_OBS_FRONT_R    23
#define PIN_OBS_REAR_L     33
#define PIN_OBS_REAR_R     34
/* Optional: PIN_ACS712_ADC = 34 — not used in this module */

/* Invert flags (modules vary: active HIGH vs LOW) */
#define INVERT_DOCK_IR_BEAM    0   /* 0 = raw, 1 = invert logic */
#define INVERT_OBS_FRONT_L     0
#define INVERT_OBS_FRONT_R     0
#define INVERT_OBS_REAR_L      0
#define INVERT_OBS_REAR_R      0

#define DOCK_DEBOUNCE_MS   50   /* Require stable state for this long */

/* Global: dock sensor debug reporting (set by web UI DOCKSENS ON/OFF) */
extern bool gDockingEnabled;

/*=============================================================================
 * API
 *===========================================================================*/

/* Call once in setup() */
void dockSensorsBegin(void);

/* Call every loop — reads and debounces, non-blocking */
void dockSensorsUpdate(void);

/* dockBeamPresent: true when beam indicates "robot in dock zone" */
bool dockBeamPresent(void);

/* obstacleFrontBlocked: true if FL or FR active */
bool obstacleFrontBlocked(void);

/* Per-sensor (for UI): true when that sensor detects obstacle */
bool dockObstacleFL(void);
bool dockObstacleFR(void);
bool dockObstacleRL(void);
bool dockObstacleRR(void);

/* obstacleRearBlocked: true if RL or RR active */
bool obstacleRearBlocked(void);

/* anyObstacleBlocked: true if any obstacle sensor active */
bool anyObstacleBlocked(void);

/* Call at interval when docking reporting enabled — prints state change or periodic */
void dockSensorsDebugPrint(uint32_t intervalMs);

#endif /* DOCK_SENSORS_H */
