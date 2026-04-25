// ============================================================
// --- ADDED: Emotional memory + flashback implementation ---
// ============================================================

#include "memory_system.h"
#include "sd_manager.h"
#include "motion_engine.h"
#include <Arduino.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef MEMORY_SD_LINE_MAX
#define MEMORY_SD_LINE_MAX 128
#endif

#ifndef MEMORY_RAM_CAP
#define MEMORY_RAM_CAP 48
#endif
#ifndef MEMORY_WEIGHT_THRESHOLD
#define MEMORY_WEIGHT_THRESHOLD 0.34f
#endif
#ifndef MEMORY_DECAY_INTERVAL_MS
#define MEMORY_DECAY_INTERVAL_MS 60000u
#endif
#ifndef MEMORY_DECAY_FACTOR
#define MEMORY_DECAY_FACTOR 0.985f
#endif
#ifndef MEMORY_FLASHBACK_MIN_MS
#define MEMORY_FLASHBACK_MIN_MS 200u
#endif
#ifndef MEMORY_FLASHBACK_MAX_EXTRA_MS
#define MEMORY_FLASHBACK_MAX_EXTRA_MS 800u
#endif
#ifndef MEMORY_DUPLICATE_WINDOW
#define MEMORY_DUPLICATE_WINDOW 12
#endif
#ifndef MEMORY_DRIVE_GAIN
#define MEMORY_DRIVE_GAIN 18.0f
#endif

static MemoryEvent s_ring[MEMORY_RAM_CAP];
static uint16_t s_ringCount = 0;
static uint16_t s_ringHead = 0;

static float s_biasLeft = 0.0f;
static float s_biasRight = 0.0f;

static uint32_t s_lastDecayMs = 0;
static uint32_t s_lastFlashbackCheckMs = 0;
static uint32_t s_lastFlashbackFireMs = 0;

static bool s_sdPending = false;
static MemoryEvent s_sdPendingEvt;
static char s_logPath[48];

static void (*s_flashHandler)(float, const char*) = nullptr;
static bool s_flashScheduled = false;
static uint32_t s_flashDueMs = 0;
static float s_flashWeight = 0.0f;
static char s_flashData[MEMORY_DATA_MAX];
static bool s_panNudgeActive = false;
static uint32_t s_panClearAtMs = 0;

static void sanitizeField(char* dst, size_t dstSz, const char* src) {
  if (!dst || dstSz == 0) return;
  dst[0] = '\0';
  if (!src) return;
  size_t j = 0;
  for (size_t i = 0; src[i] && j + 1 < dstSz; i++) {
    char c = src[i];
    if (c == '|' || c == '\r' || c == '\n') c = '_';
    dst[j++] = c;
  }
  dst[j] = '\0';
}

static void ringPush(const MemoryEvent* e) {
  if (!e) return;
  const uint16_t idx = (uint16_t)((s_ringHead + s_ringCount) % MEMORY_RAM_CAP);
  s_ring[idx] = *e;
  if (s_ringCount < MEMORY_RAM_CAP) {
    s_ringCount++;
  } else {
    s_ringHead = (uint16_t)((s_ringHead + 1) % MEMORY_RAM_CAP);
  }
}

static void recomputeBiases(void) {
  s_biasLeft = 0.0f;
  s_biasRight = 0.0f;
  const uint16_t n = s_ringCount < MEMORY_DUPLICATE_WINDOW ? s_ringCount : MEMORY_DUPLICATE_WINDOW;
  if (n == 0) return;
  uint16_t start = (uint16_t)((s_ringHead + s_ringCount - n) % MEMORY_RAM_CAP);
  for (uint16_t k = 0; k < n; k++) {
    uint16_t i = (uint16_t)((start + k) % MEMORY_RAM_CAP);
    const MemoryEvent& e = s_ring[i];
    if (e.emotionalWeight < 0.12f) continue;
    const float w = e.emotionalWeight * 0.06f;
    if (strstr(e.data, "LEFT") != nullptr || strstr(e.type, "LEFT") != nullptr) {
      s_biasLeft += w;
    }
    if (strstr(e.data, "RIGHT") != nullptr || strstr(e.type, "RIGHT") != nullptr) {
      s_biasRight += w;
    }
    if (strstr(e.data, "TOBY") != nullptr) {
      s_biasRight += w * 0.5f;
      s_biasLeft -= w * 0.15f;
    }
    if (strstr(e.data, "UNKNOWN") != nullptr) {
      s_biasLeft += w * 0.35f;
      s_biasRight += w * 0.35f;
    }
  }
  if (s_biasLeft > 0.35f) s_biasLeft = 0.35f;
  if (s_biasLeft < -0.35f) s_biasLeft = -0.35f;
  if (s_biasRight > 0.35f) s_biasRight = 0.35f;
  if (s_biasRight < -0.35f) s_biasRight = -0.35f;
}

static bool duplicateRecent(const char* data, float minW) {
  if (!data || !data[0]) return false;
  const uint16_t n = s_ringCount < MEMORY_DUPLICATE_WINDOW ? s_ringCount : MEMORY_DUPLICATE_WINDOW;
  uint8_t hits = 0;
  uint16_t start = (uint16_t)((s_ringHead + s_ringCount - n) % MEMORY_RAM_CAP);
  for (uint16_t k = 0; k < n; k++) {
    uint16_t i = (uint16_t)((start + k) % MEMORY_RAM_CAP);
    if (s_ring[i].emotionalWeight >= minW && strncmp(s_ring[i].data, data, MEMORY_DATA_MAX) == 0) {
      hits++;
    }
  }
  return hits >= 2;
}

static void tryScheduleFlashback(uint32_t nowMs, const MemoryEvent* trigger) {
  if (!trigger) return;
  if (s_flashScheduled) return;
  if ((uint32_t)(nowMs - s_lastFlashbackFireMs) < 15000u) return;
  if (trigger->emotionalWeight < 0.45f && !duplicateRecent(trigger->data, 0.38f)) {
    if (random(10000) > (int)(trigger->emotionalWeight * 120.0f)) return;
  }
  s_flashWeight = trigger->emotionalWeight;
  strncpy(s_flashData, trigger->data, sizeof(s_flashData) - 1);
  s_flashData[sizeof(s_flashData) - 1] = '\0';
  s_flashDueMs = nowMs + MEMORY_FLASHBACK_MIN_MS + (uint32_t)(random(MEMORY_FLASHBACK_MAX_EXTRA_MS + 1));
  s_flashScheduled = true;
}

static void appendSdLineNonblocking(void) {
  if (!s_sdPending || !sdIsAvailable()) return;
  File f = SD.open(s_logPath, FILE_APPEND);
  if (!f) {
    return;
  }
  char line[MEMORY_SD_LINE_MAX];
  const MemoryEvent& e = s_sdPendingEvt;
  snprintf(line, sizeof(line), "%lu|%s|%s|%s|%.4f\n", (unsigned long)e.timestamp, e.source, e.type, e.data,
           e.emotionalWeight);
  f.print(line);
  f.close();
  s_sdPending = false;
}

float memoryDefaultWeightForType(const char* type) {
  if (!type) return 0.4f;
  if (strcmp(type, "ALERT_EVENT") == 0) return 0.8f;
  if (strcmp(type, "VOICE_HELLO") == 0) return 0.6f;
  if (strcmp(type, "UNKNOWN_OBJECT") == 0) return 0.5f;
  if (strcmp(type, "VISION") == 0) return 0.45f;
  if (strcmp(type, "SENSOR") == 0) return 0.42f;
  return 0.4f;
}

void memoryRegisterFlashbackHandler(void (*cb)(float, const char*)) {
  s_flashHandler = cb;
}

float memoryGetBiasLeft(void) { return s_biasLeft; }
float memoryGetBiasRight(void) { return s_biasRight; }

void memoryInit(void) {
  s_ringCount = 0;
  s_ringHead = 0;
  s_biasLeft = s_biasRight = 0.0f;
  s_lastDecayMs = millis();
  s_lastFlashbackCheckMs = millis();
  snprintf(s_logPath, sizeof(s_logPath), "%s/events.log", SD_MEMORY_DIR);
  s_sdPending = false;
  s_flashScheduled = false;
}

bool memoryRecordEvent(const char* source, const char* type, const char* data, float emotionalWeight) {
  if (emotionalWeight <= MEMORY_WEIGHT_THRESHOLD) return false;

  MemoryEvent e = {};
  e.timestamp = millis();
  sanitizeField(e.source, sizeof(e.source), source ? source : "?");
  sanitizeField(e.type, sizeof(e.type), type ? type : "?");
  sanitizeField(e.data, sizeof(e.data), data ? data : "");
  e.emotionalWeight = emotionalWeight;

  ringPush(&e);
  recomputeBiases();

  if (sdIsAvailable()) {
    if (!s_sdPending) {
      s_sdPendingEvt = e;
      s_sdPending = true;
    }
  }

  tryScheduleFlashback(millis(), &e);
  return true;
}

static void decayRing(uint32_t nowMs) {
  if ((uint32_t)(nowMs - s_lastDecayMs) < MEMORY_DECAY_INTERVAL_MS) return;
  s_lastDecayMs = nowMs;
  for (uint16_t i = 0; i < s_ringCount; i++) {
    uint16_t idx = (uint16_t)((s_ringHead + i) % MEMORY_RAM_CAP);
    s_ring[idx].emotionalWeight *= MEMORY_DECAY_FACTOR;
    if (s_ring[idx].emotionalWeight < 0.02f) {
      s_ring[idx].emotionalWeight = 0.0f;
    }
  }
  recomputeBiases();
}

static void checkFlashbackTriggers(uint32_t nowMs) {
  if ((uint32_t)(nowMs - s_lastFlashbackCheckMs) < 400u) return;
  s_lastFlashbackCheckMs = nowMs;
  if (s_ringCount == 0) return;
  uint16_t lastIdx = (uint16_t)((s_ringHead + s_ringCount - 1) % MEMORY_RAM_CAP);
  const MemoryEvent& last = s_ring[lastIdx];
  if (duplicateRecent(last.data, 0.36f)) {
    tryScheduleFlashback(nowMs, &last);
  }
}

void memoryPoll(uint32_t nowMs) {
  if (s_panNudgeActive && (int32_t)(nowMs - s_panClearAtMs) >= 0) {
    motionSetHeadPanVelocity(0.0f);
    s_panNudgeActive = false;
  }

  decayRing(nowMs);
  checkFlashbackTriggers(nowMs);
  appendSdLineNonblocking();

  if (s_flashScheduled && (int32_t)(nowMs - s_flashDueMs) >= 0) {
    s_flashScheduled = false;
    s_lastFlashbackFireMs = nowMs;
    if (s_flashHandler) {
      s_flashHandler(s_flashWeight, s_flashData);
    }
    /* CYD “hesitation”: brief head pan nudge, cleared by memoryPoll */
    motionSetHeadPanVelocity(s_flashWeight >= 0.5f ? -0.07f : 0.06f);
    s_panNudgeActive = true;
    s_panClearAtMs = nowMs + 280u;
  }
}

void memoryApplyDriveInfluence(DriveState* ds) {
  if (!ds) return;
  float dl = (float)ds->leftSpeed + s_biasLeft * MEMORY_DRIVE_GAIN;
  float dr = (float)ds->rightSpeed + s_biasRight * MEMORY_DRIVE_GAIN;
  int li = (int)(dl + (dl >= 0 ? 0.5f : -0.5f));
  int ri = (int)(dr + (dr >= 0 ? 0.5f : -0.5f));
  if (li > 100) li = 100;
  if (li < -100) li = -100;
  if (ri > 100) ri = 100;
  if (ri < -100) ri = -100;
  ds->leftSpeed = (int8_t)li;
  ds->rightSpeed = (int8_t)ri;
}
