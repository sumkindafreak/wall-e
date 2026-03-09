// ============================================================
//  WALL-E OTA Manager
//  ArduinoOTA (port 3232) + Web /update handled in web_server
// ============================================================

#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <WiFi.h>

#define OTA_HOSTNAME  "wall-e-base"
#define OTA_PASSWORD  ""  /* Set non-empty for security */

void otaManagerInit() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (OTA_PASSWORD[0] != '\0') {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] ArduinoOTA ready (port 3232)");
}

void otaManagerHandle() {
  ArduinoOTA.handle();
}
