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
#include "battery_monitor.h"
#include "servo_control.h"
#include "neopixel_control.h"
#include "audio_control.h"
#include "eve_behavior_manager.h"
#include "battery_monitor.h"
#include "eve_desktop_companion.h"
#include "eve_spatial_awareness.h"
#include "eve_attachment_manager.h"
#if EVE_ENABLE_EYES
#include "eve_expression_state.h"
#endif
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
static uint32_t s_lastLowBatMs = 0;
static uint32_t s_lastUnauthorizedLogMs = 0;
static char s_peerLabel[24] = "none";

static bool stateAllowsRemoteControl(void) {
  return (s_state == EveState::IDLE || s_state == EveState::ATTACHED || s_state == EveState::ESCORT ||
          s_state == EveState::INTERACT || s_state == EveState::DOCKED || s_state == EveState::LOW_POWER);
}

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
  StaticJsonDocument<384> doc;
  doc["session"] = s_sessionId;
  doc["uptime_ms"] = systemStatusUptimeMs();
  doc["heap"] = ESP.getFreeHeap();
  doc["bat_ok"] = eveBatteryDataValid();
  doc["bat_v"] = eveBatteryVoltage();
  doc["bat_a"] = eveBatteryCurrentA();
  doc["bat_pct"] = eveBatteryPercent();
  doc["bat_st"] = (int)eveBatteryStatus();
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_HEARTBEAT, out.c_str());
}

static void sendEveLowBatteryJson(void) {
  StaticJsonDocument<160> doc;
  doc["v"] = eveBatteryVoltage();
  doc["a"] = eveBatteryCurrentA();
  doc["pct"] = eveBatteryPercent();
  doc["session"] = s_sessionId;
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_LOW_BATTERY, out.c_str());
  Serial.println(F("[EVE][SM] -> MSG_EVE_LOW_BATTERY"));
}

static void batteryAlarmTick(uint32_t now) {
  if (!eveBatteryHardwareEnabled() || !eveBatteryDataValid()) return;

  if (eveBatteryPercent() > EVE_BAT_CRIT_PCT) {
    if (s_state == EveState::LOW_POWER && eveBatteryPercent() > (EVE_BAT_WARN_PCT + 5)) {
      Serial.println(F("[EVE][SM] battery recovered -> IDLE"));
      s_state = EveState::IDLE;
    }
    return;
  }

  if (now - s_lastLowBatMs < EVE_BAT_LOW_REPORT_MIN_MS) return;
  s_lastLowBatMs = now;
  sendEveLowBatteryJson();

  if (s_state == EveState::IDLE || s_state == EveState::ATTACHED || s_state == EveState::ESCORT ||
      s_state == EveState::INTERACT || s_state == EveState::DOCKED) {
    Serial.println(F("[EVE][SM] critical battery -> LOW_POWER"));
    s_state = EveState::LOW_POWER;
  }
}

static void sendEveError(const char* msg, int code) {
  StaticJsonDocument<192> doc;
  doc["code"] = code;
  doc["msg"] = msg;
  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_ERROR, out.c_str());
}

void stateMachineInit(void) {
  s_state = EveState::WAIT_ATTACH;
  s_sessionId = 0;
  strncpy(s_peerLabel, "none", sizeof(s_peerLabel) - 1);
  s_peerLabel[sizeof(s_peerLabel) - 1] = '\0';
  s_lastHelloMs = 0;
  s_lastEveHbMs = 0;
  s_lastRxMs = millis();
  eveDesktopCompanionInit();
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
      batteryAlarmTick(now);
      if (now - s_lastEveHbMs >= EVE_HEARTBEAT_MS) {
        s_lastEveHbMs = now;
        sendEveHeartbeat();
      }
      if (s_state == EveState::ESCORT || s_state == EveState::INTERACT) {
        if (now - s_lastRxMs > EVE_LINK_LOST_MS) {
          Serial.println(F("[EVE][SM] link lost -> WAIT_ATTACH"));
          sendEveError("link_timeout", 1);
          s_state = EveState::WAIT_ATTACH;
          s_sessionId = 0;
          strncpy(s_peerLabel, "none", sizeof(s_peerLabel) - 1);
          s_peerLabel[sizeof(s_peerLabel) - 1] = '\0';
          s_lastHelloMs = 0;
          eyesNotifyWallEDisconnected();
        }
      }
      break;
    case EveState::LOW_POWER:
      batteryAlarmTick(now);
      if (now - s_lastEveHbMs >= EVE_HEARTBEAT_MS) {
        s_lastEveHbMs = now;
        sendEveHeartbeat();
      }
      break;
    case EveState::ERROR:
    case EveState::SLEEP:
      break;
  }

  {
    uint8_t sf = 0;
    if (s_state == EveState::SLEEP) {
      sf |= EVE_SPATIAL_FLAG_SLEEP;
    }
    if (s_state == EveState::LOW_POWER) {
      sf |= EVE_SPATIAL_FLAG_LOW_BATTERY;
    }
    if (s_state == EveState::ESCORT || s_state == EveState::INTERACT) {
      sf |= EVE_SPATIAL_FLAG_ALERT;
    }
    if (eveAttachmentIsAttached()) {
      sf |= EVE_SPATIAL_FLAG_ATTACHED;
    }
#if EVE_ENABLE_EYES
    if (eveExpressionGetCurrent() == EVE_EXPR_CONFUSED) {
      sf |= EVE_SPATIAL_FLAG_CONFUSED;
    }
#endif
    eveSpatialSetBehaviorFlags(sf);
  }

  const bool dockDesktopActive = (s_state == EveState::DOCKED);
  const bool dockCharging = eveBatteryDataValid() && eveBatteryCurrentA() > 0.03f;
  eveDesktopCompanionSetActive(dockDesktopActive, dockCharging);
  if (dockDesktopActive) {
    eveDesktopCompanionTick(now);
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
      const char* peer = doc["peer"] | "wall_e";
      if (!peer || peer[0] == '\0') {
        peer = "wall_e";
      }
      strncpy(s_peerLabel, peer, sizeof(s_peerLabel) - 1);
      s_peerLabel[sizeof(s_peerLabel) - 1] = '\0';
      Serial.print(F("[EVE][SM] WALL_E_ACK session="));
      Serial.println(s_sessionId);
      Serial.print(F("[EVE][SM] peer="));
      Serial.println(s_peerLabel);
      s_state = EveState::HANDSHAKE;
      sendEveReady();
      s_state = EveState::IDLE;
      neopixelSetPattern(1);
      eyesNotifyWallEConnected();
      break;
    }
    case MSG_ATTACH_CONFIRMED:
      if (s_sessionId == 0) {
        break;
      }
      s_state = EveState::ATTACHED;
      eyesSetMode(1);
      Serial.println(F("[EVE][SM] ATTACHED"));
      break;
    case MSG_MODE_ESCORT:
      if (s_sessionId == 0) {
        break;
      }
      s_state = EveState::ESCORT;
      Serial.println(F("[EVE][SM] ESCORT"));
      break;
    case MSG_MODE_DOCK:
      if (s_sessionId == 0) {
        break;
      }
      s_state = EveState::DOCKED;
      Serial.println(F("[EVE][SM] DOCK"));
      eyesNotifyDockingState(true, eveBatteryDataValid() && eveBatteryCurrentA() > 0.03f);
      break;
    case MSG_MODE_IDLE:
      if (s_sessionId == 0) {
        break;
      }
      s_state = EveState::IDLE;
      Serial.println(F("[EVE][SM] IDLE"));
      eyesNotifyDockingState(false, false);
      break;
    case MSG_MOVE_SERVO: {
      if (s_sessionId == 0 || !stateAllowsRemoteControl()) {
        uint32_t now = millis();
        if ((uint32_t)(now - s_lastUnauthorizedLogMs) > 1000u) {
          s_lastUnauthorizedLogMs = now;
          Serial.println(F("[EVE][SM] Ignored MOVE_SERVO before handshake/state ready"));
        }
        break;
      }
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
      if (s_sessionId == 0 || !stateAllowsRemoteControl()) {
        uint32_t now = millis();
        if ((uint32_t)(now - s_lastUnauthorizedLogMs) > 1000u) {
          s_lastUnauthorizedLogMs = now;
          Serial.println(F("[EVE][SM] Ignored PLAY_SOUND before handshake/state ready"));
        }
        break;
      }
      StaticJsonDocument<64> doc;
      if (deserializeJson(doc, jsonBuf)) {
        break;
      }
      uint8_t tr = doc["track"] | 1;
      eveBehaviorOnRemoteSound(tr);
      break;
    }
    case MSG_STOP:
      if (s_sessionId == 0 || !stateAllowsRemoteControl()) {
        uint32_t now = millis();
        if ((uint32_t)(now - s_lastUnauthorizedLogMs) > 1000u) {
          s_lastUnauthorizedLogMs = now;
          Serial.println(F("[EVE][SM] Ignored STOP before handshake/state ready"));
        }
        break;
      }
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

uint32_t stateMachineGetSessionId(void) {
  return s_sessionId;
}

bool stateMachineAllowsCompanionUart(void) {
  if (s_sessionId == 0) {
    return false;
  }
  switch (s_state) {
    case EveState::IDLE:
    case EveState::ATTACHED:
    case EveState::ESCORT:
    case EveState::INTERACT:
    case EveState::DOCKED:
    case EveState::LOW_POWER:
      return true;
    default:
      return false;
  }
}

const char* stateMachineGetPeerLabel(void) {
  return s_peerLabel;
}
