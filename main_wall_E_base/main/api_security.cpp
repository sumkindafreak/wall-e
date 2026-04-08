#include "api_security.h"
#include "battery_monitor.h"
#include "motion_authority.h"
#include "node_health_registry.h"
#include "eve_uart_bridge.h"
#include "vision_behaviour.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "dock_homing.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

extern WebServer server;

static const char* kNs = "walle_sec";
static const char* kTok = "api_tok";

#define AUDIT_LINES 24
#define AUDIT_MAX 96
static char s_audit[AUDIT_LINES][AUDIT_MAX];
static uint8_t s_auditN = 0;

void apiSecurityInit(void) {
  memset(s_audit, 0, sizeof(s_audit));
  s_auditN = 0;
}

static void auditPush(const char* line) {
  if (!line) return;
  if (s_auditN < AUDIT_LINES) {
    strncpy(s_audit[s_auditN], line, AUDIT_MAX - 1);
    s_audit[s_auditN][AUDIT_MAX - 1] = '\0';
    s_auditN++;
  } else {
    memmove(s_audit[0], s_audit[1], sizeof(s_audit[0]) * (AUDIT_LINES - 1));
    strncpy(s_audit[AUDIT_LINES - 1], line, AUDIT_MAX - 1);
    s_audit[AUDIT_LINES - 1][AUDIT_MAX - 1] = '\0';
  }
}

void apiSecurityAudit(const char* line) {
  auditPush(line);
}

static String loadToken(void) {
  Preferences p;
  p.begin(kNs, true);
  String t = p.getString(kTok, "");
  p.end();
  return t;
}

bool apiSecurityTokenOk(void) {
  String want = loadToken();
  if (want.length() == 0) return true;

  String got = server.header("X-Wall-E-Token");
  if (got.length() == 0 && server.hasArg("token")) got = server.arg("token");
  return got == want;
}

bool apiSecurityRejectIfBadToken(void) {
  if (apiSecurityTokenOk()) return false;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\"}");
  return true;
}

bool apiSecurityApplyTokenBody(const char* body, size_t len, char* errBuf, size_t errLen) {
  if (errBuf && errLen) errBuf[0] = '\0';
  if (!body || len == 0 || len > 512) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "body");
    return false;
  }
  StaticJsonDocument<256> doc;
  DeserializationError je = deserializeJson(doc, body, len);
  if (je) {
    if (errBuf && errLen) snprintf(errBuf, errLen, "json");
    return false;
  }
  String want = loadToken();
  if (want.length() > 0) {
    const char* oldt = doc["old_token"] | "";
    if (strcmp(oldt, want.c_str()) != 0) {
      if (errBuf && errLen) snprintf(errBuf, errLen, "old_token");
      return false;
    }
  }
  const char* nt = doc["token"] | "";
  Preferences p;
  p.begin(kNs, false);
  if (nt[0] == '\0') {
    p.remove(kTok);
  } else {
    p.putString(kTok, nt);
  }
  p.end();
  auditPush("[api] token updated");
  return true;
}

String apiSecurityGetAuditJSON(void) {
  String j = "{\"ok\":true,\"lines\":[";
  for (uint8_t i = 0; i < s_auditN; i++) {
    if (i) j += ',';
    j += '"';
    for (const char* p = s_audit[i]; *p; p++) {
      if (*p == '"' || *p == '\\') j += '\\';
      j += *p;
    }
    j += '"';
  }
  j += "]}";
  return j;
}

String apiSecurityGetDashboardJSON(void) {
  String j = "{\"ok\":true";
  j += ",\"motion_policy\":\""; j += motionAuthorityModeName(motionAuthorityGet()); j += "\"";
  j += ",\"node_health\":"; j += nodeHealthGetJSON();
  j += ",\"eve\":"; j += eveUartBridgeGetJSON();
  j += ",\"vision_events\":"; j += visionGetEventsJSON();
  const BatteryData& b = batteryGetData();
  j += ",\"battery_percent\":"; j += String(b.percent);
#if USE_AUTONOMOUS_DOCKING
  j += ",\"dock_fsm\":\""; j += autonomousDockingGetStateName(); j += "\"";
#else
  j += ",\"dock_legacy_active\":"; j += dockHomingIsActive() ? "true" : "false";
#endif
  j += "}";
  return j;
}
