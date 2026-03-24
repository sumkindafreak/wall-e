/*
 * FastLED 30x4 matrix layer for paludarium_4relay.
 * Day/night from NTP time; no LDR.
 */
#ifndef LED_FASTLED_H
#define LED_FASTLED_H

#include <stdint.h>

#if defined(LED_ENABLED) && (LED_ENABLED) == 1

enum LedMode {
  LED_OFF,
  LED_MANUAL,
  LED_AUTO
};

void ledBegin(void);
void ledSetBrightness(uint8_t b);
void ledSetMode(uint8_t mode);
void ledSetColor(uint8_t r, uint8_t g, uint8_t b);
/* hour/minute in local time; timeSynced = NTP has set time; dayStartMin/dayEndMin = minutes from midnight (e.g. 360, 1200) */
void ledUpdate(uint8_t hour, uint8_t minute, bool timeSynced, uint16_t dayStartMin, uint16_t dayEndMin);
/* Map matrix (x,y) to strip index; x=0..MATRIX_WIDTH-1, y=0..MATRIX_HEIGHT-1. Returns -1 if out of range. */
int ledXY(int x, int y);
uint8_t ledGetBrightness(void);
uint8_t ledGetMode(void);

#else

static inline void ledBegin(void) {}
static inline void ledSetBrightness(uint8_t) {}
static inline void ledSetMode(uint8_t) {}
static inline void ledSetColor(uint8_t, uint8_t, uint8_t) {}
static inline void ledUpdate(uint8_t, uint8_t, bool, uint16_t, uint16_t) {}
static inline int ledXY(int, int) { return -1; }
static inline uint8_t ledGetBrightness(void) { return 0; }
static inline uint8_t ledGetMode(void) { return 0; }

#endif

#endif /* LED_FASTLED_H */
