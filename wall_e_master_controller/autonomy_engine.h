// ============================================================
//  WALL-E Master Controller — Autonomous Behaviour Engine
//  Full character autonomy with exploration, curiosity, and life
// ============================================================

#ifndef AUTONOMY_ENGINE_H
#define AUTONOMY_ENGINE_H

#include <Arduino.h>

// ============================================================
//  Autonomous State Machine
// ============================================================

enum AutoState {
  AUTO_IDLE,                  // Waiting / disabled
  AUTO_SCAN,                  // Scanning environment with neck sweep
  AUTO_EVALUATE,              // Processing scan results
  AUTO_APPROACH,              // Moving toward object of interest
  AUTO_INVESTIGATE_HEIGHT,    // Random height examination
  AUTO_REACT,                 // Reacting to close object
  AUTO_WANDER,                // Random exploration movement
  AUTO_AVOID,                 // Avoiding obstacle
  AUTO_ORIENT,                // Rotating to target heading
  AUTO_EXPLORE_LOOP           // Main exploration state
};

// ============================================================
//  Personality System
// ============================================================

struct Personality {
  float curiosityLevel;       // 0.0 - 1.0 (affects height investigation frequency)
  float braveryLevel;         // 0.0 - 1.0 (affects approach distance)
  float energyLevel;          // 0.0 - 1.0 (affects movement speed)
  float randomness;           // 0.0 - 1.0 (affects spontaneous actions)
};

// ============================================================
//  Location State (Compass + GPS)
// ============================================================

struct LocationState {
  double latitude;            // GPS latitude
  double longitude;           // GPS longitude
  float heading;              // Compass heading (0-360 degrees)
  bool gpsValid;              // GPS lock status
  bool compassValid;          // Compass valid
};

// ============================================================
//  Autonomous Context
// ============================================================

struct AutoContext {
  AutoState state;            // Current state
  uint32_t stateStartTime;    // When state was entered (millis)
  float targetHeading;        // Desired compass heading
  float detectedDistance;     // Current sonar distance (cm)
  float detectedHeightScore;  // Estimated object height (0-10)
  bool objectDetected;        // Object within threshold
  bool investigating;         // Currently investigating object
  float scanMinDistance;      // Minimum distance found during scan
  float scanBestHeading;      // Neck angle with closest object
  int investigationLevel;     // Random height level (1-10)
  uint32_t investigationStartTime;  // When investigation started
  uint32_t nextActionTime;    // When next action should occur
  float wanderSpeed;          // Current wander speed (0-1.0)
};

// ============================================================
//  Configuration Constants
// ============================================================

// Scan parameters
#define SCAN_LEFT_LIMIT   -60.0f   // Neck left limit (degrees)
#define SCAN_RIGHT_LIMIT   60.0f   // Neck right limit (degrees)
#define SCAN_SPEED         30.0f   // Degrees per second
#define SCAN_STEP_DELAY    100     // ms between scan steps

// Detection thresholds
#define DETECT_CLOSE_CM    40.0f   // Trigger reaction
#define DETECT_INTEREST_CM 80.0f   // Trigger investigation
#define DETECT_FAR_CM      200.0f  // Maximum detection range
#define DETECT_MIN_CM      5.0f    // Minimum valid reading

// Exploration timing
#define EXPLORE_HEADING_CHANGE_MIN   10000   // ms (10s)
#define EXPLORE_HEADING_CHANGE_MAX   20000   // ms (20s)
#define EXPLORE_PAUSE_CHANCE         20      // % chance to pause
#define EXPLORE_SPIN_CHANCE          10      // % chance to spin
#define EXPLORE_CURIOUS_CHANCE       15      // % chance to look around

// Investigation timing
#define INVESTIGATE_MIN_MS   800    // Minimum investigation time
#define INVESTIGATE_MAX_MS   2000   // Maximum investigation time

// Reaction timing
#define REACT_DURATION_MS    1500   // How long to react

// Safety timeouts
#define SONAR_TIMEOUT_MS     2000   // Max time without valid sonar
#define COMPASS_TIMEOUT_MS   3000   // Max time without valid compass

// ============================================================
//  Public API
// ============================================================

// Initialize autonomy engine
void autonomyInit();

// Update autonomy (call every loop)
void autonomyUpdate(uint32_t now);

// Enable/disable autonomous mode
void autonomySetEnabled(bool enabled);
bool autonomyIsEnabled();

// Get current state
AutoState autonomyGetState();
const char* autonomyGetStateName();

// Get context for UI display
const AutoContext* autonomyGetContext();
const LocationState* autonomyGetLocation();
const Personality* autonomyGetPersonality();

// Set personality parameters
void autonomySetPersonality(const Personality* p);

// Emergency stop (safety)
void autonomyEmergencyStop();

// Check if joystick is overriding
void autonomySetJoystickOverride(bool active);

#endif // AUTONOMY_ENGINE_H
