/**
 * Minimal EVE UART RX on WALL-E base — parses frames from EVE, exposes JSON for /api/eve/status.
 * Framing matches eve/src/uart_link.cpp (CRC-16-CCITT over ver..payload).
 */
#include "eve_uart_bridge.h"
#include "eve_target_assist.h"
#include <HardwareSerial.h>
#include <string.h>
#include <stdio.h>
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
  MSG_EVE_TARGET_AWARENESS = 0x08,
  MSG_PLAY_SOUND = 0x33,
  MSG_CYD_EVE_SERVO = 0x35,
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
static uint8_t s_txSeq = 0;
static uint8_t s_lastType = 0;
static uint32_t s_session = 0;
static char s_lastJson[400] = "";

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

static bool eveSendFrame(uint8_t type, const char* jsonUtf8) {
#if !EVE_BRIDGE_ENABLE
  (void)type;
  (void)jsonUtf8;
  return false;
#else
  if (!jsonUtf8) return false;
  size_t plen = strlen(jsonUtf8);
  if (plen > EVE_MAX_PAYLOAD) return false;
  uint8_t seq = ++s_txSeq;
  uint8_t buf[5 + EVE_MAX_PAYLOAD + 2];
  buf[0] = 0x01; /* EVE_FRAME_VER */
  buf[1] = type;
  buf[2] = (uint8_t)(plen & 0xFF);
  buf[3] = (uint8_t)((plen >> 8) & 0xFF);
  buf[4] = seq;
  if (plen > 0) memcpy(buf + 5, jsonUtf8, plen);
  uint16_t crc = crc16_ccitt(buf, 5 + plen);
  buf[5 + plen] = (uint8_t)(crc & 0xFF);
  buf[5 + plen + 1] = (uint8_t)((crc >> 8) & 0xFF);
  EveSerial.write(EVE_SOF0);
  EveSerial.write(EVE_SOF1);
  EveSerial.write(buf, 5 + plen + 2);
  return true;
#endif
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
  if (fType == MSG_EVE_HELLO) {
    if (s_session == 0)
      s_session = (uint32_t)(millis() ^ 0xA5A5u);
    StaticJsonDocument<64> doc;
    doc["session"] = s_session;
    String o;
    serializeJson(doc, o);
    if (eveSendFrame(MSG_WALL_E_ACK, o.c_str())) {
      Serial.printf("[EVE bridge] -> WALL_E_ACK session=%lu\n", (unsigned long)s_session);
    }
  }
  if (fLen > 0 && fLen < sizeof(fPayload)) {
    fPayload[fLen] = '\0';
    strncpy(s_lastJson, (const char*)fPayload, sizeof(s_lastJson) - 1);
    if (fType == MSG_EVE_TARGET_AWARENESS) {
      eveTargetAssistIngestJson(s_lastJson, millis());
    } else {
      StaticJsonDocument<256> doc;
      DeserializationError jer = deserializeJson(doc, s_lastJson);
      if (!jer && doc.containsKey("session")) s_session = (uint32_t)(doc["session"] | 0);
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
  uint32_t now = millis();
  if (s_lastRxMs != 0 && (now - s_lastRxMs > 25000u))
    s_session = 0;
#endif
}

static const char* typeName(uint8_t t) {
  switch (t) {
    case MSG_EVE_HELLO: return "EVE_HELLO";
    case MSG_WALL_E_ACK: return "WALL_E_ACK";
    case MSG_EVE_READY: return "EVE_READY";
    case MSG_EVE_HEARTBEAT: return "EVE_HEARTBEAT";
    case MSG_EVE_LOW_BATTERY: return "EVE_LOW_BATTERY";
    case MSG_EVE_SLEEP: return "EVE_SLEEP";
    case MSG_EVE_ERROR: return "EVE_ERROR";
    case MSG_EVE_TARGET_AWARENESS: return "EVE_TARGET_AWARENESS";
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

bool eveUartBridgeIsLinkUp(void) {
  uint32_t now = millis();
  return (s_lastRxMs != 0) && ((now - s_lastRxMs) < 12000u);
}

bool eveUartBridgeSendPlaySound(uint8_t track) {
#if !EVE_BRIDGE_ENABLE
  (void)track;
  return false;
#else
  char buf[48];
  snprintf(buf, sizeof(buf), "{\"track\":%u}", (unsigned)track);
  Serial.printf("[EVE] -> PLAY_SOUND tr=%u\n", (unsigned)track);
  return eveSendFrame(MSG_PLAY_SOUND, buf);
#endif
}

bool eveUartBridgeSendCydServo(uint8_t headPanDeg, uint8_t rightArmDeg) {
#if !EVE_BRIDGE_ENABLE
  (void)headPanDeg;
  (void)rightArmDeg;
  return false;
#else
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"h\":%u,\"a\":%u}", (unsigned)headPanDeg, (unsigned)rightArmDeg);
  return eveSendFrame(MSG_CYD_EVE_SERVO, buf);
#endif
}

bool eveUartBridgeSendWallEAck(uint32_t session) {
#if !EVE_BRIDGE_ENABLE
  (void)session;
  return false;
#else
  StaticJsonDocument<64> doc;
  doc["session"] = session;
  String o;
  serializeJson(doc, o);
  return eveSendFrame(MSG_WALL_E_ACK, o.c_str());
#endif
}
