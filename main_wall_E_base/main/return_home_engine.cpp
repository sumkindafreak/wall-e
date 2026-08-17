// ============================================================
// WALL-E Return Home Engine Implementation
// GPS-based navigation with obstacle avoidance
// ============================================================

#include "return_home_engine.h"
#include "personality_engine.h"
#include <math.h>
#include <cstring>

// ============================================================
// Internal State
// ============================================================

// Zero-initialize by member type rather than relying on a positional list.
// The old initializer was written for an earlier ReturnHomeContext layout and
// silently shifted values into the wrong fields as the struct evolved.
static ReturnHomeContext s_rth = {};

static const char* RTH_STATE_NAMES[] = {
  "IDLE",
  "ORIENTING",
  "NAVIGATING",
  "AVOIDING",
  "ARRIVED",
  "FAILED"
};

// ============================================================
// Helper: Haversine Distance
// ============================================================

static float calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  const float R = 6371000.0f;
  const float dLat = (float)((lat2 - lat1) * DEG_TO_RAD);
  const float dLon = (float)((lon2 - lon1) * DEG_TO_RAD);

  const float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f) +
                  cosf((float)(lat1 * DEG_TO_RAD)) *
                      cosf((float)(lat2 * DEG_TO_RAD)) *
                      sinf(dLon / 2.0f) * sinf(dLon / 2.0f);
  const float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
  return R * c;
}

// ============================================================
// Helper: Calculate Bearing
// ============================================================

static float calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  const float lat1r = (float)(lat1 * DEG_TO_RAD);
  const float lat2r = (float)(lat2 * DEG_TO_RAD);
  const float dLon = (float)((lon2 - lon1) * DEG_TO_RAD);

  const float y = sinf(dLon) * cosf(lat2r);
  const float x = cosf(lat1r) * sinf(lat2r) -
                  sinf(lat1r) * cosf(lat2r) * cosf(dLon);
  float bearing = atan2f(y, x) * RAD_TO_DEG;
  if (bearing < 0.0f) bearing += 360.0f;
  return bearing;
}

static float angleDifference(float angle1, float angle2) {
  float diff = angle2 - angle1;
  while (diff > 180.0f) diff -= 360.0f;
  while (diff < -180.0f) diff += 360.0f;
  return diff;
}

void returnHomeInit() {
  Serial.println("[RTH] Initializing...");
  memset(&s_rth, 0, sizeof(s_rth));
  s_rth.state = RTH_IDLE;
  s_rth.lastDistance = 999999.0f;
  Serial.println("[RTH] Ready");
}

void returnHomeUpdate(uint32_t now, double currentLat, double currentLon,
                      float currentHeading, float sonarDistance, float batteryPercent) {
  (void)batteryPercent;

  if (s_rth.state == RTH_IDLE ||
      s_rth.state == RTH_ARRIVED ||
      s_rth.state == RTH_FAILED) {
    return;
  }

  s_rth.currentHeading = currentHeading;
  s_rth.obstacleDistance = sonarDistance;
  s_rth.obstacleDetected = (sonarDistance > 0.0f && sonarDistance < 40.0f);

  s_rth.distanceToHome = calculateDistance(currentLat, currentLon,
                                           s_rth.homeLatitude, s_rth.homeLongitude);
  s_rth.headingToHome = calculateBearing(currentLat, currentLon,
                                         s_rth.homeLatitude, s_rth.homeLongitude);

  // First valid navigation update establishes the progress baseline.
  if (s_rth.initialDistance <= 0.0f) {
    s_rth.initialDistance = s_rth.distanceToHome;
    s_rth.lastDistance = s_rth.distanceToHome;
    s_rth.lastProgressTime = now;
  }

  if (s_rth.distanceToHome < RTH_ARRIVAL_RADIUS_M) {
    s_rth.state = RTH_ARRIVED;
    Serial.printf("[RTH] ARRIVED! (%.1fm from home)\n", s_rth.distanceToHome);
    return;
  }

  if (fabsf(s_rth.distanceToHome - s_rth.lastDistance) < 0.5f) {
    if (now - s_rth.lastProgressTime > 10000u) {
      s_rth.stuckCount++;
      s_rth.lastProgressTime = now;
      if (s_rth.stuckCount > 3u) {
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

  switch (s_rth.state) {
    case RTH_ORIENTING: {
      const float headingError = angleDifference(s_rth.currentHeading,
                                                 s_rth.headingToHome);
      if (fabsf(headingError) < RTH_HEADING_TOLERANCE) {
        s_rth.state = RTH_NAVIGATING;
        s_rth.stateStartTime = now;
        Serial.println("[RTH] Oriented - starting navigation");
      }
      break;
    }

    case RTH_NAVIGATING: {
      if (s_rth.obstacleDetected) {
        s_rth.state = RTH_AVOIDING;
        s_rth.stateStartTime = now;
        s_rth.avoidanceStartTime = now;
        Serial.println("[RTH] Obstacle detected - avoiding");
        break;
      }

      const float headingError = angleDifference(s_rth.currentHeading,
                                                 s_rth.headingToHome);
      if (fabsf(headingError) > 30.0f) {
        s_rth.state = RTH_ORIENTING;
        s_rth.stateStartTime = now;
        Serial.println("[RTH] Heading drift - reorienting");
      }
      break;
    }

    case RTH_AVOIDING:
      if (now - s_rth.avoidanceStartTime > 5000u || !s_rth.obstacleDetected) {
        s_rth.state = RTH_ORIENTING;
        s_rth.stateStartTime = now;
        Serial.println("[RTH] Avoidance complete - reorienting");
      }
      break;

    default:
      break;
  }
}

void returnHomeStart(double homeLat, double homeLon, float batteryPercent) {
  Serial.printf("[RTH] Starting return home (battery: %.1f%%)\n", batteryPercent);

  s_rth.homeLatitude = homeLat;
  s_rth.homeLongitude = homeLon;
  s_rth.state = RTH_ORIENTING;
  s_rth.stateStartTime = millis();
  s_rth.batteryAtStart = batteryPercent;
  s_rth.navigationStartTime = millis();
  s_rth.lastProgressTime = millis();
  s_rth.initialDistance = 0.0f;
  s_rth.distanceToHome = 0.0f;
  s_rth.lastDistance = 999999.0f;
  s_rth.stuckCount = 0;
  s_rth.obstacleDetected = false;
  s_rth.avoidanceStartTime = 0;

  Serial.printf("[RTH] Home: %.6f, %.6f\n", homeLat, homeLon);
}

void returnHomeCancel() {
  if (s_rth.state != RTH_IDLE) {
    Serial.println("[RTH] Cancelled");
    s_rth.state = RTH_IDLE;
  }
}

bool returnHomeIsActive() {
  return s_rth.state != RTH_IDLE &&
         s_rth.state != RTH_ARRIVED &&
         s_rth.state != RTH_FAILED;
}

bool returnHomeHasArrived() {
  return s_rth.state == RTH_ARRIVED;
}

RTHState returnHomeGetState() {
  return s_rth.state;
}

const char* returnHomeGetStateName() {
  const int idx = (int)s_rth.state;
  if (idx < 0 || idx >= (int)(sizeof(RTH_STATE_NAMES) / sizeof(RTH_STATE_NAMES[0]))) {
    return "UNKNOWN";
  }
  return RTH_STATE_NAMES[idx];
}

float returnHomeGetDistance() {
  return s_rth.distanceToHome;
}

float returnHomeGetHeading() {
  return s_rth.headingToHome;
}

float returnHomeGetProgress() {
  if (s_rth.initialDistance <= 0.0f) return 0.0f;
  const float progress = 1.0f - (s_rth.distanceToHome / s_rth.initialDistance);
  return constrain(progress, 0.0f, 1.0f);
}

void returnHomeGetMotorCommands(int8_t* leftSpeed, int8_t* rightSpeed) {
  if (!leftSpeed || !rightSpeed) return;

  float baseSpeed = 60.0f;
  if (s_rth.distanceToHome < RTH_SLOW_RADIUS_M) {
    baseSpeed *= (s_rth.distanceToHome / RTH_SLOW_RADIUS_M);
    baseSpeed = max(baseSpeed, 20.0f);
  }

  const float urgency = returnHomeGetUrgency(s_rth.batteryAtStart);
  baseSpeed *= (0.7f + urgency * 0.3f);

  switch (s_rth.state) {
    case RTH_ORIENTING: {
      const float headingError = angleDifference(s_rth.currentHeading,
                                                 s_rth.headingToHome);
      if (headingError > 0.0f) {
        *leftSpeed = 40;
        *rightSpeed = -40;
      } else {
        *leftSpeed = -40;
        *rightSpeed = 40;
      }
      break;
    }

    case RTH_NAVIGATING: {
      const float headingError = angleDifference(s_rth.currentHeading,
                                                 s_rth.headingToHome);
      const float correction = constrain(headingError * 0.5f, -20.0f, 20.0f);
      *leftSpeed = (int8_t)(baseSpeed - correction);
      *rightSpeed = (int8_t)(baseSpeed + correction);
      break;
    }

    case RTH_AVOIDING:
      *leftSpeed = -30;
      *rightSpeed = 30;
      break;

    default:
      *leftSpeed = 0;
      *rightSpeed = 0;
      break;
  }

  *leftSpeed = (int8_t)constrain((int)*leftSpeed, -100, 100);
  *rightSpeed = (int8_t)constrain((int)*rightSpeed, -100, 100);
}

bool returnHomeShouldTrigger(float batteryPercent) {
  return batteryPercent < RTH_MIN_BATTERY_PERCENT;
}

float returnHomeGetUrgency(float batteryPercent) {
  if (batteryPercent >= RTH_MIN_BATTERY_PERCENT) return 0.0f;

  const float urgency =
      1.0f - ((batteryPercent - RTH_CRITICAL_BATTERY) /
              (RTH_MIN_BATTERY_PERCENT - RTH_CRITICAL_BATTERY));
  return constrain(urgency, 0.0f, 1.0f);
}

void returnHomePrintDebug() {
  Serial.printf("[RTH] State: %s\n", returnHomeGetStateName());
  Serial.printf("  Distance: %.1fm\n", s_rth.distanceToHome);
  Serial.printf("  Heading: %.0f deg (current: %.0f deg)\n",
                s_rth.headingToHome, s_rth.currentHeading);
  Serial.printf("  Progress: %.1f%%\n", returnHomeGetProgress() * 100.0f);
  Serial.printf("  Obstacle: %s (%.1fcm)\n",
                s_rth.obstacleDetected ? "YES" : "NO",
                s_rth.obstacleDistance);
  Serial.printf("  Elapsed: %lums\n", (unsigned long)returnHomeGetElapsedTime());
}

uint32_t returnHomeGetElapsedTime() {
  if (s_rth.navigationStartTime == 0) return 0;
  return millis() - s_rth.navigationStartTime;
}
