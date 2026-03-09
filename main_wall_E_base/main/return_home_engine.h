#pragma once

// ============================================================
//  WALL-E Return Home Engine
//  GPS-based navigation back to home base
//  Self-preservation instinct when battery low
// ============================================================

#include <Arduino.h>

// ============================================================
//  Return Home Configuration
// ============================================================

#define RTH_ARRIVAL_RADIUS_M 2.0f      // Within 2m = arrived
#define RTH_SLOW_RADIUS_M 10.0f        // Slow down within 10m
#define RTH_HEADING_TOLERANCE 15.0f    // +/- 15 degrees = close enough
#define RTH_MIN_BATTERY_PERCENT 25.0f  // Auto-trigger below 25%
#define RTH_CRITICAL_BATTERY 15.0f     // Critical - maximum urgency

// ============================================================
//  Return Home State
// ============================================================

enum RTHState {
  RTH_IDLE,           // Not returning home
  RTH_ORIENTING,      // Rotating to face home
  RTH_NAVIGATING,     // Moving toward home
  RTH_AVOIDING,       // Temporary obstacle avoidance
  RTH_ARRIVED,        // Reached home
  RTH_FAILED          // Cannot reach home (lost GPS, etc.)
};

struct ReturnHomeContext {
  RTHState state;
  uint32_t stateStartTime;
  
  // Navigation data
  double homeLatitude;
  double homeLongitude;
  float distanceToHome;      // Meters
  float headingToHome;       // 0-360 degrees
  float currentHeading;      // From compass
  
  // Obstacle handling
  float obstacleDistance;    // From sonar
  bool obstacleDetected;
  uint32_t avoidanceStartTime;
  
  // Progress tracking
  float initialDistance;     // Distance when RTH started
  uint32_t navigationStartTime;
  float batteryAtStart;
  
  // Failure detection
  uint32_t lastProgressTime;
  float lastDistance;
  uint8_t stuckCount;
};

// ============================================================
//  API
// ============================================================

void returnHomeInit();
void returnHomeUpdate(uint32_t now, double currentLat, double currentLon, 
                      float currentHeading, float sonarDistance, float batteryPercent);

// Control
void returnHomeStart(double homeLat, double homeLon, float batteryPercent);
void returnHomeCancel();
bool returnHomeIsActive();
bool returnHomeHasArrived();

// State
RTHState returnHomeGetState();
const char* returnHomeGetStateName();

// Navigation data
float returnHomeGetDistance();
float returnHomeGetHeading();
float returnHomeGetProgress();  // 0.0 - 1.0 (0% - 100%)

// Motor commands (for autonomy engine)
void returnHomeGetMotorCommands(int8_t* leftSpeed, int8_t* rightSpeed);

// Auto-trigger
bool returnHomeShouldTrigger(float batteryPercent);
float returnHomeGetUrgency(float batteryPercent);  // 0.0 - 1.0

// Debug
void returnHomePrintDebug();
uint32_t returnHomeGetElapsedTime();
