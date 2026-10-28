/**
 * Binary UART framing + message type IDs (EVE <-> WALL-E).
 *
 * The protocol is defined ONCE in firmware_common/include/eve_link_protocol.h and shared with the
 * WALL-E base, so the two ends cannot drift apart. This file only forwards to it.
 * Frame: SOF(2) VER(1) TYPE(1) LEN(2 LE) SEQ(1) PAYLOAD[L] CRC16(2 LE CCITT)
 */
#pragma once

#include "../../../firmware_common/include/eve_link_protocol.h"
