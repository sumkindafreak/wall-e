// ============================================================
//  WALL-E CYD Performance Console — Developer Console
//  Hidden debugging/monitoring interface
// ============================================================

#ifndef DEV_CONSOLE_H
#define DEV_CONSOLE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// ============================================================
//  Console Pages
// ============================================================

enum DevConsolePage {
  DEV_OVERVIEW,
  DEV_SERVO_GRAPH,
  DEV_PACKET_TIMING,
  DEV_MEMORY,
  DEV_SENSORS,
  DEV_SD_BROWSER,
  DEV_PAGE_COUNT
};

// ============================================================
//  Public API
// ============================================================

// Initialize developer console
void devConsoleInit();

// Check if console is unlocked
bool devConsoleIsUnlocked();

// Unlock console (requires specific touch pattern)
void devConsoleCheckUnlock(uint16_t x, uint16_t y, uint32_t holdTimeMs);

// Lock console
void devConsoleLock();

// Update console display
void devConsoleUpdate(uint32_t now);

// Draw console (call when page changed)
void devConsoleDraw(TFT_eSPI* tft);

// Navigate console pages
void devConsoleNextPage();
void devConsolePrevPage();

// Feed data for live monitoring
void devConsoleFeedServoData(const float servos[9]);
void devConsoleFeedPacketTiming(uint32_t sendIntervalUs, uint32_t loopTimeUs);
void devConsoleFeedSensorData(float sonar, float compass, bool gpsValid);

// Get current console page
DevConsolePage devConsoleGetPage();

#endif // DEV_CONSOLE_H
