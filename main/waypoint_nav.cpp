// ============================================================
//  WALL-E Waypoint Navigation Implementation
// ============================================================

#include "waypoint_nav.h"
#include <Preferences.h>
#include <math.h>

// Earth radius in meters
#define EARTH_RADIUS_M  6371000.0f

// Waypoint storage
static Waypoint s_waypoints[MAX_WAYPOINTS];
static uint8_t s_waypointCount = 0;
static NavState s_navState;
static Preferences s_prefs;

// ============================================================
//  Haversine Distance Calculation
// ============================================================

float waypointCalculateDistance(double lat1, double lon1, double lat2, double lon2) {
  // Convert to radians
  double lat1R = lat1 * M_PI / 180.0;
  double lon1R = lon1 * M_PI / 180.0;
  double lat2R = lat2 * M_PI / 180.0;
  double lon2R = lon2 * M_PI / 180.0;
  
  // Haversine formula
  double dLat = lat2R - lat1R;
  double dLon = lon2R - lon1R;
  
  double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
             cos(lat1R) * cos(lat2R) *
             sin(dLon / 2.0) * sin(dLon / 2.0);
  
  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  
  return (float)(EARTH_RADIUS_M * c);
}

// ============================================================
//  Bearing Calculation
// ============================================================

float waypointCalculateBearing(double lat1, double lon1, double lat2, double lon2) {
  // Convert to radians
  double lat1R = lat1 * M_PI / 180.0;
  double lon1R = lon1 * M_PI / 180.0;
  double lat2R = lat2 * M_PI / 180.0;
  double lon2R = lon2 * M_PI / 180.0;
  
  double dLon = lon2R - lon1R;
  
  double y = sin(dLon) * cos(lat2R);
  double x = cos(lat1R) * sin(lat2R) -
             sin(lat1R) * cos(lat2R) * cos(dLon);
  
  double bearing = atan2(y, x) * 180.0 / M_PI;
  
  // Normalize to 0-360
  if (bearing < 0) bearing += 360.0;
  
  return (float)bearing;
}

// ============================================================
//  Initialization
// ============================================================

void waypointInit() {
  // Clear waypoints
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    s_waypoints[i].active = false;
    s_waypoints[i].latitude = 0.0;
    s_waypoints[i].longitude = 0.0;
    s_waypoints[i].name[0] = '\0';
  }
  
  s_waypointCount = 0;
  
  // Initialize nav state
  s_navState.currentWaypointIndex = 0;
  s_navState.distanceToWaypoint = 0.0f;
  s_navState.bearingToWaypoint = 0.0f;
  s_navState.waypointActive = false;
  s_navState.navigationActive = false;
  
  // Load saved waypoints
  waypointLoad();
  
  Serial.println(F("[Waypoint] Initialized"));
}

// ============================================================
//  Waypoint Management
// ============================================================

bool waypointAdd(double lat, double lon, const char* name) {
  if (s_waypointCount >= MAX_WAYPOINTS) {
    Serial.println(F("[Waypoint] ⚠️  Waypoint list full"));
    return false;
  }
  
  // Find first inactive slot
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    if (!s_waypoints[i].active) {
      s_waypoints[i].latitude = lat;
      s_waypoints[i].longitude = lon;
      s_waypoints[i].active = true;
      strncpy(s_waypoints[i].name, name ? name : "Waypoint", 15);
      s_waypoints[i].name[15] = '\0';
      
      s_waypointCount++;
      Serial.printf("[Waypoint] Added #%d: %s (%.6f,%.6f)\n", i, s_waypoints[i].name, lat, lon);
      return true;
    }
  }
  
  return false;
}

bool waypointRemove(uint8_t index) {
  if (index >= MAX_WAYPOINTS || !s_waypoints[index].active) {
    return false;
  }
  
  s_waypoints[index].active = false;
  s_waypointCount--;
  Serial.printf("[Waypoint] Removed #%d\n", index);
  return true;
}

void waypointClearAll() {
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    s_waypoints[i].active = false;
  }
  s_waypointCount = 0;
  Serial.println(F("[Waypoint] Cleared all"));
}

const Waypoint* waypointGet(uint8_t index) {
  if (index >= MAX_WAYPOINTS) return nullptr;
  return &s_waypoints[index];
}

uint8_t waypointGetCount() {
  return s_waypointCount;
}

void waypointSetCurrent(uint8_t index) {
  if (index < MAX_WAYPOINTS && s_waypoints[index].active) {
    s_navState.currentWaypointIndex = index;
    s_navState.waypointActive = true;
    Serial.printf("[Waypoint] Target set to #%d: %s\n", index, s_waypoints[index].name);
  }
}

uint8_t waypointGetCurrent() {
  return s_navState.currentWaypointIndex;
}

// ============================================================
//  Navigation Update
// ============================================================

void waypointUpdate(double currentLat, double currentLon, float currentHeading) {
  if (!s_navState.waypointActive || !s_navState.navigationActive) {
    return;
  }
  
  uint8_t idx = s_navState.currentWaypointIndex;
  if (idx >= MAX_WAYPOINTS || !s_waypoints[idx].active) {
    s_navState.waypointActive = false;
    return;
  }
  
  // Calculate distance and bearing to waypoint
  s_navState.distanceToWaypoint = waypointCalculateDistance(
    currentLat, currentLon,
    s_waypoints[idx].latitude, s_waypoints[idx].longitude
  );
  
  s_navState.bearingToWaypoint = waypointCalculateBearing(
    currentLat, currentLon,
    s_waypoints[idx].latitude, s_waypoints[idx].longitude
  );
  
  // Check if arrived
  if (s_navState.distanceToWaypoint < WAYPOINT_ARRIVAL_RADIUS) {
    Serial.printf("[Waypoint] Arrived at #%d: %s\n", idx, s_waypoints[idx].name);
    // Auto-advance to next waypoint
    waypointNext();
  }
  
  // Debug output (rate-limited)
  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.printf("[Waypoint] Distance: %.1fm Bearing: %.0f° Heading: %.0f°\n",
      s_navState.distanceToWaypoint, s_navState.bearingToWaypoint, currentHeading);
    lastDebug = millis();
  }
}

const NavState* waypointGetNavState() {
  return &s_navState;
}

void waypointStartNavigation() {
  if (s_waypointCount > 0) {
    s_navState.navigationActive = true;
    if (!s_navState.waypointActive) {
      // Start with first active waypoint
      for (int i = 0; i < MAX_WAYPOINTS; i++) {
        if (s_waypoints[i].active) {
          waypointSetCurrent(i);
          break;
        }
      }
    }
    Serial.println(F("[Waypoint] Navigation started"));
  } else {
    Serial.println(F("[Waypoint] ⚠️  No waypoints to navigate"));
  }
}

void waypointStopNavigation() {
  s_navState.navigationActive = false;
  Serial.println(F("[Waypoint] Navigation stopped"));
}

bool waypointIsNavigating() {
  return s_navState.navigationActive;
}

bool waypointIsArrived() {
  return (s_navState.distanceToWaypoint < WAYPOINT_ARRIVAL_RADIUS);
}

void waypointNext() {
  // Find next active waypoint
  uint8_t startIdx = s_navState.currentWaypointIndex + 1;
  for (uint8_t i = 0; i < MAX_WAYPOINTS; i++) {
    uint8_t idx = (startIdx + i) % MAX_WAYPOINTS;
    if (s_waypoints[idx].active) {
      waypointSetCurrent(idx);
      return;
    }
  }
  
  // No more waypoints - stop navigation
  Serial.println(F("[Waypoint] All waypoints reached - stopping"));
  waypointStopNavigation();
}

// ============================================================
//  Persistence (Preferences)
// ============================================================

void waypointSave() {
  s_prefs.begin("waypoints", false);
  
  // Save count
  s_prefs.putUChar("count", s_waypointCount);
  
  // Save each waypoint
  for (int i = 0; i < MAX_WAYPOINTS; i++) {
    if (s_waypoints[i].active) {
      char keyLat[16], keyLon[16], keyName[16];
      snprintf(keyLat, sizeof(keyLat), "lat%d", i);
      snprintf(keyLon, sizeof(keyLon), "lon%d", i);
      snprintf(keyName, sizeof(keyName), "name%d", i);
      
      s_prefs.putDouble(keyLat, s_waypoints[i].latitude);
      s_prefs.putDouble(keyLon, s_waypoints[i].longitude);
      s_prefs.putString(keyName, s_waypoints[i].name);
    }
  }
  
  s_prefs.end();
  Serial.printf("[Waypoint] Saved %d waypoints to flash\n", s_waypointCount);
}

void waypointLoad() {
  s_prefs.begin("waypoints", true);  // Read-only
  
  uint8_t count = s_prefs.getUChar("count", 0);
  
  if (count > 0) {
    for (int i = 0; i < MAX_WAYPOINTS; i++) {
      char keyLat[16], keyLon[16], keyName[16];
      snprintf(keyLat, sizeof(keyLat), "lat%d", i);
      snprintf(keyLon, sizeof(keyLon), "lon%d", i);
      snprintf(keyName, sizeof(keyName), "name%d", i);
      
      if (s_prefs.isKey(keyLat)) {
        s_waypoints[i].latitude = s_prefs.getDouble(keyLat, 0.0);
        s_waypoints[i].longitude = s_prefs.getDouble(keyLon, 0.0);
        String name = s_prefs.getString(keyName, "Waypoint");
        strncpy(s_waypoints[i].name, name.c_str(), 15);
        s_waypoints[i].name[15] = '\0';
        s_waypoints[i].active = true;
        s_waypointCount++;
      }
    }
    Serial.printf("[Waypoint] Loaded %d waypoints from flash\n", s_waypointCount);
  }
  
  s_prefs.end();
}
