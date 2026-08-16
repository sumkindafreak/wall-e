#pragma once

// ============================================================
// WALL-E P4 <-> radio gateway framed transport
//
// The ESP32-P4 has no native ESP-NOW radio. A dedicated radio MCU
// (ESP32-C6/C3/S3) owns ESP-NOW and moves opaque packets to/from the
// P4 over UART. Robot packet formats stay unchanged.
// ============================================================

#include <stdint.h>
#include <stddef.h>

#define WALLE_RADIO_BRIDGE_SOF              0xA55Au
#define WALLE_RADIO_BRIDGE_VERSION          1u
#define WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES  250u
#define WALLE_RADIO_BRIDGE_MAX_PAYLOAD      272u

#define WALLE_RADIO_FLAG_BROADCAST 0x01u

enum WalleRadioBridgeType : uint8_t {
  WALLE_RADIO_MSG_NONE        = 0,
  WALLE_RADIO_MSG_RX_PACKET   = 1, // gateway -> P4
  WALLE_RADIO_MSG_TX_PACKET   = 2, // P4 -> gateway
  WALLE_RADIO_MSG_STATUS      = 3, // gateway -> P4
  WALLE_RADIO_MSG_SET_CHANNEL = 4, // P4 -> gateway
  WALLE_RADIO_MSG_HEARTBEAT   = 5  // either direction
};

struct __attribute__((packed)) WalleRadioBridgeHeader {
  uint16_t sof;
  uint8_t  version;
  uint8_t  type;
  uint16_t sequence;
  uint16_t payloadLength;
};

struct __attribute__((packed)) WalleRadioRxMeta {
  uint8_t  sourceMac[6];
  int8_t   rssi;
  uint8_t  channel;
  uint16_t radioLength;
};

struct __attribute__((packed)) WalleRadioTxMeta {
  uint8_t  destinationMac[6];
  uint8_t  channel;       // 0 = gateway's current channel
  uint8_t  flags;
  uint16_t radioLength;
};

struct __attribute__((packed)) WalleRadioGatewayStatus {
  uint8_t  ready;
  uint8_t  channel;
  uint8_t  peerCount;
  uint8_t  reserved;
  uint32_t uptimeMs;
  uint32_t rxPackets;
  uint32_t txPackets;
  uint32_t txFailures;
};

static_assert(sizeof(WalleRadioBridgeHeader) == 8, "Bridge header layout changed");
static_assert(sizeof(WalleRadioRxMeta) == 10, "RX metadata layout changed");
static_assert(sizeof(WalleRadioTxMeta) == 10, "TX metadata layout changed");

// CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF.
static inline uint16_t walleRadioCrc16(const uint8_t* data, size_t length,
                                      uint16_t crc = 0xFFFFu) {
  if (!data) return crc;
  for (size_t i = 0; i < length; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                            : (uint16_t)(crc << 1);
    }
  }
  return crc;
}
