// ============================================================
//  WALL-E Return Home Engine Implementation
//  GPS-based navigation with obstacle avoidance
// ============================================================

#include "return_home_engine.h"
#include "personality_engine.h"
#include <math.h>

// ============================================================
//  Internal State
// ============================================================

static ReturnHomeContext s_rth = {
  RTH_IDLE,              // state
  0,                     // startTimeMs
  false,                 // hasHome
  0.0,                   // homeLat
  0.0,                   // homeLon
  0.0,                   // currentLat
  0.0,                   // currentLon
  0.0,                   // distanceToHome
  0.0,                   // bearingToHome
  0,                     // lastProgressTimeMs
  999999.0f,             // lastDistanceToHome
  0,                     // avoidStartTimeMs
  0.0f                   // urgencyMultiplier
};

static const char* RTH_STATE_NAMES[] = {
  "IDLE",
  "ORIENTING",
  "NAVIGATING",
  "AVOIDING",
  "ARRIVED",
  "FAILED"
};

// ============================================================
//  Helper: Haversine Distance
// ============================================================

static float calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  const float R = 6371000.0f; // Earth radius in meters
  
  float dLat = (lat2 - lat1) * DEG_TO_RAD;
  float dLon = (lon2 - lon1) * DEG_TO_RAD;
  
  float a = sin(dLat/2) * sin(dLat/2) +
            cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) *
            sin(dLon/2) * sin(dLon/2);
  
  float c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

// ============================================================
//  Helper: Calculate Bearing
// ============================================================

static float calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  float dLon = (lon2 - lon1) * DEG_TO_RAD;
  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  
  float y = sin(dLon) * cos(lat2);
  float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  float bearing = atan2(y, x) * RAD_TO_DEG;
  
  // Normalize to 0-360
  if (bearing < 0) bearing += 360.0f;
  return bearing;
}

// ============================================================
//  Helper: Angle Difference
// ============================================================

static float angleDifference(float angle1, float angle2) {
  float diff = angle2 - angle1;
  while (diff > 180.0f) diff -= 360.0f;
  while (diff < -180.0f) diff += 360.0f;
  return diff;
}

// ============================================================
//  Initialization
// ============================================================

void returnHomeInit() {
  Serial.println("[RTH] Initializing...");
  memset(&s_rth, 0, sizeof(ReturnHomeContext));
  s_rth.state = RTH_IDLE;
  Serial.println("[RTH] Ready");
}

// ============================================================
//  Update
// ============================================================

void returnHomeUpdate(uint32_t now, double currentLat, double currentLon, 
                      float currentHeading, float sonarDistance, float batteryPercent) {
  
  if (s_rth.state == RTH_IDLE || s_rth.state == RTH_ARRIVED || s_rth.state == RTH_FAILED) {
    return;  // Not active
  }
  
  // Update current state
  s_rth.currentHeading = currentHeading;
  s_rth.obstacleDistance = sonarDistance;
  s_rth.obstacleDetected = (sonarDistance > 0 && sonarDistance < 40.0f);  // 40cm threshold
  
  // Calculate distance and heading to home
  s_rth.distanceToHome = calculateDistance(currentLat, currentLon,
                                           s_rth.homeLatitude, s_rth.homeLongitude);
  s_rth.headingToHome = calculateBearing(currentLat, currentLon,
                                         s_rth.homeLatitude, s_rth.homeLongitude);
  
  // Check for arrival
  if (s_rth.distanceToHome < RTH_ARRIVAL_RADIUS_M) {
    s_rth.state = RTH_ARRIVED;
    Serial.printf("[RTH] ARRIVED! (%.1fm from home)\n", s_rth.distanceToHome);
    return;
  }
  
  // Check for stuck condition
  if (abs(s_rth.distanceToHome - s_rth.lastDistance) < 0.5f) {
    if (now - s_rth.lastProgressTime > 10000) {  // 10 seconds without progress
      s_rth.stuckCount++;
      if (s_rth.stuckCount > 3) {
        s_rth.state = RTH_FAILED;
        Serial.println("[RTH] FAILED - stuck for too long");
        return;
      }
    }
  } else {
    s_rth.lastProgressTime = now;
    s_rth.lastDistance = s_rth.distanceToHome;
    s_rth.stuckCount = 0;
  }
  
  // State machine
  switch (s_rth.state) {
    case RTH_ORIENTING: {
      // Rotate to face home
      float headingError = angleDifference(s_rth.currentHeading, s_rth.headingToHome);
      
      if (abs(headingError) < RTH_HEADING_TOLERANCE) {
        // Aligned - start navigating
        s_rth.state = RTH_NAVIGATING;
        s_rth.navigationStartTime = now;
        Serial.println("[RTH] Oriented - starting navigation");
      }
      break;
    }
    
    case RTH_NAVIGATING: {
      // Check for obstacles
      if (s_rth.obstacleDetected) {
        s_rth.state = RTH_AVOIDING;
        s_rth.avoidanceStartTime = now;
        Serial.println("[RTH] Obstacle detected - avoiding");
        break;
      }
      
      // Check if heading has drifted
      float headingError = angleDifference(s_rth.currentHeading, s_rth.headingToHome);
      if (abs(headingError) > 30.0f) {
        // Need to reorient
        s_rth.state = RTH_ORIENTING;
        Serial.println("[RTH] Heading drift - reorienting");
      }
      break;
    }
    
    case RTH_AVOIDING: {
      // Avoid obstacle for max 5 seconds, then reorient
      if (now - s_rth.avoidanceStartTime > 5000 || !s_rth.obstacleDetected) {
        s_rth.state = RTH_ORIENTING;
        Serial.println("[RTH] Avoidance complete - reorienting");
      }
      break;
    }
    
    default:
      break;
  }
}

// ============================================================
//  Control
// ============================================================

void returnHomeStart(double homeLat, double homeLon, float batteryPercent) {
  Serial.printf("[RTH] Starting return home (battery: %.1f%%)\n", batteryPercent);
  
  s_rth.homeLatitude = homeLat;
  s_rth.homeLongitude = homeLon;
  s_rth.state = RTH_ORIENTING;
  s_rth.stateStartTime = millis();
  s_rth.batteryAtStart = batteryPercent;
  s_rth.navigationStartTime = millis();
  s_rth.lastProgressTime = millis();
  s_rth.stuckCount = 0;
  
  Serial.printf("[RTH] Home: %.6f, %.6f\n", homeLat, homeLon);
}

void returnHomeCancel() {
  if (s_rth.state != RTH_IDLE) {
    Serial.println("[RTH] Cancelled");
    s_rth.state = RTH_IDLE;
  }
}

bool returnHomeIsActive() {
  return (s_rth.state != RTH_IDLE && 
          s_rth.state != RTH_ARRIVED && 
          s_rth.state != RTH_FAILED);
}

bool returnHomeHasArrived() {
  return s_rth.state == RTH_ARRIVED;
}

// ============================================================
//  State
// ============================================================

RTHState returnHomeGetState() {
  return s_rth.state;
}

const char* returnHomeGetStateName() {
  return RTH_STATE_NAMES[s_rth.state];
}

// ============================================================
//  Navigation Data
// ============================================================

float returnHomeGetDistance() {
  return s_rth.distanceToHome;
}

float returnHomeGetHeading() {
  return s_rth.headingToHome;
}

float returnHomeGetProgress() {
  if (s_rth.initialDistance == 0) return 0.0f;
  
  float progress = 1.0f - (s_rth.distanceToHome / s_rth.initialDistance);
  return constrain(progress, 0.0f, 1.0f);
}

// ============================================================
//  Motor Commands
// ============================================================

void returnHomeGetMotorCommands(int8_t* leftSpeed, int8_t* rightSpeed) {
  float baseSpeed = 60;  // Base speed (0-100)
  
  // Adjust speed based on distance (slow down when close)
  if (s_rth.distanceToHome < RTH_SLOW_RADIUS_M) {
    baseSpeed *= (s_rth.distanceToHome / RTH_SLOW_RADIUS_M);
    baseSpeed = max(baseSpeed, 20.0f);  // Minimum 20%
  }
  
  // Adjust speed based on battery urgency
  float urgency = returnHomeGetUrgency(s_rth.batteryAtStart);
  baseSpeed *= (0.7f + urgency * 0.3f);  // 70% - 100% based on urgency
  
  switch (s_rth.state) {
    case RTH_ORIENTING: {
      // Rotate toward home
      float headingError = angleDifference(s_rth.currentHeading, s_rth.headingToHome);
      
      if (headingError > 0) {
        // Turn right
        *leftSpeed = 40;
        *rightSpeed = -40;
      } else {
        // Turn left
        *leftSpeed = -40;
        *rightSpeed = 40;
      }
      break;
    }
    
    case RTH_NAVIGATING: {
      // Move forward with slight heading correction
      float headingError = angleDifference(s_rth.currentHeading, s_rth.headingToHome);
      float correction = constrain(headingError * 0.5f, -20.0f, 20.0f);
      
      *leftSpeed = (int8_t)(baseSpeed - correction);
      *rightSpeed = (int8_t)(baseSpeed + correction);
      break;
    }
    
    case RTH_AVOIDING: {
      // Back up and turn
      *leftSpeed = -30;
      *rightSpeed = 30;  // Back and turn right
      break;
    }
    
    default:
      *leftSpeed = 0;
      *rightSpeed = 0;
      break;
  }
  
  // Clamp to valid range
  *leftSpeed = constrain(*leftSpeed, -100, 100);
  *rightSpeed = constrain(*rightSpeed, -100, 100);
}

// ============================================================
//  Auto-trigger
// ============================================================

bool returnHomeShouldTrigger(float batteryPercent) {
  return batteryPercent < RTH_MIN_BATTERY_PERCENT;
}

float returnHomeGetUrgency(float batteryPercent) {
  // 0.0 (above threshold) - 1.0 (critical battery)
  if (batteryPercent >= RTH_MIN_BATTERY_PERCENT) {
    return 0.0f;
  }
  
  float urgency = 1.0f - ((batteryPercent - RTH_CRITICAL_BATTERY) / 
                          (RTH_MIN_BATTERY_PERCENT - RTH_CRITICAL_BATTERY));
  return constrain(urgency, 0.0f, 1.0f);
}

// ============================================================
//  Debug
// ============================================================

void returnHomePrintDebug() {
  Serial.printf("[RTH] State: %s\n", RTH_STATE_NAMES[s_rth.state]);
  Serial.printf("  Distance: %.1fm\n", s_rth.distanceToHome);
  Serial.printf("  Heading: %.0f° (current: %.0f°)\n", s_rth.headingToHome, s_rth.currentHeading);
  Serial.printf("  Progress: %.1f%%\n", returnHomeGetProgress() * 100.0f);
  Serial.printf("  Obstacle: %s (%.1fcm)\n", 
                s_rth.obstacleDetected ? "YES" : "NO",
                s_rth.obstacleDistance);
  Serial.printf("  Elapsed: %lums\n", returnHomeGetElapsedTime());
}

uint32_t returnHomeGetElapsedTime() {
  return millis() - s_rth.navigationStartTime;
}
