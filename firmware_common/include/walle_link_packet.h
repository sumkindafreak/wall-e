// ============================================================
//  CYD ↔ Base ESP-NOW: control + telemetry
//  v2: sequenced control + CRC-8, telemetry echoes seq + ack bits
//  Canonical copy: firmware_common/include/
// ============================================================
#ifndef WALLE_LINK_PACKET_H
#define WALLE_LINK_PACKET_H

#include <stdint.h>
#include <stddef.h>

/* CRC-8 (poly 0x07) over bytes [0..21]; byte 22 = crc. */
static inline uint8_t walle_crc8_dallas(const uint8_t* d, size_t n) {
  uint8_t c = 0;
  for (size_t i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07u) : (uint8_t)(c << 1);
  }
  return c;
}

#define WALLE_CTRL_VERSION_OFF   0u
#define WALLE_CTRL_VERSION_V2   2u
#define WALLE_CTRL_OBSOLETE_BYTES  19u
#define WALLE_CTRL_PACKET_V2       23u
#define WALLE_CTRL_CRC_LEN         22u

/** Max allowed forward seq jump (exclusive upper bound vs last accepted) for CYD v2.
 *  Gaps larger than this are treated as loss/replay risk: packet dropped, link flag set. */
#define WALLE_CTRL_MAX_SEQ_JUMP    64u

#define WALLE_ACK_ESTOP            0x01u
#define WALLE_ACK_MOTION_POLICY   0x02u
#define WALLE_ACK_CHARGE_REQUEST  0x04u
#define WALLE_ACK_APPROACH_STAGE  0x08u

/** TelemetryPacket.rsv0 flags (Base → CYD). */
#define WALLE_TELEM_RSV0_SEQ_GAP     0x01u /* CYD seq jump too large or anomaly */
#define WALLE_TELEM_RSV0_TELEM_TX_FAIL 0x02u /* Base → CYD esp_now_send failed (recent) */
#define WALLE_TELEM_RSV0_EVE_UART   0x04u /* EVE companion UART link OK (base) */

typedef struct __attribute__((packed)) {
  int8_t   leftSpeed;
  int8_t   rightSpeed;
  uint8_t  driveMode;
  uint8_t  behaviourMode;
  uint8_t  action;
  uint16_t systemFlags;
  uint8_t  servoTargets[10];
  uint8_t  aux0;
  uint8_t  aux1;
  uint8_t  ctrl_version;
  uint16_t seq;
  uint8_t  crc8;
} ControlPacket;

static inline void walleControlPacketSeal(ControlPacket* p, uint16_t* seq) {
  if (!p) return;
  p->ctrl_version = WALLE_CTRL_VERSION_V2;
  if (seq) {
    (*seq)++;
    if (*seq == 0) (*seq)++;
    p->seq = *seq;
  } else {
    p->seq = 0;
  }
  p->crc8 = walle_crc8_dallas((const uint8_t*)p, WALLE_CTRL_CRC_LEN);
}

#define WALLE_TELEMETRY_MAGIC   0x54454C4Du
#define WALLE_TELEMETRY_VERSION 2u

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t  version;
  float   batteryVoltage;
  float   currentDraw;
  float   temperature;
  uint8_t moodState;
  uint8_t autonomousState;
  uint8_t safetyState;
  uint8_t autonomyEnabled;
  uint8_t autonomyState;
  float   sonarDistanceCm;
  float   compassHeading;
  float   gpsLatitude;
  float   gpsLongitude;
  uint8_t gpsValid;
  uint8_t waypointMode;
  float   waypointDistanceM;
  float   waypointBearingDeg;
  uint8_t currentWaypoint;
  uint8_t totalWaypoints;
  uint8_t motionPolicy;
  uint8_t policyDenyCyd;
  uint16_t last_control_seq_applied;
  uint8_t  control_ack_bits;
  uint8_t  rsv0;
} TelemetryPacket;

#define WALLE_TELEMETRY_SIZE_V1 52u
/** Full v2 wire size (packed, no padding) */
#define WALLE_TELEMETRY_SIZE_V2 56u

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(ControlPacket) == WALLE_CTRL_PACKET_V2, "ControlPacket v2 wire size");
static_assert(sizeof(TelemetryPacket) == WALLE_TELEMETRY_SIZE_V2, "TelemetryPacket v2 wire size");
#endif

#endif
