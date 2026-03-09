// ============================================================
//  WALL-E Master Controller — GPS Implementation
// ============================================================

#include "gps_module.h"
#include "memory_engine.h"
#include <HardwareSerial.h>

// GPS serial port
static HardwareSerial GPSSerial(2);  // Use UART2

// TinyGPS++ object
static TinyGPSPlus s_gps;

// State
static bool s_initialized = false;
static uint32_t s_lastUpdateMs = 0;
static uint32_t s_lastFixMs = 0;

bool gpsInit() {
  GPSSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);
  s_initialized = true;
  Serial.printf("[GPS] Initialized on UART2 (RX=%d, TX=%d, baud=%d)\n", 
    GPS_SERIAL_RX, GPS_SERIAL_TX, GPS_BAUD_RATE);
  return true;
}

void gpsUpdate(uint32_t now) {
  if (!s_initialized) return;
  
  // Rate limit
  if (now - s_lastUpdateMs < GPS_UPDATE_MS) {
    return;
  }
  s_lastUpdateMs = now;
  
  // Process available GPS data
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    s_gps.encode(c);
  }
  
  // Update fix timestamp
  if (s_gps.location.isValid()) {
    s_lastFixMs = now;
  }
  
  // Debug output every 5 seconds
  static uint32_t lastDebug = 0;
  if (now - lastDebug > 5000) {
    if (s_gps.location.isValid()) {
      Serial.printf("[GPS] Fix: %.6f,%.6f Alt:%.1fm Sats:%d HDOP:%d\n",
        s_gps.location.lat(), s_gps.location.lng(),
        s_gps.altitude.meters(), s_gps.satellites.value(), s_gps.hdop.value());
    } else {
      Serial.printf("[GPS] No fix - Sats:%d Chars:%d Sentences:%d Failed:%d\n",
        s_gps.satellites.value(), s_gps.charsProcessed(),
        s_gps.sentencesWithFix(), s_gps.failedChecksum());
    }
    lastDebug = now;
  }
}

double gpsGetLatitude() {
  return s_gps.location.isValid() ? s_gps.location.lat() : 0.0;
}

double gpsGetLongitude() {
  return s_gps.location.isValid() ? s_gps.location.lng() : 0.0;
}

float gpsGetAltitude() {
  return s_gps.altitude.isValid() ? s_gps.altitude.meters() : 0.0f;
}

float gpsGetSpeed() {
  return s_gps.speed.isValid() ? s_gps.speed.kmph() : 0.0f;
}

float gpsGetCourse() {
  return s_gps.course.isValid() ? s_gps.course.deg() : 0.0f;
}

uint32_t gpsSatellites() {
  return s_gps.satellites.value();
}

uint32_t gpsHdop() {
  return s_gps.hdop.value();
}

bool gpsHasFix() {
  return s_gps.location.isValid() && s_gps.location.age() < 2000;
}

uint32_t gpsGetAge() {
  return s_gps.location.age();
}

TinyGPSPlus* gpsGetObject() {
  return &s_gps;
}

#define GPS_MEMORY_FEED_INTERVAL_MS  5000  // Add to memory every 5s when moving

void gpsFeedToMemory(uint32_t now) {
  if (!s_gps.location.isValid() || s_gps.location.age() > 2000) return;
  
  static uint32_t s_lastFeedMs = 0;
  if (now - s_lastFeedMs < GPS_MEMORY_FEED_INTERVAL_MS) return;
  s_lastFeedMs = now;
  
  memoryAddGPSPosition(s_gps.location.lat(), s_gps.location.lng(), now);
}
