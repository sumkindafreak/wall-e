// ============================================================
//  WALL-E Memory Sync Protocol
//  Base ↔ Controller: personality + home position
//  Used for SD backup on controller (hybrid: NVS on base + SD on controller)
// ============================================================

#pragma once

#include <stdint.h>

#define MEMORY_SYNC_MAGIC  0x4D454D31  // "MEM1"
#define MEMORY_SYNC_VERSION 1

// Action: controller requests base to send memory
#define ACTION_MEMORY_EXPORT  10

typedef struct __attribute__((packed)) {
  uint32_t magic;       // MEMORY_SYNC_MAGIC
  uint8_t  version;     // MEMORY_SYNC_VERSION
  uint8_t  reserved[3];
  // Personality (0.0 - 1.0)
  float    curiosity;
  float    bravery;
  float    energy;
  float    randomness;
  // Home position
  double   homeLat;
  double   homeLon;
  uint8_t  homeSet;     // 0 = not set, 1 = set
  uint8_t  pad[7];
} MemoryPacket;
