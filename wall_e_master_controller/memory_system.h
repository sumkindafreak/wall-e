// ============================================================
// --- ADDED: Emotional memory + flashback (modular layer) ---
// CYD: RAM ring buffer + SD append under /wall_e/memory/events.log
// Non-blocking: memoryPoll() each loop; no delay().
// ============================================================

#ifndef MEMORY_SYSTEM_H
#define MEMORY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

#define MEMORY_SOURCE_MAX 8
#define MEMORY_TYPE_MAX 16
#define MEMORY_DATA_MAX 32

typedef struct {
  uint32_t timestamp;
  char source[MEMORY_SOURCE_MAX];
  char type[MEMORY_TYPE_MAX];
  char data[MEMORY_DATA_MAX];
  float emotionalWeight;
} MemoryEvent;

void memoryInit(void);
void memoryPoll(uint32_t nowMs);

/** Returns true if stored in RAM (and queued for SD when available). */
bool memoryRecordEvent(const char* source, const char* type, const char* data, float emotionalWeight);

/** Optional default weight from type string (ALERT_EVENT, VOICE_HELLO, …). */
float memoryDefaultWeightForType(const char* type);

/** Subtle drive mix nudge from accumulated biases [-0.35, 0.35]. Call after drive resolved, before packet TX. */
void memoryApplyDriveInfluence(DriveState* ds);

float memoryGetBiasLeft(void);
float memoryGetBiasRight(void);

/** Flashback fires after 200–800 ms delay; callback runs on CYD (head nudge, etc.). */
void memoryRegisterFlashbackHandler(void (*cb)(float emotionalWeight, const char* memoryData));

#endif
