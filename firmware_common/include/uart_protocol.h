#pragma once

/*
 * Unified UART intelligence bus.
 *
 * Additive layer for WALL-E, EVE, and the EVE dock. It keeps the existing
 * framed UART shape already used by EVE:
 *   SOF(2) VER(1) TYPE(1) LEN(2 LE) SEQ(1) JSON_PAYLOAD LEN bytes CRC16(2 LE)
 *
 * No pins, transports, or existing command IDs are changed here. New bus packet
 * types live in the extension range 0x40..0x7F.
 */

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef UART_BUS_MAX_PAYLOAD
#define UART_BUS_MAX_PAYLOAD 512u
#endif

#define UART_BUS_SOF0 0xA5u
#define UART_BUS_SOF1 0x5Au
#define UART_BUS_FRAME_VERSION 0x01u

enum UartBusDevice : uint8_t {
  UART_BUS_DEVICE_UNKNOWN = 0,
  UART_BUS_DEVICE_WALLE = 1,
  UART_BUS_DEVICE_EVE = 2,
  UART_BUS_DEVICE_DOCK = 3,
  UART_BUS_DEVICE_BROADCAST = 255
};

enum UartBusRole : uint8_t {
  UART_BUS_ROLE_DIRECT_PEER = 0,
  UART_BUS_ROLE_MASTER = 1,
  UART_BUS_ROLE_BRIDGE_NODE = 2,
  UART_BUS_ROLE_ENVIRONMENT_NODE = 3
};

enum UartBusPacketType : uint8_t {
  UART_BUS_HELLO = 0x40,
  UART_BUS_IDENTIFY = 0x41,
  UART_BUS_STATUS = 0x42,
  UART_BUS_MEMORY_SYNC = 0x43,
  UART_BUS_COMMAND = 0x44,
  UART_BUS_EVENT = 0x45,
  UART_BUS_ACK = 0x46,
  UART_BUS_NACK = 0x47
};

enum UartBusMemorySyncPhase : uint8_t {
  UART_BUS_SYNC_BEGIN = 0,
  UART_BUS_SYNC_CHUNK = 1,
  UART_BUS_SYNC_END = 2,
  UART_BUS_SYNC_ABORT = 3
};

struct UartBusFrame {
  uint8_t version;
  uint8_t type;
  uint16_t len;
  uint8_t seq;
  uint8_t payload[UART_BUS_MAX_PAYLOAD + 1u];
};

typedef void (*UartBusFrameCallback)(const UartBusFrame& frame, void* user);

static inline const char* uartBusDeviceName(UartBusDevice device) {
  switch (device) {
    case UART_BUS_DEVICE_WALLE: return "WALL_E";
    case UART_BUS_DEVICE_EVE: return "EVE";
    case UART_BUS_DEVICE_DOCK: return "DOCK";
    case UART_BUS_DEVICE_BROADCAST: return "BROADCAST";
    default: return "UNKNOWN";
  }
}

static inline const char* uartBusPacketName(uint8_t type) {
  switch (type) {
    case UART_BUS_HELLO: return "HELLO";
    case UART_BUS_IDENTIFY: return "IDENTIFY";
    case UART_BUS_STATUS: return "STATUS";
    case UART_BUS_MEMORY_SYNC: return "MEMORY_SYNC";
    case UART_BUS_COMMAND: return "COMMAND";
    case UART_BUS_EVENT: return "EVENT";
    case UART_BUS_ACK: return "ACK";
    case UART_BUS_NACK: return "NACK";
    default: return "UNKNOWN";
  }
}

static inline uint16_t uartBusCrc16Ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFFu;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

static inline bool uartBusWriteJsonFrame(Print& out, uint8_t type, uint8_t seq, const char* json) {
  if (!json) return false;
  const size_t plen = strlen(json);
  if (plen > UART_BUS_MAX_PAYLOAD) return false;

  uint8_t frame[5 + UART_BUS_MAX_PAYLOAD];
  frame[0] = UART_BUS_FRAME_VERSION;
  frame[1] = type;
  frame[2] = (uint8_t)(plen & 0xFFu);
  frame[3] = (uint8_t)((plen >> 8) & 0xFFu);
  frame[4] = seq;
  if (plen) memcpy(frame + 5, json, plen);
  const uint16_t crc = uartBusCrc16Ccitt(frame, 5 + plen);
  const uint8_t crcBuf[2] = {(uint8_t)(crc & 0xFFu), (uint8_t)((crc >> 8) & 0xFFu)};

  out.write((uint8_t)UART_BUS_SOF0);
  out.write((uint8_t)UART_BUS_SOF1);
  out.write(frame, 5 + plen);
  out.write(crcBuf, sizeof(crcBuf));
  return true;
}

static inline bool uartBusWriteBufferedJsonFrame(Print& out, uint8_t type, uint8_t seq, const char* json) {
  if (!json) return false;
  const size_t plen = strlen(json);
  if (plen > UART_BUS_MAX_PAYLOAD) return false;

  uint8_t frame[5 + UART_BUS_MAX_PAYLOAD];
  frame[0] = UART_BUS_FRAME_VERSION;
  frame[1] = type;
  frame[2] = (uint8_t)(plen & 0xFFu);
  frame[3] = (uint8_t)((plen >> 8) & 0xFFu);
  frame[4] = seq;
  if (plen) memcpy(frame + 5, json, plen);
  const uint16_t crc = uartBusCrc16Ccitt(frame, 5 + plen);
  const uint8_t crcBuf[2] = {(uint8_t)(crc & 0xFFu), (uint8_t)((crc >> 8) & 0xFFu)};

  out.write((uint8_t)UART_BUS_SOF0);
  out.write((uint8_t)UART_BUS_SOF1);
  out.write(frame, 5 + plen);
  out.write(crcBuf, sizeof(crcBuf));
  return true;
}

static inline void uartBusMakeHelloJson(char* out, size_t outLen, UartBusDevice src, UartBusRole role,
                                        uint32_t session, const char* firmwareName) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen,
           "{\"src\":\"%s\",\"role\":%u,\"session\":%lu,\"fw\":\"%s\"}",
           uartBusDeviceName(src), (unsigned)role, (unsigned long)session,
           firmwareName ? firmwareName : "");
}

static inline void uartBusMakeStatusJson(char* out, size_t outLen, UartBusDevice src, const char* state,
                                         uint32_t uptimeMs, const char* extraJsonFields) {
  if (!out || outLen == 0) return;
  snprintf(out, outLen,
           "{\"src\":\"%s\",\"state\":\"%s\",\"uptime_ms\":%lu%s%s}",
           uartBusDeviceName(src), state ? state : "unknown", (unsigned long)uptimeMs,
           (extraJsonFields && *extraJsonFields) ? "," : "",
           (extraJsonFields && *extraJsonFields) ? extraJsonFields : "");
}

class UartBusParser {
 public:
  UartBusParser() { reset(); }

  void setCallback(UartBusFrameCallback cb, void* user) {
    cb_ = cb;
    user_ = user;
  }

  void reset() {
    state_ = ST_SOF0;
    frame_ = {};
    idx_ = 0;
    crcLo_ = 0;
  }

  void feed(uint8_t byte) {
    switch (state_) {
      case ST_SOF0:
        if (byte == UART_BUS_SOF0) state_ = ST_SOF1;
        break;
      case ST_SOF1:
        state_ = (byte == UART_BUS_SOF1) ? ST_VER : ST_SOF0;
        break;
      case ST_VER:
        frame_.version = byte;
        state_ = ST_TYPE;
        break;
      case ST_TYPE:
        frame_.type = byte;
        state_ = ST_LEN_LO;
        break;
      case ST_LEN_LO:
        frame_.len = byte;
        state_ = ST_LEN_HI;
        break;
      case ST_LEN_HI:
        frame_.len |= (uint16_t)byte << 8;
        if (frame_.version != UART_BUS_FRAME_VERSION || frame_.len > UART_BUS_MAX_PAYLOAD) {
          reset();
          return;
        }
        idx_ = 0;
        state_ = ST_SEQ;
        break;
      case ST_SEQ:
        frame_.seq = byte;
        state_ = frame_.len ? ST_PAYLOAD : ST_CRC_LO;
        break;
      case ST_PAYLOAD:
        frame_.payload[idx_++] = byte;
        if (idx_ >= frame_.len) state_ = ST_CRC_LO;
        break;
      case ST_CRC_LO:
        crcLo_ = byte;
        state_ = ST_CRC_HI;
        break;
      case ST_CRC_HI:
        deliverIfValid(byte);
        reset();
        break;
    }
  }

  void poll(Stream& in, uint16_t maxBytes) {
    uint16_t count = 0;
    while (in.available() > 0 && count++ < maxBytes) {
      feed((uint8_t)in.read());
    }
  }

 private:
  enum State : uint8_t {
    ST_SOF0,
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

  void deliverIfValid(uint8_t crcHi) {
    uint8_t buf[5 + UART_BUS_MAX_PAYLOAD];
    buf[0] = frame_.version;
    buf[1] = frame_.type;
    buf[2] = (uint8_t)(frame_.len & 0xFFu);
    buf[3] = (uint8_t)((frame_.len >> 8) & 0xFFu);
    buf[4] = frame_.seq;
    if (frame_.len) memcpy(buf + 5, frame_.payload, frame_.len);

    const uint16_t expected = uartBusCrc16Ccitt(buf, 5 + frame_.len);
    const uint16_t actual = (uint16_t)crcLo_ | ((uint16_t)crcHi << 8);
    if (expected != actual) return;

    frame_.payload[frame_.len] = '\0';
    if (cb_) cb_(frame_, user_);
  }

  State state_;
  UartBusFrame frame_;
  uint16_t idx_;
  uint8_t crcLo_;
  UartBusFrameCallback cb_ = nullptr;
  void* user_ = nullptr;
};

class UartBusArbiter {
 public:
  void begin(UartBusDevice localDevice, UartBusRole role) {
    localDevice_ = localDevice;
    role_ = role;
    lastRxMs_ = 0;
    nextTxAllowedMs_ = 0;
  }

  void setRole(UartBusRole role) { role_ = role; }
  UartBusRole role() const { return role_; }

  void noteRx(uint32_t nowMs) { lastRxMs_ = nowMs; }

  bool canTransmit(uint32_t nowMs) const {
    if (nowMs < nextTxAllowedMs_) return false;
    if (role_ == UART_BUS_ROLE_MASTER || role_ == UART_BUS_ROLE_BRIDGE_NODE) return true;
    return lastRxMs_ == 0 || (uint32_t)(nowMs - lastRxMs_) > quietWindowMs_;
  }

  void markTransmit(uint32_t nowMs, uint8_t seq) {
    const uint16_t stagger = (uint16_t)(((uint8_t)localDevice_ * 13u + seq * 7u) % 35u);
    nextTxAllowedMs_ = nowMs + interFrameMs_ + stagger;
  }

  void setTiming(uint16_t quietWindowMs, uint16_t interFrameMs) {
    quietWindowMs_ = quietWindowMs;
    interFrameMs_ = interFrameMs;
  }

 private:
  UartBusDevice localDevice_ = UART_BUS_DEVICE_UNKNOWN;
  UartBusRole role_ = UART_BUS_ROLE_DIRECT_PEER;
  uint32_t lastRxMs_ = 0;
  uint32_t nextTxAllowedMs_ = 0;
  uint16_t quietWindowMs_ = 20;
  uint16_t interFrameMs_ = 8;
};

