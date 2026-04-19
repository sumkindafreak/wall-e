/**
 * Minimal EVE UART RX on WALL-E base — parses frames from EVE, exposes JSON for /api/eve/status.
 * Framing matches eve/src/uart_link.cpp (CRC-16-CCITT over ver..payload).
 */
#include "eve_uart_bridge.h"
#include <HardwareSerial.h>
#include <string.h>
#include <ArduinoJson.h>

#ifndef EVE_BRIDGE_ENABLE
#define EVE_BRIDGE_ENABLE 1
#endif

static const uint8_t EVE_SOF0 = 0xA5;
static const uint8_t EVE_SOF1 = 0x5A;
static const uint16_t EVE_MAX_PAYLOAD = 512;

enum EveMsgType : uint8_t {
  MSG_EVE_HELLO = 0x01,
  MSG_WALL_E_ACK = 0x02,
  MSG_EVE_READY = 0x03,
  MSG_EVE_HEARTBEAT = 0x04,
  MSG_EVE_LOW_BATTERY = 0x05,
  MSG_EVE_SLEEP = 0x06,
  MSG_EVE_ERROR = 0x07,
  MSG_EVE_POWER_STATUS = 0x08,   /* live battery telemetry from EVE */
};

enum ParseState : uint8_t {
  ST_SOF0 = 0,
  ST_SOF1,
  ST_VER,
  ST_TYPE,
  ST_LEN_LO,
  ST_LEN_HI,
  ST_SEQ,
  ST_PAYLOAD,
  ST_CRC_LO,
  ST_CRC_HI
};

static HardwareSerial EveSerial(2);
static ParseState st = ST_SOF0;
static uint8_t fVer = 0;
static uint8_t fType = 0;
static uint16_t fLen = 0;
static uint8_t fSeq = 0;
static uint16_t fIdx = 0;
static uint8_t fPayload[512];
static uint8_t fCrcLo = 0;
static uint8_t fCrcHi = 0;

static uint32_t s_framesOk = 0;
static uint32_t s_crcErr = 0;
static uint32_t s_lastRxMs = 0;
static uint8_t s_lastType = 0;
static uint32_t s_session = 0;
static char s_lastJson[192] = "";

/* ---- EVE power telemetry ---- */
static EvePowerTelemetry s_powerTelem = {};
static EveBatteryState   s_prevBatState = EveBatteryState::EVE_BAT_OK;

static uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
      else crc <<= 1;
    }
  }
  return crc;
}

static void resetParser(void) {
  st = ST_SOF0;
}

static void onFrame(void) {
  uint8_t buf[5 + EVE_MAX_PAYLOAD];
  buf[0] = fVer;
  buf[1] = fType;
  buf[2] = (uint8_t)(fLen & 0xFF);
  buf[3] = (uint8_t)((fLen >> 8) & 0xFF);
  buf[4] = fSeq;
  if (fLen > 0) memcpy(buf + 5, fPayload, fLen);
  uint16_t calc = crc16_ccitt(buf, (size_t)(5 + fLen));
  uint16_t wire = (uint16_t)fCrcLo | ((uint16_t)fCrcHi << 8);
  if (calc != wire) {
    s_crcErr++;
    resetParser();
    return;
  }
  s_framesOk++;
  s_lastRxMs = millis();
  s_lastType = fType;
  s_lastJson[0] = '\0';
  if (fLen > 0 && fLen < sizeof(fPayload)) {
    fPayload[fLen] = '\0';
    strncpy(s_lastJson, (const char*)fPayload, sizeof(s_lastJson) - 1);

    /* Parse JSON fields common to multiple message types. */
    StaticJsonDocument<256> doc;
    DeserializationError jer = deserializeJson(doc, s_lastJson);

    if (!jer) {
      if (doc.containsKey("session")) s_session = (uint32_t)(doc["session"] | 0);

      /* ---- Handle power status packet ---- */
      if (fType == (uint8_t)MSG_EVE_POWER_STATUS) {
        EvePowerTelemetry t;
        t.voltage          = doc["v"]   | 0.0f;
        t.current          = doc["i"]   | 0.0f;
        t.percent          = (uint8_t)(doc["pct"]  | 0);
        t.state            = (EveBatteryState)(uint8_t)(doc["state"] | 0);
        t.charging         = doc["chg"] | false;
        t.heartbeat        = (uint32_t)(doc["hb"]  | 0);
        t.eveTimestamp_ms  = (uint32_t)(doc["ts"]  | 0);
        t.receivedAt_ms    = s_lastRxMs;
        t.valid            = true;

        Serial.printf("[WALL-E][EVEPwr] V=%.2fV I=%.3fA pct=%u%% chg=%d state=%u hb=%lu\n",
                      t.voltage, t.current, t.percent,
                      (int)t.charging, (uint8_t)t.state, (unsigned long)t.heartbeat);

        /* Log and react to battery state changes. */
        if (!s_powerTelem.valid || t.state != s_prevBatState) {
          const char* labels[] = { "OK", "LOW", "CRITICAL", "CHARGING", "FULL" };
          uint8_t idx = (uint8_t)t.state;
          const char* label = (idx < 5) ? labels[idx] : "UNKNOWN";

          if (s_powerTelem.valid) {
            uint8_t prevIdx = (uint8_t)s_prevBatState;
            const char* prevLabel = (prevIdx < 5) ? labels[prevIdx] : "UNKNOWN";
            Serial.printf("[WALL-E][EVEPwr] State change: %s -> %s\n", prevLabel, label);
          } else {
            Serial.printf("[WALL-E][EVEPwr] Initial state: %s\n", label);
          }

          s_prevBatState = t.state;

          switch (t.state) {
            case EveBatteryState::EVE_BAT_OK:       onEveBatteryOk();       break;
            case EveBatteryState::EVE_BAT_LOW:      onEveBatteryLow();      break;
            case EveBatteryState::EVE_BAT_CRITICAL: onEveBatteryCritical(); break;
            case EveBatteryState::EVE_BAT_CHARGING: onEveCharging();        break;
            case EveBatteryState::EVE_BAT_FULL:     onEveBatteryFull();     break;
            default: break;
          }
        }

        s_powerTelem = t;
      }
    }
  }
  resetParser();
}

static void feedByte(uint8_t c) {
  switch (st) {
    case ST_SOF0:
      if (c == EVE_SOF0) st = ST_SOF1;
      break;
    case ST_SOF1:
      if (c == EVE_SOF1) st = ST_VER;
      else st = ST_SOF0;
      break;
    case ST_VER:
      fVer = c;
      st = ST_TYPE;
      break;
    case ST_TYPE:
      fType = c;
      st = ST_LEN_LO;
      break;
    case ST_LEN_LO:
      fLen = c;
      st = ST_LEN_HI;
      break;
    case ST_LEN_HI:
      fLen |= (uint16_t)c << 8;
      if (fLen > EVE_MAX_PAYLOAD) {
        s_crcErr++;
        resetParser();
        return;
      }
      st = ST_SEQ;
      break;
    case ST_SEQ:
      fSeq = c;
      fIdx = 0;
      st = (fLen == 0) ? ST_CRC_LO : ST_PAYLOAD;
      break;
    case ST_PAYLOAD:
      fPayload[fIdx++] = c;
      if (fIdx >= fLen) st = ST_CRC_LO;
      break;
    case ST_CRC_LO:
      fCrcLo = c;
      st = ST_CRC_HI;
      break;
    case ST_CRC_HI:
      fCrcHi = c;
      onFrame();
      break;
    default:
      resetParser();
      break;
  }
}

void eveUartBridgeInit(void) {
#if EVE_BRIDGE_ENABLE
  EveSerial.begin(EVE_BRIDGE_BAUD, SERIAL_8N1, EVE_BRIDGE_UART_RX, EVE_BRIDGE_UART_TX);
  EveSerial.setRxBufferSize(1024);
  st = ST_SOF0;
  Serial.printf("[EVE bridge] UART2 RX=%d TX=%d baud=%lu\n", EVE_BRIDGE_UART_RX, EVE_BRIDGE_UART_TX,
                (unsigned long)EVE_BRIDGE_BAUD);
#endif
}

void eveUartBridgePoll(void) {
#if EVE_BRIDGE_ENABLE
  while (EveSerial.available() > 0) {
    feedByte((uint8_t)EveSerial.read());
  }
#endif
}

static const char* typeName(uint8_t t) {
  switch (t) {
    case MSG_EVE_HELLO:         return "EVE_HELLO";
    case MSG_WALL_E_ACK:        return "WALL_E_ACK";
    case MSG_EVE_READY:         return "EVE_READY";
    case MSG_EVE_HEARTBEAT:     return "EVE_HEARTBEAT";
    case MSG_EVE_LOW_BATTERY:   return "EVE_LOW_BATTERY";
    case MSG_EVE_SLEEP:         return "EVE_SLEEP";
    case MSG_EVE_ERROR:         return "EVE_ERROR";
    case MSG_EVE_POWER_STATUS:  return "EVE_POWER_STATUS";
    default: return "unknown";
  }
}

String eveUartBridgeGetJSON(void) {
  uint32_t now = millis();
  bool linkOk = (s_lastRxMs != 0) && ((now - s_lastRxMs) < 12000u);
  String j = "{\"ok\":true";
  j += ",\"enabled\":true";
  j += ",\"link_ok\":"; j += linkOk ? "true" : "false";
  j += ",\"last_rx_age_ms\":"; j += (s_lastRxMs ? (uint32_t)(now - s_lastRxMs) : 0);
  j += ",\"frames_ok\":"; j += (uint32_t)s_framesOk;
  j += ",\"crc_errors\":"; j += (uint32_t)s_crcErr;
  j += ",\"last_type\":\""; j += typeName(s_lastType); j += "\"";
  j += ",\"last_type_hex\":\"0x";
  if (s_lastType < 16) j += '0';
  j += String(s_lastType, HEX);
  j += "\"";
  j += ",\"session\":"; j += (uint32_t)s_session;
  j += ",\"payload\":";
  if (s_lastJson[0]) {
    j += '"';
    for (const char* p = s_lastJson; *p; p++) {
      if (*p == '"' || *p == '\\') j += '\\';
      j += *p;
    }
    j += '"';
  } else {
    j += "null";
  }
  j += "}";
  return j;
}

// ---------------------------------------------------------------------------
// Power telemetry getters
// ---------------------------------------------------------------------------

const EvePowerTelemetry& eveGetPowerTelemetry(void) {
  return s_powerTelem;
}

bool eveIsPowerTelemetryFresh(void) {
  if (!s_powerTelem.valid) return false;
  uint32_t age = millis() - s_powerTelem.receivedAt_ms;
  return age < (uint32_t)EVE_POWER_TELEM_STALE_MS;
}

String eveGetPowerTelemetryJSON(void) {
  const EvePowerTelemetry& t = s_powerTelem;
  bool fresh = eveIsPowerTelemetryFresh();

  String j = "{";
  j += "\"valid\":";     j += t.valid  ? "true" : "false";
  j += ",\"fresh\":";    j += fresh    ? "true" : "false";
  j += ",\"age_ms\":";   j += t.valid  ? (uint32_t)(millis() - t.receivedAt_ms) : 0;
  j += ",\"voltage\":";  j += String(t.voltage, 2);
  j += ",\"current\":";  j += String(t.current, 3);
  j += ",\"percent\":";  j += (uint32_t)t.percent;
  j += ",\"state\":";    j += (uint32_t)(uint8_t)t.state;
  j += ",\"charging\":"; j += t.charging ? "true" : "false";
  j += ",\"heartbeat\":"; j += (uint32_t)t.heartbeat;
  j += ",\"ts_eve\":";   j += (uint32_t)t.eveTimestamp_ms;
  j += "}";
  return j;
}

// ---------------------------------------------------------------------------
// WALL-E reaction hooks for EVE battery state changes.
// These are called automatically when EVE sends a power-status packet with a
// different state than the previous one.
//
// Default implementation: serial logging + placeholder comments.
// Override by replacing these functions or extending them with your own code.
// ---------------------------------------------------------------------------

void onEveBatteryOk(void) {
  Serial.println(F("[WALL-E][EVEPwr] EVE battery OK — normal operation resumed"));
  /* Future: cancel any low-battery behaviour, resume normal motion profile. */
}

void onEveBatteryLow(void) {
  Serial.println(F("[WALL-E][EVEPwr] EVE battery LOW — begin return-to-dock logic"));
  /* Future: display warning on TFT, play alert sound, guide EVE toward dock,
     reduce WALL-E motion to preserve time for escort/docking sequence. */
}

void onEveBatteryCritical(void) {
  Serial.println(F("[WALL-E][EVEPwr] EVE battery CRITICAL — emergency dock protocol"));
  /* Future: highest-priority dock command, play urgent audio, pause all other
     tasks until EVE is confirmed docked / charging. */
}

void onEveCharging(void) {
  Serial.println(F("[WALL-E][EVEPwr] EVE is CHARGING — stand by"));
  /* Future: display charging indicator, play calm sound, disable escort state,
     enter idle/sentry mode while EVE recovers. */
}

void onEveBatteryFull(void) {
  Serial.println(F("[WALL-E][EVEPwr] EVE battery FULL — ready for action"));
  /* Future: celebrate animation, re-enable escort / full-autonomy mode,
     play ready sound, send MSG_MODE_ESCORT or MSG_ATTACH_CONFIRMED to EVE. */
}
