/*******************************************************************************
 * dock_sonar.h
 * HC-SR04 (or compatible) sonar for last-resort dock detection and future guidance.
 * When USE_SONAR is 1, distance < SONAR_DOCKED_CM is treated as "docked" if beam doesn't register.
 ******************************************************************************/

#ifndef DOCK_SONAR_H
#define DOCK_SONAR_H

#include <stdbool.h>

/* Call once at boot when USE_SONAR is 1. */
void dockSonarBegin(void);

/* Call every loop (e.g. from dockSensorsUpdate). Updates distance. */
void dockSonarUpdate(void);

/* Last valid distance in cm (0..SONAR_MAX_CM), or 0 if no echo / disabled. */
float dockSonarDistanceCm(void);

/* True when sonar is enabled, has valid reading, and distance < SONAR_DOCKED_CM (last-resort "docked"). */
bool dockSonarInRange(void);

#endif /* DOCK_SONAR_H */
