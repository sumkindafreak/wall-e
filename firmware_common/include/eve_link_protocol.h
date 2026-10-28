/**
 * EVE <-> WALL-E UART link — canonical wire protocol (header-only, no Arduino dependency).
 *
 * Shared by:
 *   - eve/arduino_ide/EVE_Companion   (EVE, via eve_protocol.h)
 *   - main_wall_E_base/main           (WALL-E base brain, via eve_uart_bridge.cpp)
 * eve_dock_esp32s3 keeps its own copy of the IDs it uses; every ID it relies on is unchanged here.
 *
 * Frame layout (little-endian length and CRC):
 *   SOF0 SOF1 | VER | TYPE | LEN_LO LEN_HI | SEQ | PAYLOAD[LEN] | CRC_LO CRC_HI
 *   CRC = CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over VER .. last payload byte.
 *   PAYLOAD = UTF-8 JSON text (may be empty), at most EVE_MAX_PAYLOAD bytes.
 *
 * Compatibility rule: only ADD message IDs and JSON fields. Never renumber or change the meaning
 * of an existing ID — older EVE/WALL-E/dock firmware simply ignores IDs it does not know.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Framing constants
// -----------------------------------------------------------------------------
static const uint8_t EVE_SOF0 = 0xA5;
static const uint8_t EVE_SOF1 = 0x5A;
static const uint8_t EVE_FRAME_VER = 0x01;
static const uint16_t EVE_MAX_PAYLOAD = 512;
/** Bytes around the payload: 2 SOF + VER + TYPE + 2 LEN + SEQ + 2 CRC. */
static const uint16_t EVE_FRAME_OVERHEAD = 9;

// -----------------------------------------------------------------------------
// Link timing (both ends use the same numbers)
// -----------------------------------------------------------------------------
/** A linked peer sends a heartbeat at least this often. */
static const uint32_t EVE_LINK_HEARTBEAT_PERIOD_MS = 250;
/** No valid frame from the linked session for this long = link lost (8 missed heartbeats). */
static const uint32_t EVE_LINK_TIMEOUT_MS = 2000;

// -----------------------------------------------------------------------------
// Message types
// -----------------------------------------------------------------------------
enum EveMsgType : uint8_t {
  // EVE -> WALL-E -----------------------------------------------------------
  MSG_EVE_HELLO = 0x01,            /**< {"proto":1,"eve":"eve-s3","caps":N,"uptime_ms":N} */
  MSG_WALL_E_ACK = 0x02,           /**< (WALL-E/dock -> EVE) {"session":N,"peer":"wall_e","hb":250} */
  MSG_EVE_READY = 0x03,            /**< {"session":N} */
  MSG_EVE_HEARTBEAT = 0x04,        /**< {"session":N, ...status fields, see eve_control_core} */
  MSG_EVE_LOW_BATTERY = 0x05,      /**< {"v":V,"a":A,"pct":P,"session":N} */
  MSG_EVE_SLEEP = 0x06,            /**< {"session":N} EVE entered standby */
  MSG_EVE_ERROR = 0x07,            /**< {"code":N,"msg":"..."} */
  MSG_EVE_TARGET_AWARENESS = 0x08, /**< eve_target_protocol.h payload */
  MSG_EVE_COMPANION = 0x09,        /**< {"m":"token"} shared behaviour */
  MSG_EVE_CMD_RESULT = 0x0A,       /**< {"seq":N,"type":N,"ok":bool,"err":"reason"} reply to a command */

  // WALL-E -> EVE ----------------------------------------------------------
  MSG_ATTACH_CONFIRMED = 0x10,  /**< Operator/WALL-E asserts EVE is attached. Not proof of contact. */
  MSG_WALL_E_HEARTBEAT = 0x11,  /**< {"session":N} keeps EVE's link supervision fed */
  MSG_MODE_ESCORT = 0x20,
  MSG_MODE_DOCK = 0x21,
  MSG_MODE_IDLE = 0x22,
  MSG_MODE_INTERACT = 0x23,
  MSG_MODE_STANDBY = 0x24,

  MSG_SET_EYES = 0x30,     /**< {"e":"happy","ms":2000}  or legacy {"mode":0..2} */
  MSG_SET_GLOW = 0x31,     /**< {"p":"idle"} or {"pattern":N} */
  MSG_MOVE_SERVO = 0x32,   /**< {"l":deg,"r":deg} head pan / right arm */
  MSG_PLAY_SOUND = 0x33,   /**< {"track":N} */
  MSG_STOP = 0x34,         /**< {"src":"..."} latches EVE's stop. Accepted in every state. */
  MSG_RESET_STATE = 0x35,  /**< Restart the link handshake. Never clears a latched stop. */
  MSG_CLEAR_STOP = 0x36,   /**< {"session":N} explicit clear of a latched stop */
  MSG_RUN_ROUTINE = 0x37,  /**< {"r":"scan"|"play"|"protect"|"greet"} operator-triggered expression */

  // Shared UART intelligence bus extension range (firmware_common/include/uart_protocol.h)
  MSG_BUS_HELLO = 0x40,
  MSG_BUS_IDENTIFY = 0x41,
  MSG_BUS_STATUS = 0x42,
  MSG_BUS_MEMORY_SYNC = 0x43,
  MSG_BUS_COMMAND = 0x44,
  MSG_BUS_EVENT = 0x45,
  MSG_BUS_ACK = 0x46,
  MSG_BUS_NACK = 0x47,
};

/** Human-readable message name for logs and the WALL-E status page. */
static inline const char* eveLinkMsgName(uint8_t t) {
  switch (t) {
    case MSG_EVE_HELLO: return "EVE_HELLO";
    case MSG_WALL_E_ACK: return "WALL_E_ACK";
    case MSG_EVE_READY: return "EVE_READY";
    case MSG_EVE_HEARTBEAT: return "EVE_HEARTBEAT";
    case MSG_EVE_LOW_BATTERY: return "EVE_LOW_BATTERY";
    case MSG_EVE_SLEEP: return "EVE_SLEEP";
    case MSG_EVE_ERROR: return "EVE_ERROR";
    case MSG_EVE_TARGET_AWARENESS: return "EVE_TARGET_AWARENESS";
    case MSG_EVE_COMPANION: return "EVE_COMPANION";
    case MSG_EVE_CMD_RESULT: return "EVE_CMD_RESULT";
    case MSG_ATTACH_CONFIRMED: return "ATTACH_CONFIRMED";
    case MSG_WALL_E_HEARTBEAT: return "WALL_E_HEARTBEAT";
    case MSG_MODE_ESCORT: return "MODE_ESCORT";
    case MSG_MODE_DOCK: return "MODE_DOCK";
    case MSG_MODE_IDLE: return "MODE_IDLE";
    case MSG_MODE_INTERACT: return "MODE_INTERACT";
    case MSG_MODE_STANDBY: return "MODE_STANDBY";
    case MSG_SET_EYES: return "SET_EYES";
    case MSG_SET_GLOW: return "SET_GLOW";
    case MSG_MOVE_SERVO: return "MOVE_SERVO";
    case MSG_PLAY_SOUND: return "PLAY_SOUND";
    case MSG_STOP: return "STOP";
    case MSG_RESET_STATE: return "RESET_STATE";
    case MSG_CLEAR_STOP: return "CLEAR_STOP";
    case MSG_RUN_ROUTINE: return "RUN_ROUTINE";
    case MSG_BUS_HELLO: return "BUS_HELLO";
    case MSG_BUS_IDENTIFY: return "BUS_IDENTIFY";
    case MSG_BUS_STATUS: return "BUS_STATUS";
    case MSG_BUS_MEMORY_SYNC: return "BUS_MEMORY_SYNC";
    case MSG_BUS_COMMAND: return "BUS_COMMAND";
    case MSG_BUS_EVENT: return "BUS_EVENT";
    case MSG_BUS_ACK: return "BUS_ACK";
    case MSG_BUS_NACK: return "BUS_NACK";
    default: return "unknown";
  }
}

// -----------------------------------------------------------------------------
// CRC + encoder
// -----------------------------------------------------------------------------
static inline uint16_t eveLinkCrc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF) {
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)((uint16_t)data[i] << 8);
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

/**
 * Build one complete frame into out[].
 * Returns the number of bytes written, or 0 when the payload is too large or out[] too small.
 */
static inline size_t eveLinkEncode(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len, uint8_t* out,
                                   size_t outCap) {
  if (len > EVE_MAX_PAYLOAD) return 0;
  if (len > 0 && payload == nullptr) return 0;
  const size_t total = (size_t)EVE_FRAME_OVERHEAD + len;
  if (out == nullptr || outCap < total) return 0;
  out[0] = EVE_SOF0;
  out[1] = EVE_SOF1;
  out[2] = EVE_FRAME_VER;
  out[3] = type;
  out[4] = (uint8_t)(len & 0xFF);
  out[5] = (uint8_t)((len >> 8) & 0xFF);
  out[6] = seq;
  if (len > 0) memcpy(out + 7, payload, len);
  const uint16_t crc = eveLinkCrc16(out + 2, (size_t)5 + len);
  out[7 + len] = (uint8_t)(crc & 0xFF);
  out[8 + len] = (uint8_t)((crc >> 8) & 0xFF);
  return total;
}

// -----------------------------------------------------------------------------
// Streaming decoder
// -----------------------------------------------------------------------------
/**
 * Feed received bytes one at a time. feed() returns true exactly when a complete frame with a
 * correct CRC AND the supported version has arrived; read it with type()/seq()/payload()/len()
 * before the next feed(). Anything else (noise, wrong version, bad length, bad CRC) is counted
 * and discarded, so it can never be mistaken for a live peer.
 */
class EveFrameDecoder {
 public:
  struct Stats {
    uint32_t ok = 0;
    uint32_t crcErr = 0;
    uint32_t versionErr = 0;
    uint32_t lengthErr = 0;
  };

  void reset() { st_ = WAIT_SOF0; }

  bool feed(uint8_t c) {
    switch (st_) {
      case WAIT_SOF0:
        if (c == EVE_SOF0) st_ = WAIT_SOF1;
        return false;
      case WAIT_SOF1:
        if (c == EVE_SOF1) {
          st_ = READ_VER;
          hdrIdx_ = 0;
        } else if (c != EVE_SOF0) {
          st_ = WAIT_SOF0;
        }
        return false;
      case READ_VER:
      case READ_TYPE:
      case READ_LEN_LO:
      case READ_LEN_HI:
      case READ_SEQ:
        hdr_[hdrIdx_++] = c;
        if (st_ == READ_LEN_HI) {
          len_ = (uint16_t)hdr_[2] | (uint16_t)((uint16_t)hdr_[3] << 8);
          if (len_ > EVE_MAX_PAYLOAD) {
            stats_.lengthErr++;
            st_ = WAIT_SOF0;
            return false;
          }
        }
        if (st_ == READ_SEQ) {
          payIdx_ = 0;
          st_ = (len_ == 0) ? READ_CRC_LO : READ_PAYLOAD;
        } else {
          st_ = (State)(st_ + 1);
        }
        return false;
      case READ_PAYLOAD:
        payload_[payIdx_++] = c;
        if (payIdx_ >= len_) st_ = READ_CRC_LO;
        return false;
      case READ_CRC_LO:
        crcLo_ = c;
        st_ = READ_CRC_HI;
        return false;
      case READ_CRC_HI: {
        st_ = WAIT_SOF0;
        uint16_t crc = eveLinkCrc16(hdr_, 5);
        crc = eveLinkCrc16(payload_, len_, crc);
        const uint16_t wire = (uint16_t)crcLo_ | (uint16_t)((uint16_t)c << 8);
        if (crc != wire) {
          stats_.crcErr++;
          return false;
        }
        if (hdr_[0] != EVE_FRAME_VER) {
          stats_.versionErr++;
          return false;
        }
        stats_.ok++;
        payload_[len_] = 0; /* convenience NUL so payload() can be read as text */
        return true;
      }
    }
    st_ = WAIT_SOF0;
    return false;
  }

  uint8_t type() const { return hdr_[1]; }
  uint8_t seq() const { return hdr_[4]; }
  uint16_t len() const { return len_; }
  const uint8_t* payload() const { return payload_; }
  /** Payload as NUL-terminated text (valid right after feed() returned true). */
  const char* text() const { return (const char*)payload_; }
  const Stats& stats() const { return stats_; }

 private:
  enum State : uint8_t {
    WAIT_SOF0,
    WAIT_SOF1,
    READ_VER,
    READ_TYPE,
    READ_LEN_LO,
    READ_LEN_HI,
    READ_SEQ,
    READ_PAYLOAD,
    READ_CRC_LO,
    READ_CRC_HI
  };
  State st_ = WAIT_SOF0;
  uint8_t hdr_[5] = {0, 0, 0, 0, 0}; /* VER TYPE LEN_LO LEN_HI SEQ */
  uint8_t hdrIdx_ = 0;
  uint16_t len_ = 0;
  uint16_t payIdx_ = 0;
  uint8_t crcLo_ = 0;
  uint8_t payload_[EVE_MAX_PAYLOAD + 1] = {0};
  Stats stats_;
};
