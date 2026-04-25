#include "eve_target_assist.h"
#include "eve_target_protocol.h"
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>

static uint32_t s_lastRxMs = 0;
static float s_bias = 0.f;
static EveAssistState s_state = ASSIST_NONE;
static uint32_t s_suppressMask = 0;
static bool s_tracking = false;
static char s_lastZone[16] = "NONE";

static const uint32_t kStaleMs = 1600u;
static const int16_t kMaxDelta = 22;

void eveTargetAssistInit(void) {
  s_lastRxMs = 0;
  s_bias = 0.f;
  s_state = ASSIST_NONE;
  s_suppressMask = 0;
  s_tracking = false;
  strcpy(s_lastZone, "NONE");
}

void eveTargetAssistSetSuppressMask(uint32_t mask) {
  s_suppressMask = mask;
}

static void logStale(void) {
  static uint32_t s_lastLog;
  uint32_t n = millis();
  if (n - s_lastLog > 2000u) {
    s_lastLog = n;
    Serial.println(F("[WALLE_ASSIST] Stale EVE target ignored"));
  }
}

void eveTargetAssistIngestJson(const char* jsonUtf8, uint32_t rxMillis) {
  if (!jsonUtf8 || !jsonUtf8[0]) {
    return;
  }
  DynamicJsonDocument doc(384);
  DeserializationError e = deserializeJson(doc, jsonUtf8);
  if (e) {
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.isNull()) {
    return;
  }

  s_lastRxMs = rxMillis;

  const char* z = obj[EVE_TARGET_JSON_ZONE] | "NONE";
  strncpy(s_lastZone, z, sizeof(s_lastZone) - 1);
  s_lastZone[sizeof(s_lastZone) - 1] = '\0';

  s_bias = obj[EVE_TARGET_JSON_BIAS] | 0.0;
  if (s_bias > 0.5f) {
    s_bias = 0.5f;
  }
  if (s_bias < -0.5f) {
    s_bias = -0.5f;
  }

  s_tracking = obj[EVE_TARGET_JSON_TRACKING] | false;

  if (!strcmp(s_lastZone, EVE_TARGET_ZONE_LEFT)) {
    s_state = ASSIST_BIAS_LEFT;
  } else if (!strcmp(s_lastZone, EVE_TARGET_ZONE_RIGHT)) {
    s_state = ASSIST_BIAS_RIGHT;
  } else if (!strcmp(s_lastZone, EVE_TARGET_ZONE_CENTER)) {
    s_state = ASSIST_ALIGN_CENTER;
  } else if (s_tracking) {
    s_state = ASSIST_HOLD_TARGET;
  } else {
    s_state = ASSIST_NONE;
  }

  static char s_prev[16] = "";
  if (strcmp(s_lastZone, s_prev) != 0 || s_tracking) {
    strncpy(s_prev, s_lastZone, sizeof(s_prev) - 1);
    s_prev[sizeof(s_prev) - 1] = '\0';
    Serial.printf("[WALLE_ASSIST] Received EVE target %s bias=%.2f track=%d\n", s_lastZone, (double)s_bias,
                  s_tracking ? 1 : 0);
  }
}

void eveTargetAssistTick(uint32_t nowMs) {
  if (s_suppressMask & EVE_ASSIST_MASK_SAFETY) {
    static uint32_t slog;
    if (nowMs - slog > 3000u) {
      slog = nowMs;
      Serial.println(F("[WALLE_ASSIST] Assist suppressed by safety"));
    }
    s_bias *= 0.5f;
    if (fabsf(s_bias) < 0.02f) {
      s_bias = 0.f;
    }
    s_state = ASSIST_SUPPRESSED_BY_SAFETY;
    return;
  }
  if (s_suppressMask & EVE_ASSIST_MASK_MANUAL) {
    s_state = ASSIST_SUPPRESSED_BY_MANUAL;
    s_bias *= 0.65f;
    if (fabsf(s_bias) < 0.02f) {
      s_bias = 0.f;
    }
    return;
  }
  if (s_suppressMask & EVE_ASSIST_MASK_DOCK) {
    s_bias *= 0.4f;
    if (fabsf(s_bias) < 0.02f) {
      s_bias = 0.f;
      s_state = ASSIST_NONE;
    }
    return;
  }

  if (s_lastRxMs == 0 || (nowMs - s_lastRxMs) > kStaleMs) {
    logStale();
    s_bias *= 0.85f;
    if (fabsf(s_bias) < 0.03f) {
      s_bias = 0.f;
      s_state = ASSIST_NONE;
    }
    return;
  }

  s_bias *= 0.995f;
}

EveAssistState eveTargetAssistGetState(void) {
  return s_state;
}

float eveTargetAssistGetTurnBias(void) {
  return s_bias;
}

static const char* assistStateName(EveAssistState s) {
  switch (s) {
    case ASSIST_NONE: return "NONE";
    case ASSIST_BIAS_LEFT: return "BIAS_LEFT";
    case ASSIST_BIAS_RIGHT: return "BIAS_RIGHT";
    case ASSIST_ALIGN_CENTER: return "ALIGN_CENTER";
    case ASSIST_HOLD_TARGET: return "HOLD_TARGET";
    case ASSIST_SUPPRESSED_BY_SAFETY: return "SUPPRESSED_SAFETY";
    case ASSIST_SUPPRESSED_BY_MANUAL: return "SUPPRESSED_MANUAL";
    default: return "UNKNOWN";
  }
}

String eveTargetAssistGetStatusJSON(void) {
  uint32_t now = millis();
  uint32_t age = s_lastRxMs ? (uint32_t)(now - s_lastRxMs) : 0u;
  bool stale = (s_lastRxMs == 0) || (age > kStaleMs);
  String j = "{\"state\":";
  j += (int)s_state;
  j += ",\"state_name\":\"";
  j += assistStateName(s_state);
  j += "\",\"zone\":\"";
  j += s_lastZone;
  j += "\",\"bias\":";
  j += String(s_bias, 4);
  j += ",\"tracking\":";
  j += s_tracking ? "true" : "false";
  j += ",\"last_rx_age_ms\":";
  j += (uint32_t)age;
  j += ",\"stale\":";
  j += stale ? "true" : "false";
  j += "}";
  return j;
}

void eveTargetAssistGetMotorDelta(int16_t* dLeft, int16_t* dRight) {
  if (!dLeft || !dRight) {
    return;
  }
  *dLeft = 0;
  *dRight = 0;

  if (s_suppressMask & (EVE_ASSIST_MASK_SAFETY | EVE_ASSIST_MASK_MANUAL | EVE_ASSIST_MASK_DOCK)) {
    return;
  }
  if (s_lastRxMs == 0 || (millis() - s_lastRxMs) > kStaleMs) {
    return;
  }

  int16_t mag = (int16_t)(fabsf(s_bias) * (float)kMaxDelta);
  if (mag < 2) {
    return;
  }
  if (s_bias < 0.f) {
    *dLeft = (int16_t)-mag;
    *dRight = mag;
  } else if (s_bias > 0.f) {
    *dLeft = mag;
    *dRight = (int16_t)-mag;
  }
}
