// ============================================================
//  WALL-E Master Controller — Autonomous Behaviour Engine
//  Full character autonomy with exploration, curiosity, and life
// ============================================================

#include "autonomy_engine.h"
#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
#include "motion_engine.h"
#include "protocol.h"
#include <Arduino.h>

// ============================================================
//  Internal State
// ============================================================

static bool s_enabled = false;
static bool s_joystickOverride = false;
static AutoContext s_context;
static LocationState s_location;
static Personality s_personality;

// Neck scan state
static float s_neckAngle = 0.0f;
static float s_neckTargetAngle = 0.0f;
static bool s_scanningLeft = true;
static uint32_t s_lastScanStepMs = 0;

// Exploration state
static float s_exploreTargetHeading = 0.0f;
static uint32_t s_nextHeadingChangeMs = 0;

// Idle behaviour timers
static uint32_t s_nextIdleActionMs = 0;
static uint32_t s_nextBlinkMs = 0;

// Safety
static uint32_t s_lastSonarValidMs = 0;
static uint32_t s_lastCompassValidMs = 0;

// ============================================================
//  State Name Lookup
// ============================================================

static const char* getStateName(AutoState state) {
  switch (state) {
    case AUTO_IDLE:              return "IDLE";
    case AUTO_SCAN:              return "SCAN";
    case AUTO_EVALUATE:          return "EVALUATE";
    case AUTO_APPROACH:          return "APPROACH";
    case AUTO_INVESTIGATE_HEIGHT: return "INVESTIGATE";
    case AUTO_REACT:             return "REACT";
    case AUTO_WANDER:            return "WANDER";
    case AUTO_AVOID:             return "AVOID";
    case AUTO_ORIENT:            return "ORIENT";
    case AUTO_EXPLORE_LOOP:      return "EXPLORE";
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
//  Neck Control for Scanning
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
  
  // Apply to motion engine (neck servo is index 1: Pan)
  // Convert -60° to +60° → 0-180° servo range
  uint8_t servoPos = (uint8_t)constrain(90 + s_neckAngle, 0, 180);
  motionSetServoDirect(1, servoPos);  // Servo 1 = Pan
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
  
  // Map level to tilt servo offset
  // Level 1-3 = look down (servo < 90)
  // Level 4-7 = look straight (servo ~90)
  // Level 8-10 = look up (servo > 90)
  uint8_t tiltPos = 90;
  if (s_context.investigationLevel <= 3) {
    tiltPos = map(s_context.investigationLevel, 1, 3, 60, 85);
  } else if (s_context.investigationLevel >= 8) {
    tiltPos = map(s_context.investigationLevel, 8, 10, 95, 120);
  }
  
  // Apply tilt (servo 0 = Tilt)
  motionSetServoDirect(0, tiltPos);
  
  // Raise eyebrows (curiosity)
  if (s_personality.curiosityLevel > 0.5f) {
    motionTriggerAnimation(5);  // Surprised animation (eyebrows up)
  }
}

// ============================================================
//  Idle Life Behaviours
// ============================================================

static void updateIdleBehaviours(uint32_t now) {
  // Micro head sway
  if (now > s_nextIdleActionMs) {
    // Random action based on personality
    int action = random(100);
    
    if (action < (int)(s_personality.randomness * 20)) {
      // Random head tilt
      uint8_t tilt = random(70, 110);
      motionSetServoDirect(0, tilt);
      Serial.println(F("[Autonomy] Idle: head tilt"));
    } else if (action < (int)(s_personality.randomness * 35)) {
      // Random neck pan
      uint8_t pan = random(75, 105);
      motionSetServoDirect(1, pan);
      Serial.println(F("[Autonomy] Idle: neck pan"));
    } else if (action < (int)(s_personality.curiosityLevel * 45)) {
      // Eyebrow raise
      motionTriggerAnimation(random(0, 2) ? 3 : 4);
      Serial.println(F("[Autonomy] Idle: eyebrow"));
    }
    
    // Next idle action in 3-8 seconds
    s_nextIdleActionMs = now + random(3000, 8000);
  }
}

// ============================================================
//  Exploration Logic
// ============================================================

static void updateExploration(DriveState* ds, uint32_t now) {
  // Check if it's time to change heading
  if (now > s_nextHeadingChangeMs) {
    // Pick random new heading
    s_exploreTargetHeading = random(0, 360);
    s_nextHeadingChangeMs = now + random(EXPLORE_HEADING_CHANGE_MIN, EXPLORE_HEADING_CHANGE_MAX);
    
    Serial.printf("[Autonomy] New target heading: %.0f°\n", s_exploreTargetHeading);
    transitionToState(AUTO_ORIENT, now);
    return;
  }
  
  // Random spontaneous actions
  int chance = random(100);
  if (chance < EXPLORE_PAUSE_CHANCE) {
    // Pause and look around
    ds->leftSpeed = 0;
    ds->rightSpeed = 0;
    transitionToState(AUTO_SCAN, now);
    Serial.println(F("[Autonomy] Pausing to scan"));
    return;
  } else if (chance < (EXPLORE_PAUSE_CHANCE + EXPLORE_SPIN_CHANCE)) {
    // Spin 180°
    s_exploreTargetHeading = s_location.heading + 180.0f;
    if (s_exploreTargetHeading >= 360.0f) s_exploreTargetHeading -= 360.0f;
    transitionToState(AUTO_ORIENT, now);
    Serial.println(F("[Autonomy] Spontaneous spin!"));
    return;
  }
  
  // Move forward slowly based on energy level
  float speed = s_personality.energyLevel * 50.0f;  // 0-50% speed
  ds->leftSpeed = (int8_t)speed;
  ds->rightSpeed = (int8_t)speed;
}

// ============================================================
//  Obstacle Avoidance
// ============================================================

static void updateAvoidance(DriveState* ds, uint32_t now) {
  // Reverse slightly
  ds->leftSpeed = -30;
  ds->rightSpeed = -30;
  
  // After 1 second, pick new direction
  if (now - s_context.stateStartTime > 1000) {
    // Turn away from obstacle
    // Pick random direction away from current heading
    float turnAmount = random(60, 120);
    if (random(0, 2) == 0) turnAmount = -turnAmount;
    
    s_exploreTargetHeading = s_location.heading + turnAmount;
    if (s_exploreTargetHeading < 0) s_exploreTargetHeading += 360.0f;
    if (s_exploreTargetHeading >= 360.0f) s_exploreTargetHeading -= 360.0f;
    
    transitionToState(AUTO_ORIENT, now);
  }
}

// ============================================================
//  Orientation (Turn to Heading)
// ============================================================

static void updateOrientation(DriveState* ds, uint32_t now) {
  if (!s_location.compassValid) {
    // Can't orient without compass - go back to wander
    transitionToState(AUTO_EXPLORE_LOOP, now);
    return;
  }
  
  // Calculate heading error
  float error = s_exploreTargetHeading - s_location.heading;
  
  // Normalize error to -180 to +180
  if (error > 180.0f) error -= 360.0f;
  if (error < -180.0f) error += 360.0f;
  
  // If close enough, done
  if (fabs(error) < 5.0f) {
    ds->leftSpeed = 0;
    ds->rightSpeed = 0;
    transitionToState(AUTO_EXPLORE_LOOP, now);
    Serial.println(F("[Autonomy] Orientation complete"));
    return;
  }
  
  // Turn in place
  int8_t turnSpeed = (int8_t)constrain(error * 0.5f, -40.0f, 40.0f);
  ds->leftSpeed = turnSpeed;
  ds->rightSpeed = -turnSpeed;
}

// ============================================================
//  Scanning State
// ============================================================

static void updateScanState(uint32_t now) {
  // Update neck sweep
  updateNeckScan(now);
  
  // Capture sonar readings
  float dist = sonarGetDistanceCm();
  if (sonarIsValid()) {
    // Track minimum distance
    if (dist < s_context.scanMinDistance) {
      s_context.scanMinDistance = dist;
      s_context.scanBestHeading = s_neckAngle;
    }
  }
  
  // Complete scan after full sweep (left to right and back)
  if (!s_scanningLeft && s_neckAngle >= 0) {
    // Scan complete - evaluate
    Serial.printf("[Autonomy] Scan complete - min dist: %.1fcm at angle %.0f°\n",
      s_context.scanMinDistance, s_context.scanBestHeading);
    
    transitionToState(AUTO_EVALUATE, now);
  }
}

// ============================================================
//  Evaluation State
// ============================================================

static void updateEvaluateState(DriveState* ds, uint32_t now) {
  // Decide what to do based on scan results
  if (s_context.scanMinDistance < DETECT_CLOSE_CM) {
    // Too close - avoid
    transitionToState(AUTO_AVOID, now);
  } else if (s_context.scanMinDistance < DETECT_INTEREST_CM) {
    // Interesting - investigate
    s_context.objectDetected = true;
    
    // Point neck toward object
    uint8_t neckPos = (uint8_t)constrain(90 + s_context.scanBestHeading, 0, 180);
    motionSetServoDirect(1, neckPos);
    
    // Start height investigation if curious
    if (random(100) < (int)(s_personality.curiosityLevel * 100)) {
      transitionToState(AUTO_INVESTIGATE_HEIGHT, now);
    } else {
      transitionToState(AUTO_APPROACH, now);
    }
  } else {
    // Nothing interesting - continue exploring
    transitionToState(AUTO_EXPLORE_LOOP, now);
  }
  
  // Reset scan state
  s_context.scanMinDistance = DETECT_FAR_CM;
}

// ============================================================
//  Approach State
// ============================================================

static void updateApproachState(DriveState* ds, uint32_t now) {
  float dist = sonarGetDistanceCm();
  
  if (!sonarIsValid() || dist > DETECT_INTEREST_CM) {
    // Lost object - go back to scanning
    transitionToState(AUTO_SCAN, now);
    return;
  }
  
  if (dist < (DETECT_CLOSE_CM * s_personality.braveryLevel)) {
    // Close enough (based on bravery) - react
    transitionToState(AUTO_REACT, now);
    return;
  }
  
  // Move toward object slowly
  float speed = 30.0f * s_personality.energyLevel;
  ds->leftSpeed = (int8_t)speed;
  ds->rightSpeed = (int8_t)speed;
}

// ============================================================
//  React State
// ============================================================

static void updateReactState(DriveState* ds, uint32_t now) {
  // Reaction animation/behaviour
  if (now - s_context.stateStartTime < REACT_DURATION_MS) {
    // React with animation
    if (now - s_context.stateStartTime < 500) {
      // Raise eyebrows
      motionTriggerAnimation(5);  // Surprised
    }
    
    // Stop moving
    ds->leftSpeed = 0;
    ds->rightSpeed = 0;
  } else {
    // Reaction complete - back away
    transitionToState(AUTO_AVOID, now);
  }
}

// ============================================================
//  Investigate Height State
// ============================================================

static void updateInvestigateState(uint32_t now) {
  if (now > s_context.nextActionTime) {
    // Investigation complete
    Serial.println(F("[Autonomy] Investigation complete"));
    
    // Return to neutral position
    motionSetServoDirect(0, 90);  // Tilt neutral
    
    transitionToState(AUTO_APPROACH, now);
  }
}

// ============================================================
//  Main Update Function
// ============================================================

void autonomyUpdate(uint32_t now) {
  if (!s_enabled) {
    // Idle behaviours even when not exploring
    updateIdleBehaviours(now);
    return;
  }
  
  if (s_joystickOverride) {
    // Joystick active - autonomy disabled
    if (s_context.state != AUTO_IDLE) {
      transitionToState(AUTO_IDLE, now);
    }
    return;
  }
  
  // Update sensor data
  float dist = sonarGetDistanceCm();
  s_context.detectedDistance = dist;
  s_context.objectDetected = (sonarIsValid() && dist < DETECT_INTEREST_CM);
  
  // Update location
  s_location.heading = compassGetHeading();
  s_location.compassValid = compassIsValid();
  s_location.latitude = gpsGetLatitude();
  s_location.longitude = gpsGetLongitude();
  s_location.gpsValid = gpsHasFix();
  
  // Safety checks
  if (sonarIsValid()) s_lastSonarValidMs = now;
  if (compassIsValid()) s_lastCompassValidMs = now;
  
  // Check for sensor failures
  if (now - s_lastSonarValidMs > SONAR_TIMEOUT_MS) {
    Serial.println(F("[Autonomy] ⚠️  Sonar timeout - stopping"));
    autonomyEmergencyStop();
    return;
  }
  
  // Emergency obstacle detection (override all states)
  if (sonarIsValid() && dist < DETECT_CLOSE_CM && s_context.state != AUTO_AVOID && s_context.state != AUTO_REACT) {
    Serial.printf("[Autonomy] ⚠️  Obstacle detected: %.1fcm\n", dist);
    transitionToState(AUTO_REACT, now);
  }
  
  // Idle behaviours (always active)
  updateIdleBehaviours(now);
  
  // State machine (modifies drive state)
  DriveState ds = {0, 0, false};
  
  switch (s_context.state) {
    case AUTO_IDLE:
      // Do nothing - waiting for enable
      break;
      
    case AUTO_SCAN:
      updateScanState(now);
      ds.leftSpeed = 0;
      ds.rightSpeed = 0;
      break;
      
    case AUTO_EVALUATE:
      updateEvaluateState(&ds, now);
      break;
      
    case AUTO_APPROACH:
      updateApproachState(&ds, now);
      break;
      
    case AUTO_INVESTIGATE_HEIGHT:
      updateInvestigateState(now);
      ds.leftSpeed = 0;
      ds.rightSpeed = 0;
      break;
      
    case AUTO_REACT:
      updateReactState(&ds, now);
      break;
      
    case AUTO_WANDER:
      // Legacy state - redirect to explore
      transitionToState(AUTO_EXPLORE_LOOP, now);
      break;
      
    case AUTO_AVOID:
      updateAvoidance(&ds, now);
      break;
      
    case AUTO_ORIENT:
      updateOrientation(&ds, now);
      break;
      
    case AUTO_EXPLORE_LOOP:
      updateExploration(&ds, now);
      break;
  }
  
  // Apply drive state to motion engine (caller will send to Base)
  // Note: This is stored in context for the main loop to pick up
  // We don't directly control ESP-NOW here
}

// ============================================================
//  Initialization
// ============================================================

void autonomyInit() {
  // Initialize context
  s_context.state = AUTO_IDLE;
  s_context.stateStartTime = millis();
  s_context.targetHeading = 0.0f;
  s_context.detectedDistance = 0.0f;
  s_context.detectedHeightScore = 0.0f;
  s_context.objectDetected = false;
  s_context.investigating = false;
  s_context.scanMinDistance = DETECT_FAR_CM;
  s_context.scanBestHeading = 0.0f;
  
  // Default personality (balanced)
  s_personality.curiosityLevel = 0.7f;
  s_personality.braveryLevel = 0.5f;
  s_personality.energyLevel = 0.6f;
  s_personality.randomness = 0.5f;
  
  // Initialize location
  s_location.latitude = 0.0;
  s_location.longitude = 0.0;
  s_location.heading = 0.0f;
  s_location.gpsValid = false;
  s_location.compassValid = false;
  
  // Initialize sensors
  sonarInit();
  bool compassOk = compassInit();
  bool gpsOk = gpsInit();
  
  if (!compassOk) {
    Serial.println(F("[Autonomy] ⚠️  Compass unavailable - heading features disabled"));
  }
  
  Serial.println(F("[Autonomy] Initialized"));
}

// ============================================================
//  Public API
// ============================================================

void autonomySetEnabled(bool enabled) {
  if (enabled != s_enabled) {
    s_enabled = enabled;
    Serial.printf("[Autonomy] %s\n", enabled ? "ENABLED" : "DISABLED");
    
    if (enabled) {
      // Start exploration
      transitionToState(AUTO_EXPLORE_LOOP, millis());
      s_nextHeadingChangeMs = millis() + random(EXPLORE_HEADING_CHANGE_MIN, EXPLORE_HEADING_CHANGE_MAX);
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

const Personality* autonomyGetPersonality() {
  return &s_personality;
}

void autonomySetPersonality(const Personality* p) {
  if (p) {
    s_personality = *p;
    Serial.printf("[Autonomy] Personality: curiosity=%.2f bravery=%.2f energy=%.2f random=%.2f\n",
      p->curiosityLevel, p->braveryLevel, p->energyLevel, p->randomness);
  }
}

void autonomyEmergencyStop() {
  s_enabled = false;
  transitionToState(AUTO_IDLE, millis());
  Serial.println(F("[Autonomy] EMERGENCY STOP"));
}

void autonomySetJoystickOverride(bool active) {
  if (active && !s_joystickOverride) {
    Serial.println(F("[Autonomy] Joystick override - autonomy paused"));
  }
  s_joystickOverride = active;
}
