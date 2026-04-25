#pragma once
#include <stdint.h>
static const uint8_t EVE_SOF0 = 0xA5;
static const uint8_t EVE_SOF1 = 0x5A;
static const uint8_t EVE_FRAME_VER = 0x01;
static const uint16_t EVE_MAX_PAYLOAD = 512;
enum EveMsgType : uint8_t { MSG_EVE_HELLO = 0x01, MSG_WALL_E_ACK = 0x02, MSG_EVE_READY = 0x03, MSG_EVE_HEARTBEAT = 0x04, MSG_EVE_LOW_BATTERY = 0x05, MSG_EVE_ERROR = 0x07, MSG_EVE_COMPANION = 0x09, MSG_MODE_DOCK = 0x21 };
