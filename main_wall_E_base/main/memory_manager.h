#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "memory_protocol.h"

void memoryManagerInit(void);
void memoryManagerTick(void);

/** Append one JSON line to the event log (no blocking SD format; LittleFS when mounted). */
void memoryManagerLog(uint16_t type, const char* detail);

bool memoryManagerFsReady(void);

void memoryManagerPersistVoiceboxMode(const char* modeName);
