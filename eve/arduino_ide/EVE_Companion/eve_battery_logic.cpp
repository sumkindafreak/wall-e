#include "eve_battery_logic.h"

#include <math.h>
#include <string.h>

void eveBatteryLogicInit(EveBatteryState* st) {
  if (!st) return;
  memset(st, 0, sizeof(*st));
  st->level = EVE_BATLVL_UNKNOWN;
  st->charging = -1;
  st->why = "no sample yet";
}

int eveBatteryLogicPercent(const EveBatteryConfig* cfg, float v) {
  if (!cfg || cfg->maxV <= cfg->minV) return 0;
  const float f = (v - cfg->minV) / (cfg->maxV - cfg->minV) * 100.0f;
  if (f <= 0.0f) return 0;
  if (f >= 100.0f) return 100;
  return (int)lroundf(f);
}

static EveBatLevel levelWithHysteresis(EveBatLevel prev, int pct, const EveBatteryConfig* c) {
  switch (prev) {
    case EVE_BATLVL_OK:
      if (pct <= c->critPct) return EVE_BATLVL_CRITICAL;
      if (pct <= c->warnPct) return EVE_BATLVL_WARN;
      return EVE_BATLVL_OK;
    case EVE_BATLVL_WARN:
      if (pct <= c->critPct) return EVE_BATLVL_CRITICAL;
      if (pct > c->warnPct + c->hystPct) return EVE_BATLVL_OK;
      return EVE_BATLVL_WARN;
    case EVE_BATLVL_CRITICAL:
      if (pct > c->warnPct + c->hystPct) return EVE_BATLVL_OK;
      if (pct > c->critPct + c->hystPct) return EVE_BATLVL_WARN;
      return EVE_BATLVL_CRITICAL;
    case EVE_BATLVL_UNKNOWN:
    default:
      if (pct <= c->critPct) return EVE_BATLVL_CRITICAL;
      if (pct <= c->warnPct) return EVE_BATLVL_WARN;
      return EVE_BATLVL_OK;
  }
}

bool eveBatteryLogicUpdate(EveBatteryState* st, const EveBatteryConfig* c, float v, bool vOk, float a, bool aOk) {
  if (!st || !c) return false;
  const EveBatLevel prevLevel = st->level;
  const int8_t prevChg = st->charging;
  const bool prevFull = st->full;

  // ---- 1. Is this voltage sample believable? --------------------------------------------------
  const char* why = nullptr;
  if (!vOk) {
    why = "sensor read failed";
  } else if (isnan(v) || isinf(v)) {
    why = "not a number";
  } else if (v < c->plausibleMinV) {
    why = "below plausible range (sensor/wiring, or pack settings wrong)";
  } else if (v > c->plausibleMaxV) {
    why = "above plausible range (sensor/wiring, or pack settings wrong)";
  }

  if (why != nullptr) {
    st->valid = false;
    st->why = why;
    st->invalidTotal++;
    if (st->invalidRun < 255) st->invalidRun++;
    if (st->invalidRun >= c->invalidToUnknown) {
      /* We no longer know the battery state. Say so instead of guessing. */
      st->level = EVE_BATLVL_UNKNOWN;
      st->charging = -1;
      st->full = false;
      st->everValid = false; /* restart the filter from the next good sample */
    }
    return st->level != prevLevel || st->charging != prevChg || st->full != prevFull;
  }

  // ---- 2. Filter -----------------------------------------------------------------------------
  st->valid = true;
  st->why = "ok";
  st->invalidRun = 0;
  const bool currentOk = aOk && !isnan(a) && !isinf(a);
  if (!st->everValid) {
    st->vFilt = v;
    st->aFilt = currentOk ? a : 0.0f;
    st->everValid = true;
  } else {
    st->vFilt += c->alpha * (v - st->vFilt);
    if (currentOk) st->aFilt += c->alpha * (a - st->aFilt);
  }
  st->currentValid = currentOk;
  st->pct = eveBatteryLogicPercent(c, st->vFilt);

  // ---- 3. Level with hysteresis --------------------------------------------------------------
  st->level = levelWithHysteresis(st->level, st->pct, c);

  // ---- 4. Charging / full: only with a known current direction -------------------------------
  if (c->chargeSign == 0 || !currentOk) {
    st->charging = -1;
    st->full = false;
  } else {
    const float inA = st->aFilt * (float)c->chargeSign; /* > 0 = into the pack */
    if (st->charging == 1) {
      st->charging = (inA > c->chargeDetectA * 0.5f) ? 1 : 0;
    } else {
      st->charging = (inA > c->chargeDetectA) ? 1 : 0;
    }
    if (st->charging == 1 && st->vFilt >= c->maxV - 0.05f && inA < c->fullTaperA) {
      st->full = true;
    } else if (st->full && (inA < -c->chargeDetectA || st->vFilt < c->maxV - 0.15f)) {
      st->full = false; /* measurably discharging again */
    }
  }
  return st->level != prevLevel || st->charging != prevChg || st->full != prevFull;
}
