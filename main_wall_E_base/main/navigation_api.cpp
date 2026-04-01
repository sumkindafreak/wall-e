// ============================================================
//  HTTP /api/navigation — upload routes, status, stop
//  Bridges web UI grid waypoints → GPS via existing waypoint_nav.
// ============================================================

#include "navigation_api.h"
#include "waypoint_nav.h"
#include "autonomy_engine.h"
#include "gps_module.h"
#include "compass_sensor.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include <math.h>

extern WebServer server;

static void navAddCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
}

#ifndef EARTH_RADIUS_M
#define EARTH_RADIUS_M 6371000.0
#endif

static void offsetMetersToLatLon(double originLat, double originLon, double eastM, double northM,
                                 double& outLat, double& outLon) {
  double latRad = originLat * M_PI / 180.0;
  outLat = originLat + (northM / EARTH_RADIUS_M) * (180.0 / M_PI);
  outLon = originLon + (eastM / (EARTH_RADIUS_M * cos(latRad))) * (180.0 / M_PI);
}

void navigationHandleRoutePost() {
  navAddCors();
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"expected JSON body\"}");
    return;
  }

  String body = server.arg("plain");
  if (body.length() == 0 || body.length() > 4096) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"body_size\"}");
    return;
  }

  if (!gpsHasFix()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no_gps_fix\"}");
    return;
  }
  if (!compassIsValid()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no_compass\"}");
    return;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"json_parse\"}");
    return;
  }

  const char* frame = doc["frame"] | "grid";
  double originLat = gpsGetLatitude();
  double originLon = gpsGetLongitude();

  waypointStopNavigation();
  waypointClearAll();

  uint8_t added = 0;

  JsonArray wps = doc["waypoints"].as<JsonArray>();
  if (wps.isNull() || wps.size() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_waypoints\"}");
    return;
  }

  if (strcmp(frame, "gps") == 0) {
    for (size_t i = 0; i < wps.size(); i++) {
      JsonObject wp = wps[i].as<JsonObject>();
      if (!wp.containsKey("lat") || !wp.containsKey("lon")) continue;
      double la = wp["lat"].as<double>();
      double lo = wp["lon"].as<double>();
      char name[16];
      snprintf(name, sizeof(name), "wp%u", (unsigned)added);
      if (waypointAdd(la, lo, name)) added++;
    }
  } else {
    // grid: meters per map unit, robot position, waypoints in same frame as UI (y down = south)
    float scale = doc["scale_m_per_unit"] | 0.5f;
    if (scale <= 0.01f || scale > 50.0f) scale = 0.5f;

    JsonObject rob = doc["robot"].as<JsonObject>();
    float rx = rob["x"] | 50.0f;
    float ry = rob["y"] | 50.0f;

    for (size_t i = 0; i < wps.size(); i++) {
      JsonObject wp = wps[i].as<JsonObject>();
      if (!wp.containsKey("x") || !wp.containsKey("y")) continue;
      float wx = wp["x"].as<float>();
      float wy = wp["y"].as<float>();
      double eastM = (double)(wx - rx) * (double)scale;
      double northM = (double)(ry - wy) * (double)scale;
      double la, lo;
      offsetMetersToLatLon(originLat, originLon, eastM, northM, la, lo);
      char name[16];
      snprintf(name, sizeof(name), "wp%u", (unsigned)added);
      if (waypointAdd(la, lo, name)) added++;
    }
  }

  if (added == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no_waypoints_added\"}");
    return;
  }

  waypointSave();
  waypointStartNavigation();
  autonomySetWaypointMode(true);
  autonomySetEnabled(true);

  char resp[96];
  snprintf(resp, sizeof(resp), "{\"ok\":true,\"count\":%u,\"frame\":\"%s\"}", (unsigned)added, frame);
  server.send(200, "application/json", resp);
}

void navigationHandleStatusGet() {
  navAddCors();
  const NavState* nav = waypointGetNavState();
  String j = "{\"ok\":true";
  j += ",\"gps_valid\":"; j += gpsHasFix() ? "true" : "false";
  j += ",\"compass_valid\":"; j += compassIsValid() ? "true" : "false";
  j += ",\"waypoint_count\":"; j += (int)waypointGetCount();
  j += ",\"navigating\":"; j += waypointIsNavigating() ? "true" : "false";
  j += ",\"waypoint_mode\":"; j += autonomyIsWaypointMode() ? "true" : "false";
  j += ",\"autonomy_enabled\":"; j += autonomyIsEnabled() ? "true" : "false";
  j += ",\"state\":\""; j += autonomyGetStateName(); j += "\"";
  if (nav) {
    j += ",\"distance_m\":"; j += String(nav->distanceToWaypoint, 2);
    j += ",\"bearing_deg\":"; j += String(nav->bearingToWaypoint, 1);
    j += ",\"current_index\":"; j += (int)waypointGetCurrent();
  }
  j += "}";
  server.send(200, "application/json", j);
}

void navigationHandleStopGet() {
  navAddCors();
  waypointStopNavigation();
  autonomySetWaypointMode(false);
  server.send(200, "application/json", "{\"ok\":true}");
}
