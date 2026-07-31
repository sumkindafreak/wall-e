// ============================================================
//  LROS sequence engine — stored show timelines + playback
//  Persistence: Preferences namespace "lros_seq", key "catalog"
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
#include "motion_authority.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "dock_homing.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <algorithm>
#include <vector>
#include <cctype>
#include <strings.h>

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

static void sortStepsArray(JsonArray steps) {
  size_t n = steps.size();
  if (n <= 1) return;
  std::vector<std::pair<uint32_t, String>> tmp;
  tmp.reserve(n);
  for (JsonObject o : steps) {
    String s;
    serializeJson(o, s);
    tmp.push_back({(uint32_t)(o["at_ms"] | 0), s});
  }
  std::sort(tmp.begin(), tmp.end(),
             [](const std::pair<uint32_t, String>& a, const std::pair<uint32_t, String>& b) {
               return a.first < b.first;
             });
  steps.clear();
  /* ArduinoJson 7: deserialize into JsonDocument, then copy keys into new array elements. */
  for (auto& p : tmp) {
    DynamicJsonDocument one(4096);
    DeserializationError e =
        deserializeJson(one, p.second.c_str(), p.second.length());
    if (e) continue;
    JsonObjectConst src = one.as<JsonObjectConst>();
    if (src.isNull()) continue;
    JsonObject dest = steps.createNestedObject();
    for (JsonPairConst kv : src) {
      dest[kv.key()] = kv.value();
    }
  }
}

static bool loadCatalog(DynamicJsonDocument& doc) {
  Preferences prefs;
  prefs.begin(kSeqNs, true);
  String s = prefs.getString(kSeqKey, "");
  prefs.end();
  if (s.length() == 0) {
    doc.clear();
    doc["v"] = 1;
    doc.createNestedArray("items");
    return true;
  }
  DeserializationError e = deserializeJson(doc, s);
  if (e) {
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
  prefs.putString(kSeqKey, out);
  prefs.end();
  return true;
}

static bool strEqLo(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    char ca = (char)tolower((unsigned char)*a++);
    char cb = (char)tolower((unsigned char)*b++);
    if (ca != cb) return false;
  }
  return *a == *b;
}

/** Optional step gate: { "when": { ... } }. Unknown keys are ignored (pass). */
static bool sequenceWhenMet(JsonObject w) {
  if (w.isNull()) return true;

  const BatteryData& bat = batteryGetData();

  if (w.containsKey("battery_pct_min")) {
    int mn = w["battery_pct_min"] | 0;
    if (!bat.valid || bat.percent < mn) return false;
  }
  if (w.containsKey("battery_pct_max")) {
    int mx = w["battery_pct_max"] | 100;
    if (!bat.valid || bat.percent > mx) return false;
  }
  if (w.containsKey("battery_voltage_min")) {
    float mn = w["battery_voltage_min"] | 0.0f;
    if (!bat.valid || bat.voltage < mn) return false;
  }
  if (w.containsKey("battery_voltage_max")) {
    float mx = w["battery_voltage_max"] | 99.0f;
    if (!bat.valid || bat.voltage > mx) return false;
  }
  if (w.containsKey("vision_engaged")) {
    bool want = w["vision_engaged"] | false;
    if (visionBehaviourIsEngaged() != want) return false;
  }
  return true;
}

static int8_t parseEmotionId(JsonObject& o) {
  if (o.containsKey("emotion_id")) {
    int v = o["emotion_id"].as<int>();
    if (v >= 0 && v <= (int)WALLE_EMOTION_TIRED) return (int8_t)v;
  }
  if (o.containsKey("emotion")) {
    const char* e = o["emotion"] | "";
    if (strEqLo(e, "neutral")) return (int8_t)WALLE_EMOTION_NEUTRAL;
    if (strEqLo(e, "curious")) return (int8_t)WALLE_EMOTION_CURIOUS;
    if (strEqLo(e, "happy")) return (int8_t)WALLE_EMOTION_HAPPY;
    if (strEqLo(e, "sad")) return (int8_t)WALLE_EMOTION_SAD;
    if (strEqLo(e, "scared")) return (int8_t)WALLE_EMOTION_SCARED;
    if (strEqLo(e, "tired")) return (int8_t)WALLE_EMOTION_TIRED;
  }
  return -1;
}

static void execStep(JsonObject& st) {
  const char* kind = st["kind"] | "noop";
  char err[48];

  if (st.containsKey("when")) {
    JsonObject w = st["when"].as<JsonObject>();
    if (!sequenceWhenMet(w)) return;
  }

  if (!strcmp(kind, "noop") || !strcmp(kind, "note") || !strcmp(kind, "comment")) {
    return;
  }
  if (!strcmp(kind, "wait")) {
    uint32_t ms = (uint32_t)(st["ms"] | 0);
    if (ms > 600000) ms = 600000;
    s_pauseSlip += ms;
    return;
  }
  if (!strcmp(kind, "emotion")) {
    int8_t id = parseEmotionId(st);
    if (id >= 0) walleEmotionPoseSetManualOverride(id);
    return;
  }
  if (!strcmp(kind, "audio_play")) {
    int id = st["track"] | st["id"] | 1;
    if (id < 1) id = 1;
    if (id > 255) id = 255;
    audioEspNowPlayTrack((uint8_t)id, WALLE_AUDIO_PRIORITY_WEB);
    return;
  }
  if (!strcmp(kind, "audio_volume")) {
    int v = st["value"] | 128;
    v = constrain(v, 0, 255);
    uint8_t df = (uint8_t)((v * 30 + 127) / 255);
    audioEspNowSetVolume(df);
    return;
  }
  if (!strcmp(kind, "navigation_route")) {
    if (!motionAuthorityAllowWeb()) return;
    if (!st.containsKey("route")) return;
    String routeStr;
    serializeJson(st["route"], routeStr);
    if (!navigationApplyRouteFromJson(routeStr.c_str(), routeStr.length(), err, sizeof(err))) {
      Serial.printf("[Seq] navigation_route failed: %s\n", err);
    }
    return;
  }
  if (!strcmp(kind, "drive_tank")) {
    if (!motionAuthorityAllowWeb()) return;
    int16_t L = (int16_t)constrain((int)(st["left"] | 0), -255, 255);
    int16_t R = (int16_t)constrain((int)(st["right"] | 0), -255, 255);
    uint32_t ms = (uint32_t)(st["ms"] | 500);
    if (ms > 30000) ms = 30000;
    if (unifiedAutonomySafetyActive()) return;
    motorSetLeftRight(L, R);
    lastCommandMillis = millis();
    s_motorStopAt = millis() + ms;
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
    int v = st["value"] | 128;
    laserSetBrightness((uint8_t)constrain(v, 0, 255));
    return;
  }
  if (!strcmp(kind, "laser_fire")) {
    int pan = st["pan"] | 50;
    int tilt = st["tilt"] | 50;
    uint32_t d = (uint32_t)(st["ms"] | st["duration_ms"] | 300);
    if (d > 5000) d = 5000;
    laserFire(pan, tilt, d);
    return;
  }
  if (!strcmp(kind, "laser_mood")) {
    int m = st["mood"] | 0;
    laserSetMoodMode((int8_t)constrain(m, -1, 8));
    return;
  }
}

void sequenceEngineInit(void) {
  DynamicJsonDocument doc(2048);
  loadCatalog(doc);
  Serial.println(F("[Seq] Engine init"));
}

void sequenceEngineTick(unsigned long now_ms) {
  if (s_motorStopAt != 0 && (long)(now_ms - s_motorStopAt) >= 0) {
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

  uint32_t wall = (uint32_t)(now_ms - s_startMs);
  uint32_t effective = (wall >= s_pauseSlip) ? (wall - s_pauseSlip) : 0;

  while (s_nextStep < steps.size()) {
    JsonObject st = steps[s_nextStep].as<JsonObject>();
    uint32_t at = (uint32_t)(st["at_ms"] | 0);
    if (effective < at) break;
    execStep(st);
    s_nextStep++;
  }

  if (s_nextStep >= steps.size()) {
    s_running = false;
    if (s_runDoc) {
      delete s_runDoc;
      s_runDoc = nullptr;
    }
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
  loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  if (items.isNull()) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "no_sequences");
    return false;
  }

  for (JsonObject it : items) {
    const char* iid = it["id"] | "";
    if (strcmp(iid, id) != 0) continue;

    JsonArray steps = it["steps"].as<JsonArray>();
    if (steps.isNull() || steps.size() == 0) {
      if (errBuf && errLen) snprintf(errBuf, errLen, "no_steps");
      return false;
    }

    String stepsJson;
    serializeJson(steps, stepsJson);
    String wrap = "{\"steps\":";
    wrap += stepsJson;
    wrap += "}";

    s_runDoc = new DynamicJsonDocument(8192);
    DeserializationError er = deserializeJson(*s_runDoc, wrap);
    if (er) {
      if (errBuf && errLen) snprintf(errBuf, errLen, "run_parse");
      return false;
    }
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
  String j = "{\"ok\":true";
  j += ",\"running\":"; j += s_running ? "true" : "false";
  j += ",\"id\":\""; j += s_runId; j += "\"";
  j += ",\"step\":"; j += (unsigned)s_nextStep;
  j += "}";
  return j;
}

String sequenceListJSON(void) {
  DynamicJsonDocument catalog(16384);
  loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();

  DynamicJsonDocument out(16384);
  out["ok"] = true;
  JsonArray arr = out.createNestedArray("sequences");

  for (JsonObject it : items) {
    JsonObject row = arr.createNestedObject();
    row["id"] = it["id"] | "";
    row["name"] = it["name"] | "";
    JsonArray steps = it["steps"].as<JsonArray>();
    uint32_t maxAt = 0;
    size_t n = 0;
    if (!steps.isNull()) {
      n = steps.size();
      for (JsonObject st : steps) {
        uint32_t at = (uint32_t)(st["at_ms"] | 0);
        if (at > maxAt) maxAt = at;
      }
    }
    row["step_count"] = (unsigned)n;
    row["duration_ms"] = maxAt;
  }

  String s;
  serializeJson(out, s);
  return s;
}

bool sequenceGetOneJSON(const char* id, String& out) {
  if (!id || !id[0]) return false;
  DynamicJsonDocument catalog(16384);
  loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  for (JsonObject it : items) {
    if (strcmp(it["id"] | "", id) != 0) continue;
    DynamicJsonDocument one(8192);
    one["ok"] = true;
    JsonObject seq = one.createNestedObject("sequence");
    seq["id"] = it["id"];
    seq["name"] = it["name"];
    seq["steps"] = it["steps"];
    serializeJson(one, out);
    return true;
  }
  return false;
}

bool sequenceDelete(const char* id) {
  if (!id || !id[0]) return false;
  DynamicJsonDocument catalog(16384);
  loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();
  for (size_t i = 0; i < items.size(); i++) {
    JsonObject it = items[i].as<JsonObject>();
    if (strcmp(it["id"] | "", id) == 0) {
      items.remove(i);
      return saveCatalog(catalog);
    }
  }
  return false;
}

bool sequenceSaveJSON(const char* body, size_t len, char* errBuf, size_t errLen) {
  if (errBuf && errLen) errBuf[0] = '\0';
  if (!body || len == 0 || len > 12000) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "body_size");
    return false;
  }

  DynamicJsonDocument in(8192);
  DeserializationError e = deserializeJson(in, body, len);
  if (e) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "json_parse");
    return false;
  }

  const char* id = in["id"] | "";
  const char* name = in["name"] | "Untitled";
  if (!id[0]) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "missing_id");
    return false;
  }

  JsonArray stepsIn = in["steps"].as<JsonArray>();
  if (stepsIn.isNull()) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "missing_steps");
    return false;
  }

  DynamicJsonDocument catalog(16384);
  loadCatalog(catalog);
  JsonArray items = catalog["items"].as<JsonArray>();

  for (size_t i = 0; i < items.size(); i++) {
    JsonObject it = items[i].as<JsonObject>();
    if (strcmp(it["id"] | "", id) == 0) {
      items.remove(i);
      break;
    }
  }

  JsonObject slot = items.createNestedObject();
  slot["id"] = id;
  slot["name"] = name;
  {
    String stepsStr;
    serializeJson(stepsIn, stepsStr);
    String wrap = "{\"steps\":";
    wrap += stepsStr;
    wrap += "}";
    DynamicJsonDocument tmp(4096);
    deserializeJson(tmp, wrap);
    slot["steps"] = tmp["steps"];
  }
  sortStepsArray(slot["steps"].as<JsonArray>());

  if (!saveCatalog(catalog)) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "save_failed");
    return false;
  }
  return true;
}
