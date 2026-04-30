#pragma once

/*
 * Shared learning engine for WALL-E, EVE, and dock-origin events.
 *
 * Header-only and storage-agnostic: sketches can keep this in RAM, mirror it to
 * LittleFS/SD, or stream entries over the UART bus. It is intentionally capped
 * and deterministic for ESP32 memory safety.
 */

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef LEARNING_TRIGGER_LEN
#define LEARNING_TRIGGER_LEN 32u
#endif
#ifndef LEARNING_ACTION_LEN
#define LEARNING_ACTION_LEN 32u
#endif
#ifndef LEARNING_RESULT_LEN
#define LEARNING_RESULT_LEN 32u
#endif

enum LearningSource : uint8_t {
  LEARNING_SOURCE_UNKNOWN = 0,
  LEARNING_SOURCE_WALLE = 1,
  LEARNING_SOURCE_EVE = 2,
  LEARNING_SOURCE_DOCK = 3,
  LEARNING_SOURCE_SHARED = 4
};

struct LearningEntry {
  uint32_t id;
  LearningSource source;
  uint32_t timestampMs;
  uint16_t successScore;  // 0..1000
  char trigger[LEARNING_TRIGGER_LEN];
  char action[LEARNING_ACTION_LEN];
  char result[LEARNING_RESULT_LEN];
};

static inline uint32_t learningHash32(const char* a, const char* b, const char* c, uint8_t source) {
  uint32_t h = 2166136261u;
  const char* parts[3] = {a ? a : "", b ? b : "", c ? c : ""};
  h ^= source;
  h *= 16777619u;
  for (uint8_t p = 0; p < 3; p++) {
    for (const char* s = parts[p]; *s; ++s) {
      h ^= (uint8_t)*s;
      h *= 16777619u;
    }
    h ^= 0x1Fu;
    h *= 16777619u;
  }
  return h == 0 ? 1u : h;
}

static inline const char* learningSourceName(LearningSource source) {
  switch (source) {
    case LEARNING_SOURCE_WALLE: return "WALL_E";
    case LEARNING_SOURCE_EVE: return "EVE";
    case LEARNING_SOURCE_DOCK: return "DOCK";
    case LEARNING_SOURCE_SHARED: return "SHARED";
    default: return "UNKNOWN";
  }
}

static inline void learningCopyField(char* dst, size_t dstLen, const char* src) {
  if (!dst || dstLen == 0) return;
  if (!src) src = "";
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

static inline bool learningEntrySameIdentity(const LearningEntry& a, const LearningEntry& b) {
  return a.id != 0 && a.id == b.id;
}

static inline bool learningEntryBetterThan(const LearningEntry& incoming, const LearningEntry& existing) {
  if (incoming.successScore != existing.successScore) {
    return incoming.successScore > existing.successScore;
  }
  return incoming.timestampMs > existing.timestampMs;
}

template <size_t Capacity>
class LearningEngine {
 public:
  void begin(LearningSource localSource) {
    localSource_ = localSource;
    count_ = 0;
    nextReplace_ = 0;
    lastSyncMs_ = 0;
    memset(entries_, 0, sizeof(entries_));
  }

  size_t size() const { return count_; }
  size_t capacity() const { return Capacity; }
  uint32_t lastSyncMs() const { return lastSyncMs_; }

  const LearningEntry* get(size_t index) const {
    return index < count_ ? &entries_[index] : nullptr;
  }

  LearningEntry* findById(uint32_t id) {
    for (size_t i = 0; i < count_; i++) {
      if (entries_[i].id == id) return &entries_[i];
    }
    return nullptr;
  }

  const LearningEntry* findById(uint32_t id) const {
    for (size_t i = 0; i < count_; i++) {
      if (entries_[i].id == id) return &entries_[i];
    }
    return nullptr;
  }

  bool logExperience(const char* trigger, const char* action, const char* result,
                     uint16_t successScore, uint32_t timestampMs) {
    LearningEntry e = {};
    e.source = localSource_;
    e.timestampMs = timestampMs;
    e.successScore = successScore > 1000u ? 1000u : successScore;
    learningCopyField(e.trigger, sizeof(e.trigger), trigger);
    learningCopyField(e.action, sizeof(e.action), action);
    learningCopyField(e.result, sizeof(e.result), result);
    e.id = learningHash32(e.trigger, e.action, e.result, (uint8_t)e.source);
    return merge(e);
  }

  bool merge(const LearningEntry& incoming) {
    if (incoming.id == 0) return false;
    LearningEntry* existing = findById(incoming.id);
    if (existing) {
      if (learningEntryBetterThan(incoming, *existing)) {
        *existing = incoming;
        return true;
      }
      return false;
    }

    if (count_ < Capacity) {
      entries_[count_++] = incoming;
      return true;
    }

    entries_[nextReplace_] = incoming;
    nextReplace_ = (nextReplace_ + 1u) % Capacity;
    return true;
  }

  size_t mergeFrom(const LearningEntry* incoming, size_t incomingCount) {
    size_t changed = 0;
    if (!incoming) return 0;
    for (size_t i = 0; i < incomingCount; i++) {
      if (merge(incoming[i])) changed++;
    }
    return changed;
  }

  void markSyncComplete(uint32_t timestampMs) { lastSyncMs_ = timestampMs; }

  bool serializeEntryJson(size_t index, char* out, size_t outLen) const {
    const LearningEntry* e = get(index);
    if (!e || !out || outLen == 0) return false;
    snprintf(out, outLen,
             "{\"id\":%lu,\"src\":\"%s\",\"ts\":%lu,\"score\":%u,"
             "\"trigger\":\"%s\",\"action\":\"%s\",\"result\":\"%s\"}",
             (unsigned long)e->id, learningSourceName(e->source), (unsigned long)e->timestampMs,
             (unsigned)e->successScore, e->trigger, e->action, e->result);
    return true;
  }

  bool serializeSyncChunkJson(size_t startIndex, size_t maxEntries, char* out, size_t outLen,
                              uint16_t chunkIndex, bool finalChunk) const {
    if (!out || outLen == 0 || startIndex >= count_) return false;
    size_t used = snprintf(out, outLen,
                           "{\"phase\":\"chunk\",\"chunk\":%u,\"final\":%s,\"entries\":[",
                           (unsigned)chunkIndex, finalChunk ? "true" : "false");
    if (used >= outLen) return false;

    const size_t requestedEnd = startIndex + maxEntries;
    const size_t end = count_ < requestedEnd ? count_ : requestedEnd;
    for (size_t i = startIndex; i < end; i++) {
      char one[180];
      if (!serializeEntryJson(i, one, sizeof(one))) continue;
      const size_t need = strlen(one) + (i == startIndex ? 0u : 1u);
      if (used + need + 3u >= outLen) break;
      if (i != startIndex) out[used++] = ',';
      strcpy(out + used, one);
      used += strlen(one);
    }
    if (used + 3u >= outLen) return false;
    out[used++] = ']';
    out[used++] = '}';
    out[used] = '\0';
    return true;
  }

 private:
  LearningSource localSource_ = LEARNING_SOURCE_UNKNOWN;
  LearningEntry entries_[Capacity];
  size_t count_ = 0;
  size_t nextReplace_ = 0;
  uint32_t lastSyncMs_ = 0;
};

