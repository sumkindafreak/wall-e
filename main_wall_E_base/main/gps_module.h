// ============================================================
// WALL-E Base — GPS module
// TinyGPS++ integration for location awareness
// ============================================================

#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "base_board_pins.h"

#define GPS_SERIAL_RX   BASE_PIN_GPS_RX
#define GPS_SERIAL_TX   BASE_PIN_GPS_TX
#define GPS_BAUD_RATE   9600

#define GPS_UPDATE_MS   100

bool gpsInit();
void gpsUpdate(uint32_t now);

double gpsGetLatitude();
double gpsGetLongitude();
float gpsGetAltitude();

float gpsGetSpeed();
float gpsGetCourse();

uint32_t gpsSatellites();
uint32_t gpsHdop();

bool gpsHasFix();
uint32_t gpsGetAge();

TinyGPSPlus* gpsGetObject();
void gpsFeedToMemory(uint32_t now);

#endif // GPS_MODULE_H
