// ============================================================
// WALL-E Base — ultrasonic sonar sensor
// ============================================================

#ifndef SONAR_SENSOR_H
#define SONAR_SENSOR_H

#include <Arduino.h>
#include "base_board_pins.h"

#define SONAR_TRIGGER_PIN  BASE_PIN_SONAR_TRIGGER
#define SONAR_ECHO_PIN     BASE_PIN_SONAR_ECHO

#define SONAR_TIMEOUT_US      25000
#define SONAR_UPDATE_MS       50
#define SONAR_FILTER_SIZE     5
#define SONAR_SPEED_OF_SOUND  343.0f

#define SONAR_MIN_CM   5.0f
#define SONAR_MAX_CM   200.0f

bool sonarInit();
void sonarUpdate(uint32_t now);

float sonarGetDistanceCm();
bool sonarIsValid();
float sonarGetRawDistanceCm();
uint32_t sonarGetLastUpdateMs();

bool sonarIsObjectDetected(float maxRangeCm = 200.0f);
bool sonarIsObjectClose(float thresholdCm = 40.0f);

#endif // SONAR_SENSOR_H
