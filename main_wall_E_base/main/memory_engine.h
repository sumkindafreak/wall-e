#pragma once

// ============================================================
//  WALL-E Memory Engine
//  Spatial memory and object familiarity system
//  Prevents repetitive behavior and creates learned preferences
// ============================================================

#include <Arduino.h>

// ============================================================
//  Configuration
// ============================================================

#define MEMORY_GPS_HISTORY_SIZE 10      // Last 10 GPS positions
#define MEMORY_HEADING_BINS 36          // 10-degree bins (360/10)
#define MEMORY_ZONE_RADIUS_M 5.0f       // 5 meter radius for "same zone"
#define MEMORY_FAMILIARITY_DECAY 0.95f  // Decay per update (exponential)

// ============================================================
//  GPS Position Memory
// ============================================================

struct GPSPosition {
  double latitude;
  double longitude;
  uint32_t timestamp;
  bool valid;
};

struct GPSHistory {
  GPSPosition positions[MEMORY_GPS_HISTORY_SIZE];
  uint8_t writeIndex;   // Circular buffer write pointer
  uint8_t count;        // Number of valid entries
};

// ============================================================
//  Direction Familiarity (360° divided into bins)
// ============================================================

struct DirectionMemory {
  float familiarityScore[MEMORY_HEADING_BINS];  // 0.0 (new) - 1.0 (very familiar)
  uint32_t lastSeenTime[MEMORY_HEADING_BINS];   // Timestamp of last detection
  uint32_t detectionCount[MEMORY_HEADING_BINS]; // Total detections per direction
};

// ============================================================
//  Object Memory
// ============================================================

struct ObjectMemory {
  float lastDistance;
  float lastHeading;
  uint32_t lastSeenTime;
  uint32_t totalDetections;
  float familiarityScore;  // 0.0 - 1.0
};

// ============================================================
//  Complete Memory State
// ============================================================

struct MemoryState {
  GPSHistory gpsHistory;
  DirectionMemory directions;
  ObjectMemory currentObject;
  GPSPosition homePosition;  // Saved base location
  bool homeSet;
};

// ============================================================
//  API
// ============================================================

void memoryInit();
void memoryUpdate(uint32_t now);

// GPS History
void memoryAddGPSPosition(double lat, double lon, uint32_t timestamp);
bool memoryIsInRecentZone(double lat, double lon);
float memoryGetZoneFamiliarity(double lat, double lon);
const GPSPosition* memoryGetGPSHistory(uint8_t* count);

// Direction/Heading Memory
void memoryRecordHeading(float heading, uint32_t now);
float memoryGetHeadingFamiliarity(float heading);
uint32_t memoryGetHeadingDetections(float heading);
float memoryGetLeastFamiliarHeading();  // Returns heading with lowest familiarity

// Object Memory
void memoryUpdateObject(float distance, float heading, uint32_t now);
float memoryGetObjectFamiliarity();
bool memoryIsObjectNovel();  // True if object is new/unfamiliar

// Home Position
void memorySetHome(double lat, double lon);
bool memoryGetHome(double* lat, double* lon);
bool memoryIsHomeSet();
float memoryGetDistanceToHome(double currentLat, double currentLon);
float memoryGetHeadingToHome(double currentLat, double currentLon);

// Curiosity Modifiers
float memoryGetCuriosityMultiplier(float heading);  // 0.5x (familiar) - 1.5x (novel)
float memoryGetZoneCuriosityMultiplier(double lat, double lon);

// Persistence
void memorySave();
void memoryLoad();
void memoryClear();

// Debug
void memoryPrintDebug();
void memoryPrintGPSHistory();
void memoryPrintDirectionMemory();
