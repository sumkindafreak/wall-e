// ============================================================
//  WALL-E Master Controller — Ultrasonic Sonar Sensor
//  Non-blocking distance measurement for right eye
// ============================================================

#ifndef SONAR_SENSOR_H
#define SONAR_SENSOR_H

#include <Arduino.h>

// Pin definitions (adjust for your hardware)
#define SONAR_TRIGGER_PIN  26   // GPIO for trigger
#define SONAR_ECHO_PIN     27   // GPIO for echo

// Measurement constants
#define SONAR_TIMEOUT_US     25000   // 25ms timeout (~4m range)
#define SONAR_UPDATE_MS      50      // Update every 50ms (20Hz)
#define SONAR_FILTER_SIZE    5       // Rolling average filter size
#define SONAR_SPEED_OF_SOUND 343.0f  // m/s at 20°C

// Valid range
#define SONAR_MIN_CM   5.0f
#define SONAR_MAX_CM   200.0f

// Initialize sonar
bool sonarInit();

// Update sonar (non-blocking, call every loop)
void sonarUpdate(uint32_t now);

// Get current distance
float sonarGetDistanceCm();

// Check if reading is valid
bool sonarIsValid();

// Get raw unfiltered distance
float sonarGetRawDistanceCm();

// Get last update time
uint32_t sonarGetLastUpdateMs();

// Behavioral: object detection for interest engine
// Returns true if valid reading and distance within range (default 5-200cm)
bool sonarIsObjectDetected(float maxRangeCm = 200.0f);
bool sonarIsObjectClose(float thresholdCm = 40.0f);  // For REACT state

#endif // SONAR_SENSOR_H
