// ============================================================
// LROS sequence engine — stored show timelines + playback
// Persistence: Preferences namespace "lros_seq", key "catalog"
// ============================================================

#include "sequence_engine.h"
#include "navigation_api.h"
#include "motor_control.h"
#include "laser_control.h"
#include "walle_emotion_pose.h"
#include "audio_espnow.h"
#include "audio_protocol.h"
#include "unified_autonomy_engine.h"
#include "battery_monitor.h"
#include "vision_behaviour.h"
#include "vision_protocol.h"
#include "motion_authority.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "dock_homing.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <algorithm>
#include <vector>
#include <cctype>
#include <cstring>

extern unsigned long lastCommandMillis;

static const char* kSeqNs = "lros_seq";
static const char* kSeqKey = "catalog";

static DynamicJsonDocument* s_runDoc = nullptr;
static size_t s_nextStep = 0;
static uint32_t s_startMs = 0;
static uint32_t s_pauseSlip = 0;
static uint32_t s_motorStopAt = 0;
static bool s_running = false;
static char s_runId[40] = "";

static bool strEqLo(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    const char ca = (char)tolower((unsigned char)*a++);
    const char cb = (char)tolower((unsigned char)*b++);
    if (ca != cb) return false;
  }
  return *a == *b;
}

// ArduinoJson cannot deserialize directly into an existing JsonObject. Sort
// through serialized step copies, then deserialize each copy into a temporary
// document and add the parsed variant back into the destination array.
static void sortStepsArray(JsonArray steps) {
  const size_t n = steps.size();
  if (n <= 1) return;

  std::vector<std::pair<uint32_t, String>> sorted;
  sorted.reserve(n);
  for (JsonObject step : steps) {
    String encoded;
    serializeJson(step, encoded);
    sorted.push_back({(uint32_t)(step["at_ms"] | 0u), encoded});
  }

  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const std::pair<uint32_t, String>& a,
                      const std::pair<uint32_t, String>& b) {
                     return a.first < b.first;
                   });

  steps.clear();
  for (const auto& entry : sorted) {
    const size_t capacity = max((size_t)512,
                                (size_t)entry.second.length() * 2u + 256u);
    DynamicJsonDocument parsed(capacity);
    if (deserializeJson(parsed, entry.second) == DeserializationError::Ok) {
      steps.add(parsed.as<JsonVariantConst>());
    }
  }
}

static bool loadCatalog(DynamicJsonDocument& doc) {
  Preferences prefs;
  prefs.begin(kSeqNs, true);
  const String stored = prefs.getString(kSeqKey, "");
  prefs.end();

  if (stored.length() == 0) {
    doc.clear();
    doc["v"] = 1;
    doc.createNestedArray("items");
    return true;
  }

  const DeserializationError error = deserializeJson(doc, stored);
  if (error) {
    doc.clear();
    doc["v"] = 1;
    doc.createNestedArray("items");
    return false;
  }

  if (!doc["items"].is<JsonArray>()) {
    doc.clear();
    doc["v"] = 1;
    doc.createNestedArray("items");
  }
  return true;
}

static bool saveCatalog(DynamicJsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  if (out.length() > 32000) return false;

  Preferences prefs;
  prefs.begin(kSeqNs, false);
  const size_t written = prefs.putString(kSeqKey, out);
  prefs.end();
  return written == out.length();
}

static int8_t parseEmotionId(JsonObject& object) {
  if (object.containsKey("emotion_id")) {
    const int value = object["emotion_id"].as<int>();
    if (value >= 0 && value <= (int)WALLE_EMOTION_TIRED) return (int8_t)value;
  }

  if (object.containsKey("emotion")) {
    const char* emotion = object["emotion"] | "";
    if (strEqLo(emotion, "neutral")) return (int8_t)WALLE_EMOTION_NEUTRAL;
    if (strEqLo(emotion, "curious")) return (int8_t)WALLE_EMOTION_CURIOUS;
    if (strEqLo(emotion, "happy")) return (int8_t)WALLE_EMOTION_HAPPY;
    if (strEqLo(emotion, "sad")) return (int8_t)WALLE_EMOTION_SAD;
    if (strEqLo(emotion, "scared")) return (int8_t)WALLE_EMOTION_SCARED;
    if (strEqLo(emotion, "tired")) return (int8_t)WALLE_EMOTION_TIRED;
  }
  return -1;
}

static int parseVisionEventName(const char* name) {
  if (!name || !name[0]) return -1;
  if (strEqLo(name, "none")) return VEVENT_NONE;
  if (strEqLo(name, "target_detected") || strEqLo(name, "detected")) {
    return VEVENT_TARGET_DETECTED;
  }
  if (strEqLo(name, "target_approaching") || strEqLo(name, "approaching")) {
    return VEVENT_TARGET_APPROACHING;
  }
  if (strEqLo(name, "target_centered") || strEqLo(name, "centered")) {
    return VEVENT_TARGET_CENTERED;
  }
  if (strEqLo(name, "target_lost") || strEqLo(name, "lost")) {
    return VEVENT_TARGET_LOST;
  }
  return -1;
}

// Optional step predicate documented by LROS:
//   when: {
//     battery_pct_min: 20,
//     battery_pct_max: 80,
//     dock_fsm: "IDLE" | numeric DockState,
//     vision_event: "target_detected" | numeric VEVENT_*
//   }
// Every supplied condition must be satisfied. Missing sensor data fails closed
// for that condition instead of executing a step on guessed state.
static bool sequenceWhenMet(JsonObject condition) {
  if (condition.isNull()) return true;

  if (condition.containsKey("battery_pct_min") ||
      condition.containsKey("battery_pct_max")) {
    const BatteryData& battery = batteryGetData();
    if (!battery.valid || battery.percent < 0) return false;

    if (condition.containsKey("battery_pct_min")) {
      const int minimum = constrain(condition["battery_pct_min"].as<int>(), 0, 100);
      if (battery.percent < minimum) return false;
    }
    if (condition.containsKey("battery_pct_max")) {
      const int maximum = constrain(condition["battery_pct_max"].as<int>(), 0, 100);
      if (battery.percent > maximum) return false;
    }
  }

  if (condition.containsKey("dock_fsm")) {
#if USE_AUTONOMOUS_DOCKING
    JsonVariant dockCondition = condition["dock_fsm"];
    const DockState current = autonomousDockingGetState();
    if (dockCondition.is<const char*>()) {
      if (!strEqLo(dockCondition.as<const char*>(), autonomousDockingGetStateName())) {
        return false;
      }
    } else {
      if (dockCondition.as<int>() != (int)current) return false;
    }
#else
    JsonVariant dockCondition = condition["dock_fsm"];
    const bool active = dockHomingIsActive();
    if (dockCondition.is<const char*>()) {
      const char* wanted = dockCondition.as<const char*>();
      if (active && !strEqLo(wanted, "ACTIVE") && !strEqLo(wanted, "DOCKING")) return false;
      if (!active && !strEqLo(wanted, "IDLE")) return false;
    } else if ((dockCondition.as<int>() != 0) != active) {
      return false;
    }
#endif
  }

  if (condition.containsKey("vision_event")) {
    JsonVariant eventCondition = condition["vision_event"];
    int wanted = -1;
    if (eventCondition.is<const char*>()) {
      wanted = parseVisionEventName(eventCondition.as<const char*>());
    } else {
      wanted = eventCondition.as<int>();
    }
    if (wanted < 0 || wanted > VEVENT_TARGET_LOST) return false;
    if ((int)visionGetLastEventCode() != wanted) return false;
  }

  return true;
}

static void execStep(JsonObject& step) {
  const char* kind = step["kind"] | "noop";
  char error[48] = {};

  if (step.containsKey("when")) {
    JsonObject when = step["when"].as<JsonObject>();
    if (!sequenceWhenMet(when)) return;
  }

  if (!strcmp(kind, "noop") || !strcmp(kind, "note") ||
      !strcmp(kind, "comment")) {
    return;
  }

  if (!strcmp(kind, "wait")) {
    uint32_t duration = (uint32_t)(step["ms"] | 0u);
    if (duration > 600000u) duration = 600000u;
    s_pauseSlip += duration;
    return;
  }

  if (!strcmp(kind, "emotion")) {
    const int8_t id = parseEmotionId(step);
    if (id >= 0) walleEmotionPoseSetManualOverride(id);
    return;
  }

  if (!strcmp(kind, "audio_play")) {
    int id = step.containsKey("track") ? step["track"].as<int>() :
             (step.containsKey("id") ? step["id"].as<int>() : 1);
    id = constrain(id, 1, 255);
    (void)audioEspNowPlayTrack((uint8_t)id, WALLE_AUDIO_PRIORITY_WEB);
    return;
  }

  if (!strcmp(kind, "audio_volume")) {
    const int value = constrain((int)(step["value"] | 128), 0, 255);
    const uint8_t dfPlayerVolume = (uint8_t)((value * 30 + 127) / 255);
    (void)audioEspNowSetVolume(dfPlayerVolume);
    return;
  }

  if (!strcmp(kind, "navigation_route")) {
    if (!motionAuthorityAllowWeb() || !step.containsKey("route")) return;
    String routeJson;
    serializeJson(step["route"], routeJson);
    if (!navigationApplyRouteFromJson(routeJson.c_str(), routeJson.length(),
                                      error, sizeof(error))) {
      Serial.printf("[Seq] navigation_route failed: %s\n", error);
    }
    return;
  }

  if (!strcmp(kind, "drive_tank")) {
    if (!motionAuthorityAllowWeb() || unifiedAutonomySafetyActive()) return;
    const int16_t left = (int16_t)constrain((int)(step["left"] | 0), -255, 255);
    const int16_t right = (int16_t)constrain((int)(step["right"] | 0), -255, 255);
    uint32_t duration = (uint32_t)(step["ms"] | 500u);
    if (duration > 30000u) duration = 30000u;
    motorSetLeftRight(left, right);
    lastCommandMillis = millis();
    s_motorStopAt = millis() + duration;
    return;
  }

  if (!strcmp(kind, "motor_stop")) {
    motorStop();
    s_motorStopAt = 0;
    return;
  }

  if (!strcmp(kind, "laser_off")) {
    laserOff();
    return;
  }
  if (!strcmp(kind, "laser_on")) {
    laserOn();
    return;
  }
  if (!strcmp(kind, "laser_brightness")) {
    laserSetBrightness((uint8_t)constrain((int)(step["value"] | 128), 0, 255));
    return;
  }
  if (!strcmp(kind, "laser_fire")) {
    const int pan = step["pan"] | 50;
    const int tilt = step["tilt"] | 50;
    uint32_t duration = step.containsKey("ms")
                            ? step["ms"].as<uint32_t>()
                            : (step.containsKey("duration_ms")
                                   ? step["duration_ms"].as<uint32_t>()
                                   : 300u);
    if (duration > 5000u) duration = 5000u;
    laserFire(pan, tilt, duration);
    return;
  }
  if (!strcmp(kind, "laser_mood")) {
    laserSetMoodMode((int8_t)constrain((int)(step["mood"] | 0), -1, 8));
  }
}

void sequenceEngineInit(void) {
  DynamicJsonDocument doc(2048);
  (void)loadCatalog(doc);
  Serial.println(F("[Seq] Engine init"));
}

void sequenceEngineTick(unsigned long nowMs) {
  if (s_motorStopAt != 0 && (long)(nowMs - s_motorStopAt) >= 0) {
    motorStop();
    s_motorStopAt = 0;
  }

  if (!s_running || !s_runDoc) return;
  if (unifiedAutonomySafetyActive()) {
    sequenceStop();
    return;
  }

  JsonArray steps = (*s_runDoc)["steps"].as<JsonArray>();
  if (steps.isNull()) {
    sequenceStop();
    return;
  }

  const uint32_t wall = (uint32_t)(nowMs - s_startMs);
  const uint32_t effective = wall >= s_pauseSlip ? wall - s_pauseSlip : 0u;

  while (s_nextStep < steps.size()) {
    JsonObject step = steps[s_nextStep].as<JsonObject>();
    const uint32_t at = (uint32_t)(step["at_ms"] | 0u);
    if (effective < at) break;
    execStep(step);
    ++s_nextStep;
  }

  if (s_nextStep >= steps.size()) {
    s_running = false;
    delete s_runDoc;
    s_runDoc = nullptr;
    s_runId[0] = '\0';
    Serial.println(F("[Seq] Finished"));
  }
}

void sequenceStop(void) {
  s_running = false;
  s_motorStopAt = 0;
  motorStop();
  if (s_runDoc) {
    delete s_runDoc;
    s_runDoc = nullptr;
  }
  s_nextStep = 0;
  s_pauseSlip = 0;
  s_runId[0] = '\0';
}

bool sequenceIsRunning(void) {
  return s_running;
}

bool sequenceRun(const char* id, char* errBuf, size_t errLen) {
  if (errBuf && errLen) errBuf[0] = '\0';
  if (!id || !id[0]) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "missing_id");
    return false;
  }

  sequenceStop();

  DynamicJsonDocument catalog(16384);
  (void)loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  if (items.isNull()) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "no_sequences");
    return false;
  }

  for (JsonObject item : items) {
    if (strcmp(item["id"] | "", id) != 0) continue;

    JsonArray steps = item["steps"].as<JsonArray>();
    if (steps.isNull() || steps.size() == 0) {
      if (errBuf && errLen) snprintf(errBuf, errLen, "no_steps");
      return false;
    }

    String stepsJson;
    serializeJson(steps, stepsJson);
    String wrapper = "{\"steps\":";
    wrapper += stepsJson;
    wrapper += "}";

    DynamicJsonDocument* runDoc = new DynamicJsonDocument(8192);
    if (!runDoc) {
      if (errBuf && errLen) snprintf(errBuf, errLen, "alloc_failed");
      return false;
    }

    const DeserializationError parseError = deserializeJson(*runDoc, wrapper);
    if (parseError) {
      delete runDoc;
      if (errBuf && errLen) snprintf(errBuf, errLen, "run_parse");
      return false;
    }

    s_runDoc = runDoc;
    s_nextStep = 0;
    s_startMs = millis();
    s_pauseSlip = 0;
    s_running = true;
    strncpy(s_runId, id, sizeof(s_runId) - 1);
    s_runId[sizeof(s_runId) - 1] = '\0';
    Serial.printf("[Seq] Run %s steps=%u\n", id, (unsigned)steps.size());
    return true;
  }

  if (errBuf && errLen) snprintf(errBuf, errLen, "not_found");
  return false;
}

String sequenceGetStatusJSON(void) {
  String json = "{\"ok\":true";
  json += ",\"running\":";
  json += s_running ? "true" : "false";
  json += ",\"id\":\"";
  json += s_runId;
  json += "\",\"step\":";
  json += (unsigned)s_nextStep;
  json += "}";
  return json;
}

String sequenceListJSON(void) {
  DynamicJsonDocument catalog(16384);
  (void)loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();

  DynamicJsonDocument out(16384);
  out["ok"] = true;
  JsonArray result = out.createNestedArray("sequences");

  for (JsonObject item : items) {
    JsonObject row = result.createNestedObject();
    row["id"] = item["id"] | "";
    row["name"] = item["name"] | "";

    JsonArray steps = item["steps"].as<JsonArray>();
    uint32_t maxAt = 0;
    size_t count = 0;
    if (!steps.isNull()) {
      count = steps.size();
      for (JsonObject step : steps) {
        const uint32_t at = (uint32_t)(step["at_ms"] | 0u);
        if (at > maxAt) maxAt = at;
      }
    }
    row["step_count"] = (unsigned)count;
    row["duration_ms"] = maxAt;
  }

  String json;
  serializeJson(out, json);
  return json;
}

bool sequenceGetOneJSON(const char* id, String& out) {
  if (!id || !id[0]) return false;

  DynamicJsonDocument catalog(16384);
  (void)loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  for (JsonObject item : items) {
    if (strcmp(item["id"] | "", id) != 0) continue;

    DynamicJsonDocument one(8192);
    one["ok"] = true;
    JsonObject sequence = one.createNestedObject("sequence");
    sequence["id"] = item["id"];
    sequence["name"] = item["name"];
    sequence["steps"] = item["steps"];
    serializeJson(one, out);
    return true;
  }
  return false;
}

bool sequenceDelete(const char* id) {
  if (!id || !id[0]) return false;

  DynamicJsonDocument catalog(16384);
  (void)loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  for (size_t i = 0; i < items.size(); ++i) {
    JsonObject item = items[i].as<JsonObject>();
    if (strcmp(item["id"] | "", id) == 0) {
      items.remove(i);
      return saveCatalog(catalog);
    }
  }
  return false;
}

bool sequenceSaveJSON(const char* body, size_t len,
                      char* errBuf, size_t errLen) {
  if (errBuf && errLen) errBuf[0] = '\0';
  if (!body || len == 0 || len > 12000) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "body_size");
    return false;
  }

  DynamicJsonDocument input(8192);
  const DeserializationError error = deserializeJson(input, body, len);
  if (error) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "json_parse");
    return false;
  }

  const char* id = input["id"] | "";
  const char* name = input["name"] | "Untitled";
  if (!id[0]) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "missing_id");
    return false;
  }

  JsonArray incomingSteps = input["steps"].as<JsonArray>();
  if (incomingSteps.isNull()) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "missing_steps");
    return false;
  }

  DynamicJsonDocument catalog(16384);
  (void)loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();

  for (size_t i = 0; i < items.size(); ++i) {
    JsonObject item = items[i].as<JsonObject>();
    if (strcmp(item["id"] | "", id) == 0) {
      items.remove(i);
      break;
    }
  }

  JsonObject slot = items.createNestedObject();
  slot["id"] = id;
  slot["name"] = name;
  slot["steps"] = incomingSteps;
  sortStepsArray(slot["steps"].as<JsonArray>());

  if (!saveCatalog(catalog)) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "save_failed");
    return false;
  }
  return true;
}
