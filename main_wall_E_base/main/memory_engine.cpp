// ============================================================
//  WALL-E Memory Engine Implementation
//  Spatial memory, GPS history, and familiarity learning
// ============================================================

#include "memory_engine.h"
#include <Preferences.h>
#include <math.h>

// ============================================================
//  Internal State
// ============================================================

static MemoryState s_memory = {0};
static Preferences s_prefs;

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
//  Helper: Heading to Bin Index
// ============================================================

static uint8_t headingToBin(float heading) {
  int bin = (int)(heading / 10.0f);  // 10-degree bins
  return constrain(bin, 0, MEMORY_HEADING_BINS - 1);
}

// ============================================================
//  Initialization
// ============================================================

void memoryInit() {
  Serial.println("[Memory] Initializing...");
  
  // Clear GPS history
  memset(&s_memory.gpsHistory, 0, sizeof(GPSHistory));
  
  // Clear direction memory
  memset(&s_memory.directions, 0, sizeof(DirectionMemory));
  
  // Clear object memory
  memset(&s_memory.currentObject, 0, sizeof(ObjectMemory));
  
  // Try to load home position from Preferences
  s_prefs.begin("memory", true);  // Read-only
  if (s_prefs.isKey("homeLat")) {
    s_memory.homePosition.latitude = s_prefs.getDouble("homeLat", 0.0);
    s_memory.homePosition.longitude = s_prefs.getDouble("homeLon", 0.0);
    s_memory.homeSet = true;
    Serial.printf("[Memory] Home loaded: %.6f, %.6f\n", 
                  s_memory.homePosition.latitude,
                  s_memory.homePosition.longitude);
  } else {
    s_memory.homeSet = false;
    Serial.println("[Memory] No home position saved");
  }
  s_prefs.end();
  
  Serial.println("[Memory] Ready");
}

void memoryUpdate(uint32_t now) {
  // Apply exponential decay to familiarity scores
  for (int i = 0; i < MEMORY_HEADING_BINS; i++) {
    s_memory.directions.familiarityScore[i] *= MEMORY_FAMILIARITY_DECAY;
  }
  
  // Decay object familiarity over time
  uint32_t timeSinceLastSeen = now - s_memory.currentObject.lastSeenTime;
  if (timeSinceLastSeen > 60000) {  // 1 minute
    s_memory.currentObject.familiarityScore *= 0.99f;  // Slow decay
  }
}

// ============================================================
//  GPS History
// ============================================================

void memoryAddGPSPosition(double lat, double lon, uint32_t timestamp) {
  GPSHistory* hist = &s_memory.gpsHistory;
  
  // Add to circular buffer
  hist->positions[hist->writeIndex].latitude = lat;
  hist->positions[hist->writeIndex].longitude = lon;
  hist->positions[hist->writeIndex].timestamp = timestamp;
  hist->positions[hist->writeIndex].valid = true;
  
  hist->writeIndex = (hist->writeIndex + 1) % MEMORY_GPS_HISTORY_SIZE;
  if (hist->count < MEMORY_GPS_HISTORY_SIZE) {
    hist->count++;
  }
  
  Serial.printf("[Memory] GPS added: %.6f, %.6f (history: %d)\n", lat, lon, hist->count);
}

bool memoryIsInRecentZone(double lat, double lon) {
  GPSHistory* hist = &s_memory.gpsHistory;
  
  for (int i = 0; i < hist->count; i++) {
    if (hist->positions[i].valid) {
      float dist = calculateDistance(lat, lon,
                                    hist->positions[i].latitude,
                                    hist->positions[i].longitude);
      if (dist < MEMORY_ZONE_RADIUS_M) {
        return true;
      }
    }
  }
  return false;
}

float memoryGetZoneFamiliarity(double lat, double lon) {
  int matchCount = 0;
  GPSHistory* hist = &s_memory.gpsHistory;
  
  for (int i = 0; i < hist->count; i++) {
    if (hist->positions[i].valid) {
      float dist = calculateDistance(lat, lon,
                                    hist->positions[i].latitude,
                                    hist->positions[i].longitude);
      if (dist < MEMORY_ZONE_RADIUS_M * 2) {  // Wider radius for familiarity
        matchCount++;
      }
    }
  }
  
  // More matches = more familiar (0.0 - 1.0)
  return (float)matchCount / (float)MEMORY_GPS_HISTORY_SIZE;
}

const GPSPosition* memoryGetGPSHistory(uint8_t* count) {
  *count = s_memory.gpsHistory.count;
  return s_memory.gpsHistory.positions;
}

// ============================================================
//  Direction/Heading Memory
// ============================================================

void memoryRecordHeading(float heading, uint32_t now) {
  uint8_t bin = headingToBin(heading);
  
  // Increase familiarity for this direction
  s_memory.directions.familiarityScore[bin] += 0.1f;
  if (s_memory.directions.familiarityScore[bin] > 1.0f) {
    s_memory.directions.familiarityScore[bin] = 1.0f;
  }
  
  s_memory.directions.lastSeenTime[bin] = now;
  s_memory.directions.detectionCount[bin]++;
}

float memoryGetHeadingFamiliarity(float heading) {
  uint8_t bin = headingToBin(heading);
  return s_memory.directions.familiarityScore[bin];
}

uint32_t memoryGetHeadingDetections(float heading) {
  uint8_t bin = headingToBin(heading);
  return s_memory.directions.detectionCount[bin];
}

float memoryGetLeastFamiliarHeading() {
  float minFamiliarity = 1.0f;
  uint8_t minBin = 0;
  
  for (int i = 0; i < MEMORY_HEADING_BINS; i++) {
    if (s_memory.directions.familiarityScore[i] < minFamiliarity) {
      minFamiliarity = s_memory.directions.familiarityScore[i];
      minBin = i;
    }
  }
  
  // Return center of bin
  return (minBin * 10.0f) + 5.0f;
}

// ============================================================
//  Object Memory
// ============================================================

void memoryUpdateObject(float distance, float heading, uint32_t now) {
  ObjectMemory* obj = &s_memory.currentObject;
  
  // Check if this is similar to last object (within 20cm and 30 degrees)
  bool isSimilar = (abs(distance - obj->lastDistance) < 20.0f) &&
                   (abs(heading - obj->lastHeading) < 30.0f);
  
  if (isSimilar) {
    // Same object - increase familiarity
    obj->familiarityScore += 0.05f;
    if (obj->familiarityScore > 1.0f) {
      obj->familiarityScore = 1.0f;
    }
    obj->totalDetections++;
  } else {
    // New object - reset familiarity
    obj->familiarityScore = 0.0f;
    obj->totalDetections = 1;
  }
  
  obj->lastDistance = distance;
  obj->lastHeading = heading;
  obj->lastSeenTime = now;
  
  // Also record in direction memory
  memoryRecordHeading(heading, now);
}

float memoryGetObjectFamiliarity() {
  return s_memory.currentObject.familiarityScore;
}

bool memoryIsObjectNovel() {
  return s_memory.currentObject.familiarityScore < 0.3f;
}

// ============================================================
//  Home Position
// ============================================================

void memorySetHome(double lat, double lon) {
  s_memory.homePosition.latitude = lat;
  s_memory.homePosition.longitude = lon;
  s_memory.homePosition.valid = true;
  s_memory.homeSet = true;
  
  Serial.printf("[Memory] Home set: %.6f, %.6f\n", lat, lon);
}

bool memoryGetHome(double* lat, double* lon) {
  if (s_memory.homeSet) {
    *lat = s_memory.homePosition.latitude;
    *lon = s_memory.homePosition.longitude;
    return true;
  }
  return false;
}

bool memoryIsHomeSet() {
  return s_memory.homeSet;
}

float memoryGetDistanceToHome(double currentLat, double currentLon) {
  if (!s_memory.homeSet) return -1.0f;
  
  return calculateDistance(currentLat, currentLon,
                          s_memory.homePosition.latitude,
                          s_memory.homePosition.longitude);
}

float memoryGetHeadingToHome(double currentLat, double currentLon) {
  if (!s_memory.homeSet) return -1.0f;
  
  return calculateBearing(currentLat, currentLon,
                         s_memory.homePosition.latitude,
                         s_memory.homePosition.longitude);
}

// ============================================================
//  Curiosity Modifiers
// ============================================================

float memoryGetCuriosityMultiplier(float heading) {
  float familiarity = memoryGetHeadingFamiliarity(heading);
  
  // Unfamiliar directions are more interesting
  // Familiar: 0.5x curiosity, Novel: 1.5x curiosity
  return 1.5f - familiarity;
}

float memoryGetZoneCuriosityMultiplier(double lat, double lon) {
  float familiarity = memoryGetZoneFamiliarity(lat, lon);
  
  // Same logic: unfamiliar zones are more interesting
  return 1.5f - familiarity;
}

// ============================================================
//  Persistence
// ============================================================

void memorySave() {
  s_prefs.begin("memory", false);
  
  if (s_memory.homeSet) {
    s_prefs.putDouble("homeLat", s_memory.homePosition.latitude);
    s_prefs.putDouble("homeLon", s_memory.homePosition.longitude);
    Serial.println("[Memory] Home position saved");
  }
  
  s_prefs.end();
}

void memoryLoad() {
  memoryInit();  // Reload from Preferences
}

void memoryClear() {
  Serial.println("[Memory] Clearing all memory...");
  memset(&s_memory, 0, sizeof(MemoryState));
  
  s_prefs.begin("memory", false);
  s_prefs.clear();
  s_prefs.end();
  
  Serial.println("[Memory] Memory cleared");
}

// ============================================================
//  Debug
// ============================================================

void memoryPrintDebug() {
  Serial.println("[Memory] Current state:");
  Serial.printf("  GPS history: %d positions\n", s_memory.gpsHistory.count);
  Serial.printf("  Home set: %s\n", s_memory.homeSet ? "YES" : "NO");
  Serial.printf("  Object familiarity: %.2f\n", s_memory.currentObject.familiarityScore);
  Serial.printf("  Object detections: %lu\n", s_memory.currentObject.totalDetections);
}

void memoryPrintGPSHistory() {
  Serial.println("[Memory] GPS History:");
  GPSHistory* hist = &s_memory.gpsHistory;
  
  for (int i = 0; i < hist->count; i++) {
    int idx = (hist->writeIndex - hist->count + i + MEMORY_GPS_HISTORY_SIZE) % MEMORY_GPS_HISTORY_SIZE;
    if (hist->positions[idx].valid) {
      Serial.printf("  [%d] %.6f, %.6f @ %lu\n",
                    i,
                    hist->positions[idx].latitude,
                    hist->positions[idx].longitude,
                    hist->positions[idx].timestamp);
    }
  }
}

void memoryPrintDirectionMemory() {
  Serial.println("[Memory] Direction familiarity:");
  for (int i = 0; i < MEMORY_HEADING_BINS; i++) {
    if (s_memory.directions.familiarityScore[i] > 0.01f) {
      Serial.printf("  %d°: %.2f (%lu detections)\n",
                    i * 10,
                    s_memory.directions.familiarityScore[i],
                    s_memory.directions.detectionCount[i]);
    }
  }
  
  float leastFamiliar = memoryGetLeastFamiliarHeading();
  Serial.printf("  Least familiar direction: %.0f°\n", leastFamiliar);
}
