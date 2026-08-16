/**
 * Node health registry — aggregates radio heartbeats for WebUI.
 */
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "battery_monitor.h"
#include "radio_transport.h"
#include <Arduino.h>
#include <cstring>
#include <cmath>

#ifndef NODE_HEALTH_TIMEOUT_MS
#define NODE_HEALTH_TIMEOUT_MS 3500u
#endif
#ifndef NODE_HEALTH_BROADCAST_MS
#define NODE_HEALTH_BROADCAST_MS 1000u
#endif

/* Match dock_station DockState order */
enum {
  DOCK_ST_BOOT = 0,
  DOCK_ST_NOT_DOCKED = 1,
  DOCK_ST_DOCKED_IDLE = 2,
  DOCK_ST_CHARGING = 3,
  DOCK_ST_CHARGED = 4,
  DOCK_ST_FAULT = 5
};

struct NodeSlot {
  uint32_t last_seen_ms;
  WalleNodeHealthPacket_t pkt;
  bool ever_seen;
};

static NodeSlot s_slot[WALLE_NODE_COUNT];
static uint32_t s_last_broadcast_ms = 0;
static uint32_t s_last_base_fill_ms = 0;

static int8_t readBoardTempC(void) {
#if defined(ARDUINO_ARCH_ESP32)
  float t = temperatureRead();
  if (!std::isfinite(t) || t < -55.0f || t > 125.0f) {
    return WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  }
  return (int8_t)constrain((int)t, -127, 127);
#else
  return WALLE_NODE_HEALTH_UNKNOWN_TEMP;
#endif
}

void nodeHealthInit(void) {
  memset(s_slot, 0, sizeof(s_slot));
  s_last_broadcast_ms = 0;
  s_last_base_fill_ms = 0;
  (void)radioTransportInit();
}

static void fillBaseSlot(uint32_t now) {
  NodeSlot& sl = s_slot[WALLE_NODE_BASE];
  const BatteryData& b = batteryGetData();
  sl.pkt.magic = WALLE_NODE_HEALTH_MAGIC;
  sl.pkt.version = WALLE_NODE_HEALTH_VERSION;
  sl.pkt.node_id = WALLE_NODE_BASE;
  sl.pkt.role = WALLE_ROLE_ACTOR;
  sl.pkt.battery_pct = (b.valid && b.percent >= 0)
                           ? (uint8_t)constrain(b.percent, 0, 100)
                           : WALLE_NODE_HEALTH_UNKNOWN_BAT;
  sl.pkt.temp_c = readBoardTempC();
  sl.pkt.uptime_ms = now;
  sl.pkt.last_error = 0;
  sl.pkt.flags = 0;
  sl.last_seen_ms = now;
  sl.ever_seen = true;
}

void nodeHealthOnPacket(const uint8_t* data, int len) {
  if (!data || len < (int)sizeof(WalleNodeHealthPacket_t)) return;
  const WalleNodeHealthPacket_t* p = (const WalleNodeHealthPacket_t*)data;
  if (p->magic != WALLE_NODE_HEALTH_MAGIC ||
      p->version != WALLE_NODE_HEALTH_VERSION) return;
  if (p->node_id >= WALLE_NODE_COUNT) return;
  if (p->node_id == WALLE_NODE_BASE) return;

  NodeSlot& sl = s_slot[p->node_id];
  memcpy(&sl.pkt, p, sizeof(WalleNodeHealthPacket_t));
  sl.last_seen_ms = millis();
  sl.ever_seen = true;
}

void nodeHealthOnDockBeacon(const DockBeaconPacket_t* bp) {
  if (!bp || bp->magic != DOCK_BEACON_MAGIC) return;

  NodeSlot& sl = s_slot[WALLE_NODE_DOCK];
  sl.pkt.magic = WALLE_NODE_HEALTH_MAGIC;
  sl.pkt.version = WALLE_NODE_HEALTH_VERSION;
  sl.pkt.node_id = WALLE_NODE_DOCK;
  sl.pkt.role = WALLE_ROLE_SENSOR;
  sl.pkt.battery_pct = WALLE_NODE_HEALTH_UNKNOWN_BAT;
  sl.pkt.temp_c = WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  sl.pkt.uptime_ms = bp->uptime_ms;
  sl.pkt.last_error = (bp->state == DOCK_ST_FAULT) ? 1u : 0u;

  uint16_t f = 0;
  if (bp->beam_present) f |= WALLE_NODE_FLAG_DOCKED;
  if (bp->state == DOCK_ST_CHARGING || bp->state == DOCK_ST_CHARGED) {
    f |= WALLE_NODE_FLAG_DOCKED;
  }
  if (bp->charge_enabled && bp->state == DOCK_ST_CHARGING) {
    f |= WALLE_NODE_FLAG_CHARGING;
  }
  if (bp->state == DOCK_ST_FAULT) f |= WALLE_NODE_FLAG_FAULT;
  sl.pkt.flags = f;

  sl.last_seen_ms = millis();
  sl.ever_seen = true;
}

void nodeHealthMarkVisionSeen(void) {
  const uint32_t now = millis();
  NodeSlot& sl = s_slot[WALLE_NODE_VISION];
  sl.pkt.magic = WALLE_NODE_HEALTH_MAGIC;
  sl.pkt.version = WALLE_NODE_HEALTH_VERSION;
  sl.pkt.node_id = WALLE_NODE_VISION;
  sl.pkt.role = WALLE_ROLE_SENSOR;
  sl.pkt.battery_pct = WALLE_NODE_HEALTH_UNKNOWN_BAT;
  sl.pkt.temp_c = WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  sl.pkt.uptime_ms = now;
  sl.pkt.last_error = 0;
  sl.pkt.flags = 0;
  sl.last_seen_ms = now;
  sl.ever_seen = true;
}

void nodeHealthTick(void) {
  const uint32_t now = millis();

  if (now - s_last_base_fill_ms >= 200u) {
    s_last_base_fill_ms = now;
    fillBaseSlot(now);
  }

  if (now - s_last_broadcast_ms >= NODE_HEALTH_BROADCAST_MS) {
    s_last_broadcast_ms = now;
    WalleNodeHealthPacket_t out = s_slot[WALLE_NODE_BASE].pkt;
    (void)radioTransportBroadcast(&out, sizeof(out));
  }
}

static bool slotOnline(uint8_t id, uint32_t now) {
  if (id >= WALLE_NODE_COUNT) return false;
  if (id == WALLE_NODE_BASE) return true;
  const NodeSlot& sl = s_slot[id];
  if (!sl.ever_seen) return false;
  return (now - sl.last_seen_ms) < NODE_HEALTH_TIMEOUT_MS;
}

bool nodeHealthIsMasterStale(void) {
  return !slotOnline(WALLE_NODE_MASTER, millis());
}

bool nodeHealthIsOnline(uint8_t node_id) {
  return slotOnline(node_id, millis());
}

uint16_t nodeHealthGetFlags(uint8_t node_id) {
  if (node_id >= WALLE_NODE_COUNT) return 0;
  return s_slot[node_id].pkt.flags;
}

String nodeHealthGetJSON(void) {
  const uint32_t now = millis();
  String j = "{\"v\":1,\"timeout_ms\":";
  j += (uint32_t)NODE_HEALTH_TIMEOUT_MS;
  j += ",\"global_hb_ms\":";
  j += now;
  j += ",\"nodes\":[";

  static const char* names[] = {"base", "master", "audio", "dock", "vision"};
  for (int i = 0; i < (int)WALLE_NODE_COUNT; i++) {
    if (i) j += ',';
    const NodeSlot& sl = s_slot[i];
    const bool online = slotOnline((uint8_t)i, now);
    j += '{';
    j += "\"id\":\""; j += names[i]; j += '"';
    j += ",\"nid\":"; j += (int)sl.pkt.node_id;
    j += ",\"role\":"; j += (int)sl.pkt.role;
    j += ",\"battery_pct\":";
    j += (sl.pkt.battery_pct == WALLE_NODE_HEALTH_UNKNOWN_BAT
              ? -1
              : (int)sl.pkt.battery_pct);
    j += ",\"temp_c\":";
    if (sl.pkt.temp_c == WALLE_NODE_HEALTH_UNKNOWN_TEMP) j += "null";
    else j += (int)sl.pkt.temp_c;
    j += ",\"uptime_ms\":"; j += (uint32_t)sl.pkt.uptime_ms;
    j += ",\"error\":"; j += (uint32_t)sl.pkt.last_error;
    j += ",\"flags\":"; j += (uint32_t)sl.pkt.flags;
    j += ",\"online\":"; j += (online ? "true" : "false");
    if (sl.ever_seen) {
      const uint32_t age = now - sl.last_seen_ms;
      j += ",\"age_ms\":"; j += age;
    } else {
      j += ",\"age_ms\":null";
    }
    j += '}';
  }
  j += "]}";
  return j;
}
