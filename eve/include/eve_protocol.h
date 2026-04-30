/**
 * Binary UART framing + message type IDs (EVE <-> WALL-E).
 * Frame: SOF(2) VER(1) TYPE(1) LEN(2 LE) SEQ(1) PAYLOAD[L] CRC16(2 LE CCITT)
 */
#pragma once

#include <stdint.h>

static const uint8_t EVE_SOF0 = 0xA5;
static const uint8_t EVE_SOF1 = 0x5A;
static const uint8_t EVE_FRAME_VER = 0x01;
static const uint16_t EVE_MAX_PAYLOAD = 512;

enum EveMsgType : uint8_t {
  // EVE -> WALL-E
  MSG_EVE_HELLO = 0x01,
  MSG_WALL_E_ACK = 0x02,
  MSG_EVE_READY = 0x03,
  MSG_EVE_HEARTBEAT = 0x04,
  MSG_EVE_LOW_BATTERY = 0x05,
  MSG_EVE_SLEEP = 0x06,
  MSG_EVE_ERROR = 0x07,
  MSG_EVE_TARGET_AWARENESS = 0x08,
  /** JSON `{"m":"token"}` — shared behaviour / bi-directional companion (base: walle_shared_behaviour) */
  MSG_EVE_COMPANION = 0x09,

  // WALL-E -> EVE
  MSG_ATTACH_CONFIRMED = 0x10,
  MSG_MODE_ESCORT = 0x20,
  MSG_MODE_DOCK = 0x21,
  MSG_MODE_IDLE = 0x22,

  MSG_SET_EYES = 0x30,
  MSG_SET_GLOW = 0x31,
  MSG_MOVE_SERVO = 0x32,
  MSG_PLAY_SOUND = 0x33,
  MSG_STOP = 0x34,
  MSG_RESET_STATE = 0x35,

  // Shared UART intelligence bus extension range.
  MSG_BUS_HELLO = 0x40,
  MSG_BUS_IDENTIFY = 0x41,
  MSG_BUS_STATUS = 0x42,
  MSG_BUS_MEMORY_SYNC = 0x43,
  MSG_BUS_COMMAND = 0x44,
  MSG_BUS_EVENT = 0x45,
  MSG_BUS_ACK = 0x46,
  MSG_BUS_NACK = 0x47,
};
