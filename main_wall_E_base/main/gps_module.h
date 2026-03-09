// ============================================================
//  WALL-E Master Controller — GPS Module
//  TinyGPS++ integration for location awareness
// ============================================================

#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <TinyGPS++.h>

// GPS serial configuration (adjust for your hardware)
#define GPS_SERIAL_RX  16   // GPIO for RX
#define GPS_SERIAL_TX  17   // GPIO for TX
#define GPS_BAUD_RATE  9600

// Update rate
#define GPS_UPDATE_MS  100   // Process GPS data every 100ms

// Initialize GPS
bool gpsInit();

// Update GPS (call every loop)
void gpsUpdate(uint32_t now);

// Get current location
double gpsGetLatitude();
double gpsGetLongitude();
float gpsGetAltitude();

// Get movement data
float gpsGetSpeed();        // km/h
float gpsGetCourse();       // degrees from north

// Get satellite/quality info
uint32_t gpsSatellites();
uint32_t gpsHdop();

// Check if GPS has valid fix
bool gpsHasFix();

// Get age of data (ms since last update)
uint32_t gpsGetAge();

// Get TinyGPS++ object for advanced use
TinyGPSPlus* gpsGetObject();

// Feed current position to memory engine (call when has fix)
void gpsFeedToMemory(uint32_t now);

#endif // GPS_MODULE_H
