// ============================================================
//  WALL-E Base Autonomous Behaviour Engine
//  Full character autonomy with waypoint navigation
//  Runs on Base ESP32-S3 where sensors are located
// ============================================================

#include "autonomy_engine.h"
#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
#include "waypoint_nav.h"
#include "imu_manager.h"
#include "servo_manager.h"
#include "personality_engine.h"
#include "emotion_engine.h"
#include "interest_engine.h"
#include "memory_engine.h"
#include "return_home_engine.h"
#include "battery_monitor.h"
#include <Arduino.h>

// ============================================================
//  Internal State
// ============================================================

static bool s_enabled = false;
static bool s_manualOverride = false;
static AutoContext s_context;
static LocationState s_location;

// Neck scan state (servo control)
static float s_neckAngle = 0.0f;
static bool s_scanningLeft = true;
static uint32_t s_lastScanStepMs = 0;

// Exploration state
static float s_exploreTargetHeading = 0.0f;
static uint32_t s_nextHeadingChangeMs = 0;

// Idle behaviour timers
static uint32_t s_nextIdleActionMs = 0;

// Safety
static uint32_t s_lastSonarValidMs = 0;
static uint32_t s_lastCompassValidMs = 0;

// Motor output
static int8_t s_leftSpeed = 0;
static int8_t s_rightSpeed = 0;

// ============================================================
//  State Name Lookup
// ============================================================

static const char* getStateName(AutoState state) {
  switch (state) {
    case AUTO_IDLE:              return "IDLE";
    case AUTO_SCAN:              return "SCAN";
    case AUTO_TRACK_OBJECT:      return "TRACK";
    case AUTO_EVALUATE:          return "EVALUATE";
    case AUTO_APPROACH:          return "APPROACH";
    case AUTO_INVESTIGATE_HEIGHT: return "INVESTIGATE";
    case AUTO_REACT:             return "REACT";
    case AUTO_WANDER:            return "WANDER";
    case AUTO_AVOID:             return "AVOID";
    case AUTO_ORIENT:            return "ORIENT";
    case AUTO_THINK:             return "THINK";
    case AUTO_RETURN_HOME:       return "RETURN_HOME";
    case AUTO_EXPLORE_LOOP:      return "EXPLORE";
    case AUTO_NAVIGATE_WAYPOINT: return "WAYPOINT_NAV";
    default:                     return "UNKNOWN";
  }
}

// ============================================================
//  State Transition
// ============================================================

static void transitionToState(AutoState newState, uint32_t now) {
  if (s_context.state != newState) {
    Serial.printf("[Autonomy] %s → %s\n", getStateName(s_context.state), getStateName(newState));
    s_context.state = newState;
    s_context.stateStartTime = now;
  }
}

// ============================================================
//  Neck Control for Scanning (Servo)
// ============================================================

static void updateNeckScan(uint32_t now) {
  // Time-based neck movement (non-blocking)
  if (now - s_lastScanStepMs < SCAN_STEP_DELAY) {
    return;
  }
  s_lastScanStepMs = now;
  
  // Calculate step based on scan speed
  float step = SCAN_SPEED * (SCAN_STEP_DELAY / 1000.0f);
  
  if (s_scanningLeft) {
    s_neckAngle -= step;
    if (s_neckAngle <= SCAN_LEFT_LIMIT) {
      s_neckAngle = SCAN_LEFT_LIMIT;
      s_scanningLeft = false;
    }
  } else {
    s_neckAngle += step;
    if (s_neckAngle >= SCAN_RIGHT_LIMIT) {
      s_neckAngle = SCAN_RIGHT_LIMIT;
      s_scanningLeft = true;
    }
  }
  
  // Apply to servo (neck pan servo - assuming SERVO_HEAD_PAN = 0)
  // Convert angle to 0-100 scale (90deg = 50 center, +/- 30deg scan)
  uint8_t servoPos = (uint8_t)constrain(50 + (s_neckAngle * 50 / 90), 0, 100);
  servoSet(SERVO_HEAD_PAN, servoPos, SERVO_FAST_SPEED);  // Neck pan
}

// ============================================================
//  Height Investigation
// ============================================================

static void startHeightInvestigation(uint32_t now) {
  // Generate random height level (1-10)
  s_context.investigationLevel = random(1, 11);
  s_context.investigationStartTime = now;
  
  // Random duration
  uint32_t duration = random(INVESTIGATE_MIN_MS, INVESTIGATE_MAX_MS);
  s_context.nextActionTime = now + duration;
  
  Serial.printf("[Autonomy] Investigating height level %d\n", s_context.investigationLevel);
  
  // Map level to tilt servo position (0-100 scale)
  uint8_t tiltPos = 50;  // Center
  if (s_context.investigationLevel <= 3) {
    tiltPos = map(s_context.investigationLevel, 1, 3, 30, 45);  // Look down
  } else if (s_context.investigationLevel >= 8) {
    tiltPos = map(s_context.investigationLevel, 8, 10, 55, 70);  // Look up
  }
  
  // Apply tilt (SERVO_NECK_TOP or SERVO_NECK_BOT - using neck top)
  servoSet(SERVO_NECK_TOP, tiltPos, SERVO_FAST_SPEED);
}

// ============================================================
//  Idle Life Behaviours
// ============================================================

static void updateIdleBehaviours(uint32_t now) {
  if (now > s_nextIdleActionMs) {
    int action = random(100);
    
    if (action < (int)(personalityGetRandomness() * 20)) {
      // Random neck tilt (slight variation)
      uint8_t tilt = random(40, 60);
      servoSet(SERVO_NECK_TOP, tilt, SERVO_SLOW_SPEED);
    } else if (action < (int)(personalityGetRandomness() * 35)) {
      // Random head pan (slight variation)
      uint8_t pan = random(45, 55);
      servoSet(SERVO_HEAD_PAN, pan, SERVO_SLOW_SPEED);
    }
    
    s_nextIdleActionMs = now + random(3000, 8000);
  }
}

// ============================================================
//  Waypoint Navigation
// ============================================================

static void updateWaypointNavigation(uint32_t now) {
  if (!s_location.gpsValid || !s_location.compassValid) {
    Serial.println(F("[Autonomy] GPS/Compass invalid - pausing waypoint nav"));
    transitionToState(AUTO_IDLE, now);
    return;
  }
  
  const NavState* navState = waypointGetNavState();
  if (!navState->waypointActive) {
    Serial.println(F("[Autonomy] No active waypoint"));
    transitionToState(AUTO_IDLE, now);
    return;
  }
  
  // Check for obstacles
  float dist = sonarGetDistanceCm();
  if (sonarIsValid() && dist < DETECT_CLOSE_CM) {
    Serial.printf("[Autonomy] Obstacle at %.1fcm - pausing waypoint nav\n", dist);
    transitionToState(AUTO_AVOID, now);
    return;
  }
  
  // Calculate heading error
  float error = navState->bearingToWaypoint - s_location.heading;
  if (error > 180.0f) error -= 360.0f;
  if (error < -180.0f) error += 360.0f;
  
  // If heading is off, orient first
  if (fabs(error) > WAYPOINT_HEADING_TOLERANCE) {
    s_context.targetHeading = navState->bearingToWaypoint;
    transitionToState(AUTO_ORIENT, now);
    return;
  }
  
  // Move toward waypoint
  float speed = 40.0f * personalityGetEnergy();
  s_leftSpeed = (int8_t)speed;
  s_rightSpeed = (int8_t)speed;
  
  // Debug
  static uint32_t lastDebug = 0;
  if (now - lastDebug > 2000) {
    Serial.printf("[Autonomy] Waypoint: %.1fm @ %.0f° (heading: %.0f°)\n",
      navState->distanceToWaypoint, navState->bearingToWaypoint, s_location.heading);
    lastDebug = now;
  }
}

// ============================================================
//  Exploration Logic
// ============================================================

static void updateExploration(uint32_t now) {
  // Object detected while exploring -> track it and build interest
  if (sonarIsObjectDetected(DETECT_INTEREST_CM)) {
    s_context.scanBestHeading = s_neckAngle;
    emotionTriggerDiscovery();
    transitionToState(AUTO_TRACK_OBJECT, now);
    return;
  }
  
  if (now > s_nextHeadingChangeMs) {
    s_exploreTargetHeading = random(0, 360);
    s_nextHeadingChangeMs = now + random(EXPLORE_HEADING_CHANGE_MIN, EXPLORE_HEADING_CHANGE_MAX);
    Serial.printf("[Autonomy] New target heading: %.0f°\n", s_exploreTargetHeading);
    transitionToState(AUTO_ORIENT, now);
    return;
  }
  
  int chance = random(100);
  if (chance < EXPLORE_PAUSE_CHANCE) {
    s_leftSpeed = 0;
    s_rightSpeed = 0;
    transitionToState(AUTO_SCAN, now);
    return;
  } else if (chance < (EXPLORE_PAUSE_CHANCE + EXPLORE_SPIN_CHANCE)) {
    s_exploreTargetHeading = s_location.heading + 180.0f;
    if (s_exploreTargetHeading >= 360.0f) s_exploreTargetHeading -= 360.0f;
    transitionToState(AUTO_ORIENT, now);
    return;
  } else if (chance < (EXPLORE_PAUSE_CHANCE + EXPLORE_SPIN_CHANCE + EXPLORE_CURIOUS_CHANCE)) {
    s_leftSpeed = 0;
    s_rightSpeed = 0;
    transitionToState(AUTO_THINK, now);
    return;
  }
  
  float speed = personalityGetEnergy() * 50.0f * emotionGetMovementSpeed();
  s_leftSpeed = (int8_t)speed;
  s_rightSpeed = (int8_t)speed;
}

// ============================================================
//  Obstacle Avoidance
// ============================================================

static void updateAvoidance(uint32_t now) {
  s_leftSpeed = -30;
  s_rightSpeed = -30;
  
  if (now - s_context.stateStartTime > 1000) {
    float turnAmount = random(60, 120);
    if (random(0, 2) == 0) turnAmount = -turnAmount;
    
    s_exploreTargetHeading = s_location.heading + turnAmount;
    if (s_exploreTargetHeading < 0) s_exploreTargetHeading += 360.0f;
    if (s_exploreTargetHeading >= 360.0f) s_exploreTargetHeading -= 360.0f;
    
    // Return to waypoint mode if active
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_ORIENT, now);
    }
  }
}

// ============================================================
//  Orientation (Turn to Heading)
// ============================================================

static void updateOrientation(uint32_t now) {
  if (!s_location.compassValid) {
    transitionToState(AUTO_EXPLORE_LOOP, now);
    return;
  }
  
  float error = s_context.targetHeading - s_location.heading;
  if (error > 180.0f) error -= 360.0f;
  if (error < -180.0f) error += 360.0f;
  
  if (fabs(error) < 5.0f) {
    s_leftSpeed = 0;
    s_rightSpeed = 0;
    
    // Return to appropriate mode
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_EXPLORE_LOOP, now);
    }
    return;
  }
  
  int8_t turnSpeed = (int8_t)constrain(error * 0.5f, -40.0f, 40.0f);
  s_leftSpeed = turnSpeed;
  s_rightSpeed = -turnSpeed;
}

// ============================================================
//  Scanning State
// ============================================================

static void updateScanState(uint32_t now) {
  updateNeckScan(now);
  
  float dist = sonarGetDistanceCm();
  if (sonarIsValid()) {
    if (dist < s_context.scanMinDistance) {
      s_context.scanMinDistance = dist;
      s_context.scanBestHeading = s_neckAngle;
    }
  }
  
  if (!s_scanningLeft && s_neckAngle >= 0) {
    Serial.printf("[Autonomy] Scan complete - min dist: %.1fcm at angle %.0f°\n",
      s_context.scanMinDistance, s_context.scanBestHeading);
    transitionToState(AUTO_EVALUATE, now);
  }
}

// ============================================================
//  Evaluation State
// ============================================================

static void updateEvaluateState(uint32_t now) {
  if (s_context.scanMinDistance < DETECT_CLOSE_CM) {
    transitionToState(AUTO_AVOID, now);
  } else if (s_context.scanMinDistance < DETECT_INTEREST_CM) {
    s_context.objectDetected = true;
    // Point head toward detected object (convert heading to 0-100 scale)
    uint8_t neckPos = (uint8_t)constrain(50 + (s_context.scanBestHeading * 50 / 90), 0, 100);
    servoSet(SERVO_HEAD_PAN, neckPos, SERVO_FAST_SPEED);
    
    if (random(100) < (int)(personalityGetCuriosity() * 100)) {
      transitionToState(AUTO_INVESTIGATE_HEIGHT, now);
    } else {
      transitionToState(AUTO_APPROACH, now);
    }
  } else {
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_EXPLORE_LOOP, now);
    }
  }
  
  s_context.scanMinDistance = DETECT_FAR_CM;
}

// ============================================================
//  Approach State
// ============================================================

static void updateApproachState(uint32_t now) {
  float dist = sonarGetDistanceCm();
  
  if (!sonarIsValid() || dist > DETECT_INTEREST_CM) {
    transitionToState(AUTO_SCAN, now);
    return;
  }
  
  if (dist < (DETECT_CLOSE_CM * personalityGetBravery())) {
    transitionToState(AUTO_REACT, now);
    return;
  }
  
  float speed = 30.0f * personalityGetEnergy();
  s_leftSpeed = (int8_t)speed;
  s_rightSpeed = (int8_t)speed;
}

// ============================================================
//  React State
// ============================================================

static void updateReactState(uint32_t now) {
  emotionTriggerObstacleNear();
  if (now - s_context.stateStartTime < REACT_DURATION_MS) {
    s_leftSpeed = 0;
    s_rightSpeed = 0;
  } else {
    transitionToState(AUTO_AVOID, now);
  }
}

// ============================================================
//  Track Object State (neck + micro eye toward object)
// ============================================================

static void updateTrackObjectState(uint32_t now) {
  float dist = sonarGetDistanceCm();
  bool objectThere = sonarIsObjectDetected(DETECT_INTEREST_CM);
  
  if (!objectThere) {
    interestReset();
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_EXPLORE_LOOP, now);
    }
    return;
  }
  
  // Object heading = body heading + neck angle (neck relative to body)
  float objectHeading = s_location.heading + s_neckAngle;
  while (objectHeading < 0) objectHeading += 360.0f;
  while (objectHeading >= 360.0f) objectHeading -= 360.0f;
  
  memoryUpdateObject(dist, objectHeading, now);
  
  if (interestShouldInvestigate()) {
    emotionTriggerInvestigateStart();
    interestReset();
    transitionToState(AUTO_INVESTIGATE_HEIGHT, now);
    return;
  }
  
  // Keep neck pointed at object (scanBestHeading from last scan, or hold)
  uint8_t neckPos = (uint8_t)constrain(50 + (s_context.scanBestHeading * 50 / 90), 0, 100);
  servoSet(SERVO_HEAD_PAN, neckPos, SERVO_FAST_SPEED);
  
  // Micro eye tracking: small offset toward object (intensity scales with interest)
  float interestNorm = interestGetLevel() / 100.0f;
  int eyeOffset = (int)(interestNorm * 5.0f);  // 0-5 pixels "look" toward object
  if (s_context.scanBestHeading < 0) eyeOffset = -eyeOffset;
  uint8_t eyePos = (uint8_t)constrain(50 + eyeOffset, 45, 55);
  servoSet(SERVO_EYE_LEFT, eyePos, SERVO_SLOW_SPEED);
  servoSet(SERVO_EYE_RIGHT, eyePos, SERVO_SLOW_SPEED);
  
  s_leftSpeed = 0;
  s_rightSpeed = 0;
}

// ============================================================
//  Think State (pause and reflect)
// ============================================================

#define THINK_DURATION_MS  2000

static void updateThinkState(uint32_t now) {
  if (now - s_context.stateStartTime < THINK_DURATION_MS) {
    s_leftSpeed = 0;
    s_rightSpeed = 0;
    uint8_t tilt = random(45, 55);
    servoSet(SERVO_NECK_TOP, tilt, SERVO_SLOW_SPEED);
  } else {
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_EXPLORE_LOOP, now);
    }
  }
}

// ============================================================
//  Investigate Height State
// ============================================================

static void updateInvestigateState(uint32_t now) {
  if (now > s_context.nextActionTime) {
    servoSet(SERVO_NECK_TOP, 50, SERVO_FAST_SPEED);  // Tilt neutral (center)
    emotionTriggerInvestigateEnd();
    
    if (s_context.waypointMode && waypointIsNavigating()) {
      transitionToState(AUTO_NAVIGATE_WAYPOINT, now);
    } else {
      transitionToState(AUTO_APPROACH, now);
    }
  }
}

// ============================================================
//  Main Update Function
// ============================================================

void autonomyUpdate(uint32_t now, int8_t* outLeftSpeed, int8_t* outRightSpeed) {
  // Default to stopped
  s_leftSpeed = 0;
  s_rightSpeed = 0;
  
  if (!s_enabled) {
    updateIdleBehaviours(now);
    *outLeftSpeed = 0;
    *outRightSpeed = 0;
    return;
  }
  
  if (s_manualOverride) {
    if (s_context.state != AUTO_IDLE) {
      transitionToState(AUTO_IDLE, now);
    }
    *outLeftSpeed = 0;
    *outRightSpeed = 0;
    return;
  }
  
  // Update sensor data
  float dist = sonarGetDistanceCm();
  s_context.detectedDistance = dist;
  s_context.objectDetected = sonarIsObjectDetected(DETECT_INTEREST_CM);
  
  // Update location from sensors
  s_location.heading = compassGetHeading();
  s_location.compassValid = compassIsValid();
  s_location.latitude = gpsGetLatitude();
  s_location.longitude = gpsGetLongitude();
  s_location.gpsValid = gpsHasFix();
  
  // Object heading for interest/memory (body + neck)
  float objectHeading = s_location.heading + s_neckAngle;
  while (objectHeading < 0) objectHeading += 360.0f;
  while (objectHeading >= 360.0f) objectHeading -= 360.0f;
  
  // Behavioral engines
  interestUpdate(now, s_context.objectDetected, dist, objectHeading);
  
  const BatteryData& bat = batteryGetData();
  float batteryPct = bat.valid ? (float)bat.percent : 100.0f;
  emotionUpdate(now, batteryPct, s_context.objectDetected, interestGetLevel());
  
  memoryUpdate(now);
  
  // Return Home: if active, let RTH engine drive motors
  if (returnHomeIsActive()) {
    returnHomeUpdate(now, s_location.latitude, s_location.longitude,
                     s_location.heading, dist, batteryPct);
    returnHomeGetMotorCommands(&s_leftSpeed, &s_rightSpeed);
    *outLeftSpeed = s_leftSpeed;
    *outRightSpeed = s_rightSpeed;
    updateIdleBehaviours(now);
    return;
  }
  
  // Auto-trigger return home on low battery
  if (returnHomeShouldTrigger(batteryPct) && memoryIsHomeSet()) {
    double hLat, hLon;
    memoryGetHome(&hLat, &hLon);
    returnHomeStart(hLat, hLon, batteryPct);
    transitionToState(AUTO_RETURN_HOME, now);
    emotionTriggerReturnHome();
  }
  
  // Update IMU data
  const ImuData& imu = imuGetData();
  s_location.pitch = imu.pitch;
  s_location.roll = imu.roll;
  s_location.imuValid = imu.valid && isIMUCalibrated();
  
  // Safety: check IMU tilt
  if (s_location.imuValid && imu.tiltAlert) {
    Serial.println(F("[Autonomy] ⚠️  Excessive tilt detected - stopping"));
    autonomyEmergencyStop();
    *outLeftSpeed = 0;
    *outRightSpeed = 0;
    return;
  }
  
  // Safety checks
  if (sonarIsValid()) s_lastSonarValidMs = now;
  if (compassIsValid()) s_lastCompassValidMs = now;
  
  if (now - s_lastSonarValidMs > SONAR_TIMEOUT_MS) {
    Serial.println(F("[Autonomy] ⚠️  Sonar timeout - stopping"));
    autonomyEmergencyStop();
    *outLeftSpeed = 0;
    *outRightSpeed = 0;
    return;
  }
  
  // Emergency obstacle detection
  if (sonarIsObjectClose(DETECT_CLOSE_CM) && 
      s_context.state != AUTO_AVOID && s_context.state != AUTO_REACT) {
    Serial.printf("[Autonomy] ⚠️  Obstacle: %.1fcm\n", dist);
    transitionToState(AUTO_REACT, now);
  }
  
  // Idle behaviours
  updateIdleBehaviours(now);
  
  // State machine
  switch (s_context.state) {
    case AUTO_IDLE:
      break;
      
    case AUTO_SCAN:
      updateScanState(now);
      break;
      
    case AUTO_TRACK_OBJECT:
      updateTrackObjectState(now);
      break;
      
    case AUTO_EVALUATE:
      updateEvaluateState(now);
      break;
      
    case AUTO_APPROACH:
      updateApproachState(now);
      break;
      
    case AUTO_INVESTIGATE_HEIGHT:
      updateInvestigateState(now);
      break;
      
    case AUTO_REACT:
      updateReactState(now);
      break;
      
    case AUTO_WANDER:
      transitionToState(AUTO_EXPLORE_LOOP, now);
      break;
      
    case AUTO_AVOID:
      updateAvoidance(now);
      break;
      
    case AUTO_ORIENT:
      updateOrientation(now);
      break;
      
    case AUTO_THINK:
      updateThinkState(now);
      break;
      
    case AUTO_RETURN_HOME:
      // Motor commands already set by returnHomeGetMotorCommands above when active
      break;
      
    case AUTO_EXPLORE_LOOP:
      updateExploration(now);
      break;
      
    case AUTO_NAVIGATE_WAYPOINT:
      updateWaypointNavigation(now);
      break;
  }
  
  // Output motor commands
  *outLeftSpeed = s_leftSpeed;
  *outRightSpeed = s_rightSpeed;
}

// ============================================================
//  Initialization
// ============================================================

void autonomyInit() {
  s_context.state = AUTO_IDLE;
  s_context.stateStartTime = millis();
  s_context.targetHeading = 0.0f;
  s_context.detectedDistance = 0.0f;
  s_context.detectedHeightScore = 0.0f;
  s_context.objectDetected = false;
  s_context.investigating = false;
  s_context.scanMinDistance = DETECT_FAR_CM;
  s_context.scanBestHeading = 0.0f;
  s_context.waypointMode = false;
  
  // Initialize location
  s_location.latitude = 0.0;
  s_location.longitude = 0.0;
  s_location.heading = 0.0f;
  s_location.pitch = 0.0f;
  s_location.roll = 0.0f;
  s_location.gpsValid = false;
  s_location.compassValid = false;
  s_location.imuValid = false;
  
  Serial.println(F("[Autonomy] Engine initialized"));
}

// ============================================================
//  Public API
// ============================================================

void autonomySetEnabled(bool enabled) {
  if (enabled != s_enabled) {
    s_enabled = enabled;
    Serial.printf("[Autonomy] %s\n", enabled ? "ENABLED" : "DISABLED");
    
    if (enabled) {
      if (s_context.waypointMode && waypointIsNavigating()) {
        transitionToState(AUTO_NAVIGATE_WAYPOINT, millis());
      } else {
        transitionToState(AUTO_EXPLORE_LOOP, millis());
        s_nextHeadingChangeMs = millis() + random(EXPLORE_HEADING_CHANGE_MIN, EXPLORE_HEADING_CHANGE_MAX);
      }
    } else {
      transitionToState(AUTO_IDLE, millis());
    }
  }
}

bool autonomyIsEnabled() {
  return s_enabled;
}

AutoState autonomyGetState() {
  return s_context.state;
}

const char* autonomyGetStateName() {
  return getStateName(s_context.state);
}

const AutoContext* autonomyGetContext() {
  return &s_context;
}

const LocationState* autonomyGetLocation() {
  return &s_location;
}

void autonomyEmergencyStop() {
  s_enabled = false;
  transitionToState(AUTO_IDLE, millis());
  s_leftSpeed = 0;
  s_rightSpeed = 0;
  Serial.println(F("[Autonomy] EMERGENCY STOP"));
}

void autonomySetManualOverride(bool active) {
  if (active && !s_manualOverride) {
    Serial.println(F("[Autonomy] Manual override - autonomy paused"));
  }
  s_manualOverride = active;
}

void autonomySetWaypointMode(bool enabled) {
  s_context.waypointMode = enabled;
  Serial.printf("[Autonomy] Waypoint mode: %s\n", enabled ? "ON" : "OFF");
  
  if (enabled && s_enabled) {
    transitionToState(AUTO_NAVIGATE_WAYPOINT, millis());
  }
}

bool autonomyIsWaypointMode() {
  return s_context.waypointMode;
}
