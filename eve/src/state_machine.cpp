/**
 * EVE companion state machine + handshake with WALL-E over UART.
 */
#include "state_machine.h"
#include "config.h"
#include "uart_link.h"
#include "eve_protocol.h"
#include <string.h>
#include "system_status.h"
#include "eyes_control.h"
#include "servo_control.h"
#include "neopixel_control.h"
#include "audio_control.h"
#include "power_monitor.h"
#include <ArduinoJson.h>

enum class EveState : uint8_t {
  BOOT,
  WAIT_ATTACH,
  HANDSHAKE,
  IDLE,
  ATTACHED,
  ESCORT,
  INTERACT,
  LOW_POWER,
  DOCKED,
  ERROR,
  SLEEP
};

static EveState s_state = EveState::BOOT;
static uint32_t s_sessionId = 0;
static uint32_t s_lastHelloMs = 0;
static uint32_t s_lastEveHbMs = 0;
static uint32_t s_lastRxMs = 0;
static uint32_t s_lastPowerStatusMs = 0;

static void sendEveHello(void) {
  StaticJsonDocument<256> doc;
  doc["proto"] = 1;
  doc["eve"] = "eve-s3";
  doc["caps"] = 0;
  doc["uptime_ms"] = systemStatusUptimeMs();
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_HELLO, out.c_str());
  Serial.println(F("[EVE][SM] -> EVE_HELLO"));
}

static void sendEveReady(void) {
  StaticJsonDocument<128> doc;
  doc["session"] = s_sessionId;
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_READY, out.c_str());
  Serial.println(F("[EVE][SM] -> EVE_READY"));
}

static void sendEveHeartbeat(void) {
  StaticJsonDocument<192> doc;
  doc["session"] = s_sessionId;
  doc["uptime_ms"] = systemStatusUptimeMs();
  doc["heap"] = ESP.getFreeHeap();
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_HEARTBEAT, out.c_str());
}

static void sendEveError(const char* msg, int code) {
  StaticJsonDocument<192> doc;
  doc["code"] = code;
  doc["msg"] = msg;
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_ERROR, out.c_str());
}

/** Transmit live power telemetry to WALL-E (JSON payload). */
static void sendEvePowerStatus(void) {
  const PowerStatus& ps = getPowerStatus();
  StaticJsonDocument<256> doc;
  doc["node"]  = "EVE";
  doc["v"]     = ps.voltage;
  doc["i"]     = ps.current;
  doc["pct"]   = ps.percent;
  doc["state"] = (uint8_t)ps.state;
  doc["chg"]   = ps.charging;
  doc["hb"]    = ps.heartbeat;
  doc["ts"]    = ps.timestamp_ms;
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_POWER_STATUS, out.c_str());
}

void stateMachineInit(void) {
  s_state = EveState::WAIT_ATTACH;
  s_lastHelloMs = 0;
  s_lastEveHbMs = 0;
  s_lastRxMs = millis();
  s_lastPowerStatusMs = 0;
  Serial.println(F("[EVE][SM] WAIT_ATTACH"));
}

void stateMachineTick(void) {
  uint32_t now = millis();

  switch (s_state) {
    case EveState::BOOT:
      break;
    case EveState::WAIT_ATTACH:
      if (now - s_lastHelloMs >= EVE_HELLO_RETRY_MS) {
        s_lastHelloMs = now;
        sendEveHello();
      }
      break;
    case EveState::HANDSHAKE:
      break;
    case EveState::IDLE:
    case EveState::ATTACHED:
    case EveState::ESCORT:
    case EveState::INTERACT:
    case EveState::DOCKED:
      if (now - s_lastEveHbMs >= EVE_HEARTBEAT_MS) {
        s_lastEveHbMs = now;
        sendEveHeartbeat();
      }
      /* Send power status at a separate (configurable) interval. */
      if (now - s_lastPowerStatusMs >= EVE_POWER_STATUS_INTERVAL_MS) {
        s_lastPowerStatusMs = now;
        sendEvePowerStatus();
      }
      if (s_state == EveState::ESCORT || s_state == EveState::INTERACT) {
        if (now - s_lastRxMs > EVE_LINK_LOST_MS) {
          Serial.println(F("[EVE][SM] link lost -> WAIT_ATTACH"));
          sendEveError("link_timeout", 1);
          s_state = EveState::WAIT_ATTACH;
          s_sessionId = 0;
          s_lastHelloMs = 0;
        }
      }
      break;
    case EveState::LOW_POWER:
    case EveState::ERROR:
    case EveState::SLEEP:
      break;
  }
}

void stateMachineOnUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
  (void)seq;
  s_lastRxMs = millis();

  char jsonBuf[EVE_MAX_PAYLOAD + 1];
  if (len > EVE_MAX_PAYLOAD) {
    return;
  }
  if (len > 0) {
    memcpy(jsonBuf, payload, len);
  }
  jsonBuf[len] = '\0';

  Serial.print(F("[EVE][SM] RX type=0x"));
  Serial.print(type, HEX);
  Serial.print(F(" seq="));
  Serial.println(seq);

  switch (type) {
    case MSG_WALL_E_ACK: {
      StaticJsonDocument<128> doc;
      if (deserializeJson(doc, jsonBuf)) {
        return;
      }
      s_sessionId = doc["session"] | 0u;
      Serial.print(F("[EVE][SM] WALL_E_ACK session="));
      Serial.println(s_sessionId);
      s_state = EveState::HANDSHAKE;
      sendEveReady();
      s_state = EveState::IDLE;
      neopixelSetPattern(1);
      break;
    }
    case MSG_ATTACH_CONFIRMED:
      s_state = EveState::ATTACHED;
      eyesSetMode(1);
      Serial.println(F("[EVE][SM] ATTACHED"));
      break;
    case MSG_MODE_ESCORT:
      s_state = EveState::ESCORT;
      Serial.println(F("[EVE][SM] ESCORT"));
      break;
    case MSG_MODE_DOCK:
      s_state = EveState::DOCKED;
      Serial.println(F("[EVE][SM] DOCK"));
      break;
    case MSG_MODE_IDLE:
      s_state = EveState::IDLE;
      Serial.println(F("[EVE][SM] IDLE"));
      break;
    case MSG_MOVE_SERVO: {
      StaticJsonDocument<128> doc;
      if (deserializeJson(doc, jsonBuf)) {
        break;
      }
      int l = doc["l"] | 90;
      int r = doc["r"] | 90;
      servoSetAngles((int16_t)l, (int16_t)r);
      break;
    }
    case MSG_PLAY_SOUND: {
      StaticJsonDocument<64> doc;
      if (deserializeJson(doc, jsonBuf)) {
        break;
      }
      uint8_t tr = doc["track"] | 1;
      audioPlayTrack(tr);
      break;
    }
    case MSG_STOP:
      servoSetAngles(90, 90);
      audioPlayTrack(0);
      break;
    case MSG_RESET_STATE:
      stateMachineInit();
      break;
    default:
      break;
  }
}
