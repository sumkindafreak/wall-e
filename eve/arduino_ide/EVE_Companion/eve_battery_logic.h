/**
 * EVE battery evaluation — plain C++ (no Arduino), unit-tested on a PC.
 *
 * Input: one raw sample per poll (pack volts + amps, each with an "I read it OK" flag).
 * Output: a filtered, hysteresis-protected view that the rest of EVE can trust:
 *   - valid=false for anything implausible; an invalid reading is never turned into "empty" or
 *     "full". After a few invalid samples in a row the level becomes UNKNOWN.
 *   - charging / full are only reported when the current direction is configured
 *     (chargeSign != 0). Otherwise charging = -1 (unknown) and full = false.
 *
 * The percentage is a straight line between minV and maxV of the configured pack (2S Li-ion
 * 6.0-8.4 V in config.h). It is a rough guide, not a fuel gauge.
 */
#pragma once

#include <stdint.h>

#include "eve_control_core.h" /* EveBatLevel */

typedef struct {
  float minV;           /* 0 %  */
  float maxV;           /* 100 % */
  float plausibleMinV;  /* readings outside [plausibleMinV, plausibleMaxV] are invalid */
  float plausibleMaxV;
  int warnPct;
  int critPct;
  int hystPct;          /* recovery needs this much above a threshold */
  int8_t chargeSign;    /* sign of the current while charging: -1, +1, or 0 = unknown */
  float chargeDetectA;  /* |A| in the charge direction that counts as charging */
  float fullTaperA;     /* charge current below this (while still charging) near maxV = full */
  float alpha;          /* EMA weight of a new sample, 0 < alpha <= 1 */
  uint8_t invalidToUnknown; /* consecutive invalid samples before the level becomes UNKNOWN */
} EveBatteryConfig;

typedef struct {
  bool everValid;
  bool valid;          /* latest sample plausible */
  float vFilt;         /* filtered pack volts (last valid) */
  float aFilt;         /* filtered amps (last valid); 0 when current unavailable */
  bool currentValid;
  int pct;
  EveBatLevel level;
  int8_t charging;     /* -1 unknown, 0 no, 1 yes */
  bool full;
  uint8_t invalidRun;  /* consecutive invalid samples */
  uint32_t invalidTotal;
  const char* why;     /* reason for the latest invalid sample ("ok" when valid) */
} EveBatteryState;

void eveBatteryLogicInit(EveBatteryState* st);
/**
 * Feed one sample. vOk/aOk = the sensor read itself succeeded (I2C ack, no overflow...).
 * Returns true when level, charging or full changed (worth logging / reporting).
 */
bool eveBatteryLogicUpdate(EveBatteryState* st, const EveBatteryConfig* cfg, float v, bool vOk, float a, bool aOk);
/** 0..100 from volts using the configured pack range (clamped). */
int eveBatteryLogicPercent(const EveBatteryConfig* cfg, float v);
