/**
 * Node health registry — aggregates ESP-NOW heartbeats for WebUI.
 */
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "battery_monitor.h"
#include "loop_stats.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

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

static uint8_t s_bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_peer_ok = false;

static bool ensureBroadcastPeer(void) {
  if (s_peer_ok) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_bcast, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_AP;
  esp_err_t r = esp_now_add_peer(&peer);
  if (r == ESP_OK || r == ESP_ERR_ESPNOW_EXIST) {
    s_peer_ok = true;
    return true;
  }
  return false;
}

static int8_t readBoardTempC(void) {
#if defined(ARDUINO_ARCH_ESP32)
  float t = temperatureRead();
  if (isnan(t) || t < -55.0f || t > 125.0f) return WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  return (int8_t)constrain((int)t, -127, 127);
#else
  return WALLE_NODE_HEALTH_UNKNOWN_TEMP;
#endif
}

void nodeHealthInit(void) {
  memset(s_slot, 0, sizeof(s_slot));
  s_last_broadcast_ms = 0;
  s_last_base_fill_ms = 0;
}

extern unsigned long lastCommandMillis;

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
  sl.pkt.free_heap = ESP.getFreeHeap();
  sl.pkt.loop_p95_ms = loopStatsGetP95Ms();
  sl.pkt.wifi_rssi = (int8_t)WALLE_NODE_RSSI_UNKNOWN; /* softAP: no single-client RSSI here */
  sl.pkt.sd_ok = 0; /* add SD if wired on base */
  if (lastCommandMillis > 0u && now >= lastCommandMillis) {
    sl.pkt.last_peer_rx_age_ms = (uint32_t)(now - lastCommandMillis);
  } else {
    sl.pkt.last_peer_rx_age_ms = 0xFFFFFFFFu;
  }
  sl.last_seen_ms = now;
  sl.ever_seen = true;
}

void nodeHealthOnPacket(const uint8_t* data, int len) {
  if (!data || len < 8) return;
  const WalleNodeHealthPacket_t* p = (const WalleNodeHealthPacket_t*)data;
  if (p->magic != WALLE_NODE_HEALTH_MAGIC) return;
  if (p->version < 1 || p->version > 2) return;
  if (p->version == 1) {
    if (len < (int)WALLE_NODE_HEALTH_V1_SIZE) return;
  } else {
    if (len < (int)WALLE_NODE_HEALTH_V2_SIZE) return;
  }
  if (p->node_id >= WALLE_NODE_COUNT) return;
  /* Local BASE slot is filled from battery/ESP — ignore echoed or stray BASE packets. */
  if (p->node_id == WALLE_NODE_BASE) return;

  NodeSlot& sl = s_slot[p->node_id];
  memset(&sl.pkt, 0, sizeof(sl.pkt));
  size_t cpy = (p->version == 1) ? (size_t)WALLE_NODE_HEALTH_V1_SIZE : (size_t)WALLE_NODE_HEALTH_V2_SIZE;
  memcpy(&sl.pkt, data, cpy);
  if (p->version == 1) {
    sl.pkt.version = 1; /* v1 node; extended fields were zeroed */
  }
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
  sl.pkt.free_heap = 0;
  sl.pkt.loop_p95_ms = 0;
  sl.pkt.wifi_rssi = (int8_t)WALLE_NODE_RSSI_UNKNOWN;
  sl.pkt.sd_ok = 0;
  sl.pkt.last_peer_rx_age_ms = 0xFFFFFFFFu;

  uint16_t f = 0;
  if (bp->beam_present) f |= WALLE_NODE_FLAG_DOCKED;
  if (bp->state == DOCK_ST_CHARGING || bp->state == DOCK_ST_CHARGED) f |= WALLE_NODE_FLAG_DOCKED;
  if (bp->charge_enabled && (bp->state == DOCK_ST_CHARGING)) f |= WALLE_NODE_FLAG_CHARGING;
  if (bp->state == DOCK_ST_FAULT) f |= WALLE_NODE_FLAG_FAULT;
  sl.pkt.flags = f;

  sl.last_seen_ms = millis();
  sl.ever_seen = true;
}

void nodeHealthMarkVisionSeen(void) {
  uint32_t now = millis();
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
  sl.pkt.free_heap = ESP.getFreeHeap();
  sl.pkt.loop_p95_ms = 0;
  sl.pkt.wifi_rssi = (int8_t)WALLE_NODE_RSSI_UNKNOWN;
  sl.pkt.sd_ok = 0;
  sl.pkt.last_peer_rx_age_ms = 0xFFFFFFFFu;
  sl.last_seen_ms = now;
  sl.ever_seen = true;
}

void nodeHealthTick(void) {
  uint32_t now = millis();

  if (now - s_last_base_fill_ms >= 200u) {
    s_last_base_fill_ms = now;
    fillBaseSlot(now);
  }

  if (now - s_last_broadcast_ms >= NODE_HEALTH_BROADCAST_MS) {
    s_last_broadcast_ms = now;
    if (ensureBroadcastPeer()) {
      WalleNodeHealthPacket_t out = s_slot[WALLE_NODE_BASE].pkt;
      esp_now_send(s_bcast, (uint8_t*)&out, sizeof(out));
    }
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
  uint32_t now = millis();
  String j = "{\"v\":2,\"timeout_ms\":";
  j += (uint32_t)NODE_HEALTH_TIMEOUT_MS;
  j += ",\"global_hb_ms\":";
  j += now;
  j += ",\"nodes\":[";

  static const char* names[] = {"base", "master", "audio", "dock", "vision"};
  for (int i = 0; i < (int)WALLE_NODE_COUNT; i++) {
    if (i) j += ',';
    const NodeSlot& sl = s_slot[i];
    bool online = slotOnline((uint8_t)i, now);
    j += '{';
    j += "\"id\":\""; j += names[i]; j += '"';
    j += ",\"nid\":"; j += (int)sl.pkt.node_id;
    j += ",\"role\":"; j += (int)sl.pkt.role;
    j += ",\"battery_pct\":"; j += (sl.pkt.battery_pct == WALLE_NODE_HEALTH_UNKNOWN_BAT ? -1 : (int)sl.pkt.battery_pct);
    j += ",\"temp_c\":";
    if (sl.pkt.temp_c == WALLE_NODE_HEALTH_UNKNOWN_TEMP) j += "null";
    else j += (int)sl.pkt.temp_c;
    j += ",\"uptime_ms\":"; j += (uint32_t)sl.pkt.uptime_ms;
    j += ",\"error\":"; j += (uint32_t)sl.pkt.last_error;
    j += ",\"flags\":"; j += (uint32_t)sl.pkt.flags;
    j += ",\"online\":"; j += (online ? "true" : "false");
    if (sl.ever_seen) {
      uint32_t age = (now - sl.last_seen_ms);
      j += ",\"age_ms\":"; j += age;
    } else {
      j += ",\"age_ms\":null";
    }
    j += ",\"hver\":"; j += (int)sl.pkt.version;
    if (sl.pkt.version >= 2) {
      j += ",\"free_heap\":"; j += (uint32_t)sl.pkt.free_heap;
      j += ",\"loop_p95_ms\":"; j += (uint32_t)sl.pkt.loop_p95_ms;
      j += ",\"rssi\":";
      if (sl.pkt.wifi_rssi == WALLE_NODE_RSSI_UNKNOWN) j += "null";
      else j += String((int)sl.pkt.wifi_rssi);
      j += ",\"sd_ok\":"; j += (unsigned)sl.pkt.sd_ok;
      if (sl.pkt.last_peer_rx_age_ms == 0xFFFFFFFFu) {
        j += ",\"last_peer_age_ms\":null";
      } else {
        j += ",\"last_peer_age_ms\":"; j += (uint32_t)sl.pkt.last_peer_rx_age_ms;
      }
    }
    j += '}';
  }
  j += "]}";
  return j;
}
