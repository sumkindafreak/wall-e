#pragma once

#include <Arduino.h>
#include <stdint.h>

/** Raw ranges in mm; 0 or > EVE_TOF_FAR_IGNORE_MM = invalid / no object */
typedef struct {
  int32_t left_mm;
  int32_t right_mm;
  int32_t center_mm;
  uint8_t valid_mask; /* bit0 left, bit1 right, bit2 center */
} EveTofRawFrame;

void eveTofManagerInit(void);
/** Rate-limited internally to EVE_TOF_POLL_MS when enabled */
void eveTofManagerPoll(uint32_t nowMs);
bool eveTofManagerGetLastFrame(EveTofRawFrame* out);
