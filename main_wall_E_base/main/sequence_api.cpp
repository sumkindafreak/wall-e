// ============================================================
//  HTTP API for LROS sequences (JSON catalog + run/stop)
// ============================================================

#include "sequence_api.h"
#include "sequence_engine.h"
#include "api_security.h"
#include <ArduinoJson.h>
#include <WebServer.h>

extern WebServer server;

static void seqCors(void) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
}

static void handleSequencesList(void) {
  seqCors();
  server.send(200, "application/json", sequenceListJSON());
}

static void handleSequencesGet(void) {
  seqCors();
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_id\"}");
    return;
  }
  String out;
  if (!sequenceGetOneJSON(server.arg("id").c_str(), out)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"not_found\"}");
    return;
  }
  server.send(200, "application/json", out);
}

static void handleSequencesSave(void) {
  seqCors();
  if (apiSecurityRejectIfBadToken()) return;
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"expected JSON body\"}");
    return;
  }
  String body = server.arg("plain");
  char err[48];
  if (!sequenceSaveJSON(body.c_str(), body.length(), err, sizeof(err))) {
    String j = "{\"ok\":false,\"error\":\"";
    j += err;
    j += "\"}";
    server.send(400, "application/json", j);
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSequencesDelete(void) {
  seqCors();
  if (apiSecurityRejectIfBadToken()) return;
  String id;
  if (server.hasArg("plain")) {
    StaticJsonDocument<256> doc;
    DeserializationError e = deserializeJson(doc, server.arg("plain"));
    if (e) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"json_parse\"}");
      return;
    }
    id = doc["id"] | "";
  } else if (server.hasArg("id")) {
    id = server.arg("id");
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_id\"}");
    return;
  }
  if (!id.length()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_id\"}");
    return;
  }
  if (!sequenceDelete(id.c_str())) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"not_found\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSequencesRun(void) {
  seqCors();
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_id\"}");
    return;
  }
  char err[48];
  if (!sequenceRun(server.arg("id").c_str(), err, sizeof(err))) {
    String j = "{\"ok\":false,\"error\":\"";
    j += err;
    j += "\"}";
    server.send(400, "application/json", j);
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSequencesStop(void) {
  seqCors();
  sequenceStop();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSequencesStatus(void) {
  seqCors();
  server.send(200, "application/json", sequenceGetStatusJSON());
}

void sequenceRegisterWebRoutes(void) {
  server.on("/api/sequences/list", HTTP_GET, handleSequencesList);
  server.on("/api/sequences/get", HTTP_GET, handleSequencesGet);
  server.on("/api/sequences/save", HTTP_POST, handleSequencesSave);
  server.on("/api/sequences/delete", HTTP_POST, handleSequencesDelete);
  server.on("/api/sequences/run", HTTP_GET, handleSequencesRun);
  server.on("/api/sequences/stop", HTTP_GET, handleSequencesStop);
  server.on("/api/sequences/status", HTTP_GET, handleSequencesStatus);
}
