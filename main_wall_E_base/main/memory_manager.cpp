#include "memory_manager.h"
#include <LittleFS.h>

static bool s_fs = false;

#define WALLE_EVENTS_FILE "/walle_events.jsonl"
#define WALLE_VOICEBOX_FILE "/walle_voicebox.json"

void memoryManagerInit(void) {
  s_fs = LittleFS.begin(true);
  if (s_fs) {
    Serial.println(F("[MEMORY] LittleFS ready"));
  } else {
    Serial.println(F("[MEMORY] LittleFS unavailable — logging to Serial only"));
  }
}

bool memoryManagerFsReady(void) { return s_fs; }

void memoryManagerLog(uint16_t type, const char* detail) {
  uint32_t t = millis();
  Serial.printf("[MEMORY] ev=%u t=%lu %s\n", (unsigned)type, (unsigned long)t, detail ? detail : "");
  if (!s_fs) return;
  File f = LittleFS.open(WALLE_EVENTS_FILE, "a");
  if (!f) return;
  f.printf("{\"ms\":%lu,\"type\":%u,\"d\":\"%s\"}\n", (unsigned long)t, (unsigned)type,
           detail ? detail : "");
  f.close();
}

void memoryManagerTick(void) {}

void memoryManagerPersistVoiceboxMode(const char* modeName) {
  if (!s_fs || !modeName) return;
  File f = LittleFS.open(WALLE_VOICEBOX_FILE, "w");
  if (!f) return;
  f.printf("{\"mode\":\"%s\",\"ms\":%lu}\n", modeName, (unsigned long)millis());
  f.close();
}

String memoryManagerEventsJson(size_t maxBytes) {
  String out = "[";
  if (!s_fs) {
    out += "]";
    return out;
  }
  File f = LittleFS.open(WALLE_EVENTS_FILE, "r");
  if (!f) {
    out += "]";
    return out;
  }

  bool first = true;
  while (f.available() && out.length() < maxBytes) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (!first) out += ",";
    out += line;
    first = false;
  }
  f.close();
  out += "]";
  return out;
}
