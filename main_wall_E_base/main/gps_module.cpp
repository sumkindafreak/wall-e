// ============================================================
// WALL-E Base — GPS implementation
// ============================================================

#include "gps_module.h"
#include "memory_engine.h"
#include <HardwareSerial.h>

static HardwareSerial GPSSerial(2);
static TinyGPSPlus s_gps;

static bool s_initialized = false;
static uint32_t s_lastUpdateMs = 0;
static uint32_t s_lastFixMs = 0;

bool gpsInit() {
  // A TX pin of -1 is valid for the P4 NMEA receive-only wiring.
  GPSSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);
  s_initialized = true;
  Serial.printf("[GPS] UART2 RX=%d TX=%d baud=%lu\n",
                GPS_SERIAL_RX,
                GPS_SERIAL_TX,
                (unsigned long)GPS_BAUD_RATE);
  return true;
}

void gpsUpdate(uint32_t now) {
  if (!s_initialized) return;
  if (now - s_lastUpdateMs < GPS_UPDATE_MS) return;
  s_lastUpdateMs = now;

  while (GPSSerial.available() > 0) {
    const char c = (char)GPSSerial.read();
    s_gps.encode(c);
  }

  if (s_gps.location.isValid()) s_lastFixMs = now;

  static uint32_t lastDebug = 0;
  if (now - lastDebug > 5000u) {
    if (s_gps.location.isValid()) {
      Serial.printf("[GPS] Fix: %.6f,%.6f Alt:%.1fm Sats:%lu HDOP:%lu\n",
                    s_gps.location.lat(),
                    s_gps.location.lng(),
                    s_gps.altitude.meters(),
                    (unsigned long)s_gps.satellites.value(),
                    (unsigned long)s_gps.hdop.value());
    } else {
      Serial.printf("[GPS] No fix - Sats:%lu Chars:%lu Sentences:%lu Failed:%lu\n",
                    (unsigned long)s_gps.satellites.value(),
                    (unsigned long)s_gps.charsProcessed(),
                    (unsigned long)s_gps.sentencesWithFix(),
                    (unsigned long)s_gps.failedChecksum());
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
  return s_gps.location.isValid() && s_gps.location.age() < 2000u;
}

uint32_t gpsGetAge() {
  return s_gps.location.age();
}

TinyGPSPlus* gpsGetObject() {
  return &s_gps;
}

#define GPS_MEMORY_FEED_INTERVAL_MS 5000u

void gpsFeedToMemory(uint32_t now) {
  if (!s_gps.location.isValid() || s_gps.location.age() > 2000u) return;

  static uint32_t s_lastFeedMs = 0;
  if (now - s_lastFeedMs < GPS_MEMORY_FEED_INTERVAL_MS) return;
  s_lastFeedMs = now;

  memoryAddGPSPosition(s_gps.location.lat(), s_gps.location.lng(), now);
}
