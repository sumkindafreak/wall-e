#include "dock_control.h"
#include "charging_control.h"
#include "dock_eve_link.h"
#include "led_status.h"
#include "../firmware_common/include/dock_manager.h"
#include <esp_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DockFsmState s_st = ST_WAIT_FOR_DOCK;
static uint32_t s_wait_t0_ms = 0;
static uint32_t s_last_eve_rx_ms = 0;
static uint32_t s_last_eve_heartbeat_ms = 0;
static uint32_t s_session_id = 0;
static uint32_t s_startup_prime_until_ms = 0;
static bool s_startup_prime_active = false;
static DockManager s_dockManager;

static void note_eve_voltage_from_json_(const uint8_t* payload, size_t len) {
  if (!payload || len == 0 || len >= 220) return;
  char buf[220];
  memcpy(buf, payload, len);
  buf[len] = '\0';
  const char* key = strstr(buf, "\"bat_v\"");
  if (!key) return;
  const char* colon = strchr(key, ':');
  if (!colon) return;
  float pack_v = (float)atof(colon + 1);
  charging_note_eve_voltage(pack_v);
}

static uint32_t make_session_id_(void) {
  uint32_t sid = ((uint32_t)(uint16_t)esp_random() << 16) ^ (uint32_t)millis();
  return sid == 0 ? 1u : sid;
}

static bool dock_set_charging_(bool enable) {
  if (enable && charging_is_safety_locked_out()) {
    return false;
  }
  if (enable && !charging_voltage_ok_to_enable()) {
    return false;
  }
  charging_set_enable(enable);
  return true;
}

static void dock_set_light_(DockLightMode mode) {
  switch (mode) {
    case DOCK_LIGHT_IDLE: led_set_mode(LED_MODE_BLUE_PULSE); break;
    case DOCK_LIGHT_DOCKED: led_set_mode(LED_MODE_BLUE_PULSE); break;
    case DOCK_LIGHT_SYNCING: led_set_mode(LED_MODE_PURPLE_FLASH); break;
    case DOCK_LIGHT_CHARGING: led_set_mode(LED_MODE_GREEN_SOLID); break;
    case DOCK_LIGHT_INTERACTIVE: led_set_mode(LED_MODE_STANDBY); break;
    case DOCK_LIGHT_FAULT: led_set_mode(LED_MODE_RED_BLINK); break;
    case DOCK_LIGHT_OFF:
    default: led_set_mode(LED_MODE_OFF); break;
  }
}

static bool dock_send_bus_json_(uint8_t type, const char* json) {
  return dockEveLinkSendJson((EveMsgType)type, json);
}

static void send_dock_ack_(void) {
  if (s_session_id == 0) s_session_id = make_session_id_();
  char json[96];
  snprintf(json, sizeof(json), "{\"session\":%lu,\"peer\":\"eve_dock_c3\"}", (unsigned long)s_session_id);
  dockEveLinkSendJson(MSG_WALL_E_ACK, json);
  Serial.println(F("[DOCK] -> WALL_E_ACK"));
}

static void send_mode_dock_(void) {
  char json[64];
  snprintf(json, sizeof(json), "{\"session\":%lu}", (unsigned long)s_session_id);
  dockEveLinkSendJson(MSG_MODE_DOCK, json);
  Serial.println(F("[DOCK] -> MSG_MODE_DOCK"));
}

static void start_startup_charge_prime_(uint32_t now_ms) {
#if DOCK_STARTUP_CHARGE_PRIME_MS > 0
  s_startup_prime_active = true;
  s_startup_prime_until_ms = now_ms + DOCK_STARTUP_CHARGE_PRIME_MS;
  charging_set_enable(true);
  Serial.print(F("[DOCK] Startup charge prime ON for ms="));
  Serial.println((unsigned long)DOCK_STARTUP_CHARGE_PRIME_MS);
#else
  (void)now_ms;
  s_startup_prime_active = false;
  s_startup_prime_until_ms = 0;
#endif
}

static void update_startup_charge_prime_(uint32_t now_ms) {
  if (!s_startup_prime_active) return;
  if (s_st == ST_DOCKED || s_st == ST_CHARGING) {
    s_startup_prime_active = false;
    return;
  }
  if ((int32_t)(now_ms - s_startup_prime_until_ms) < 0) {
    if (!charging_is_enabled()) {
      charging_set_enable(true);
    }
    return;
  }

#if DOCK_KEEP_CHARGE_ON_NO_EVE_ACTIVITY
  if (s_last_eve_rx_ms == 0) {
    if (!charging_is_enabled()) {
      charging_set_enable(true);
    }
    static uint32_t s_last_wait_log_ms = 0;
    if ((uint32_t)(now_ms - s_last_wait_log_ms) > 5000u) {
      s_last_wait_log_ms = now_ms;
      Serial.println(F("[DOCK] No EVE sensor/UART activity yet; holding charge ON while waiting for init"));
    }
    return;
  }
#endif

  s_startup_prime_active = false;
  charging_emergency_off();
  s_dockManager.setCharging(false);
  Serial.println(F("[DOCK] Startup charge prime ended; charging off until EVE handshake"));
}

static void enter_charging_(uint32_t now_ms) {
  s_startup_prime_active = false;
  s_last_eve_heartbeat_ms = now_ms;
  s_st = ST_DOCKED;
  send_mode_dock_();
  s_dockManager.onEveStatus(now_ms, false);
  if (charging_voltage_ok_to_enable()) {
    charging_set_enable(true);
    s_st = ST_CHARGING;
    s_dockManager.setCharging(true);
    led_set_mode(LED_MODE_GREEN_SOLID);
  }
}

static void on_eve_frame_(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
  (void)seq;
  uint32_t now_ms = millis();
  s_last_eve_rx_ms = now_ms;
  switch (type) {
    case MSG_EVE_HELLO:
      Serial.println(F("[DOCK] RX MSG_EVE_HELLO"));
      if (s_st == ST_CHARGING || s_st == ST_DOCKED) charging_emergency_off();
      s_st = ST_WAIT_FOR_DOCK;
      s_session_id = 0;
      send_dock_ack_();
      s_dockManager.onEveHello(now_ms, s_session_id);
      break;
    case MSG_EVE_READY:
      Serial.println(F("[DOCK] RX MSG_EVE_READY"));
      note_eve_voltage_from_json_(payload, len);
      enter_charging_(now_ms);
      break;
    case MSG_EVE_HEARTBEAT:
      note_eve_voltage_from_json_(payload, len);
      s_last_eve_heartbeat_ms = now_ms;
      s_dockManager.onEveStatus(now_ms, false);
      if (s_st == ST_WAIT_FOR_DOCK && s_session_id != 0) enter_charging_(now_ms);
      break;
    case MSG_BUS_MEMORY_SYNC:
      s_dockManager.onSyncBegin(now_ms);
      break;
    case MSG_EVE_LOW_BATTERY:
      Serial.println(F("[DOCK] RX MSG_EVE_LOW_BATTERY"));
      break;
    case MSG_EVE_ERROR:
      Serial.println(F("[DOCK] RX MSG_EVE_ERROR"));
      break;
    default:
      break;
  }
}

void dock_init(void) {
  s_st = ST_WAIT_FOR_DOCK;
  s_wait_t0_ms = millis();
  s_last_eve_rx_ms = 0;
  s_last_eve_heartbeat_ms = 0;
  s_session_id = 0;
  s_startup_prime_until_ms = 0;
  s_startup_prime_active = false;

  DockManagerCallbacks cb;
  cb.setCharging = dock_set_charging_;
  cb.setLight = dock_set_light_;
  cb.sendJson = dock_send_bus_json_;
  s_dockManager.begin(cb);

  dockEveLinkInit();
  dockEveLinkSetFrameCallback(on_eve_frame_);
  led_set_mode(LED_MODE_BLUE_PULSE);
  charging_emergency_off();
  start_startup_charge_prime_(s_wait_t0_ms);
}

const char* dock_state_name(DockFsmState s) {
  switch (s) {
    case ST_IDLE: return "IDLE";
    case ST_WAIT_FOR_DOCK: return "WAIT_FOR_DOCK";
    case ST_DOCKED: return "DOCKED";
    case ST_CHARGING: return "CHARGING";
    default: return "?";
  }
}

static void fail_lost_link_(void) {
  Serial.println(F("[DOCK] FAIL-SAFE: no EVE heartbeat; returning to HELLO scan"));
  charging_emergency_off();
  s_dockManager.setCharging(false);
  led_set_mode(LED_MODE_BLUE_PULSE);
  s_st = ST_WAIT_FOR_DOCK;
  s_wait_t0_ms = millis();
  s_last_eve_rx_ms = 0;
  s_last_eve_heartbeat_ms = 0;
  s_session_id = 0;
  start_startup_charge_prime_(s_wait_t0_ms);
}

void dock_update(uint32_t now_ms) {
  dockEveLinkPoll(DOCK_UART_MAX_BYTES_PER_LOOP);
  charging_update(now_ms);
  if (charging_is_safety_locked_out() && s_st == ST_CHARGING) {
    s_st = ST_DOCKED;
    s_dockManager.setCharging(false);
    led_set_mode(LED_MODE_RED_BLINK);
  }
  update_startup_charge_prime_(now_ms);
  if ((s_st == ST_DOCKED || s_st == ST_CHARGING) && s_last_eve_heartbeat_ms != 0 &&
      (uint32_t)(now_ms - s_last_eve_heartbeat_ms) > DOCK_LOST_NO_RX_MS) {
    fail_lost_link_();
  }

  s_dockManager.tick(now_ms);

  switch (s_st) {
    case ST_WAIT_FOR_DOCK:
      if (charging_is_safety_locked_out()) {
        if (led_get_mode() != LED_MODE_RED_BLINK) led_set_mode(LED_MODE_RED_BLINK);
      } else if (charging_is_enabled() && s_last_eve_rx_ms == 0) {
        if (led_get_mode() != LED_MODE_RED_BLINK) led_set_mode(LED_MODE_RED_BLINK);
      } else if (led_get_mode() != LED_MODE_BLUE_PULSE) {
        led_set_mode(LED_MODE_BLUE_PULSE);
      }
      if ((uint32_t)(now_ms - s_wait_t0_ms) > DOCK_MAX_WAIT_MS) {
        s_wait_t0_ms = now_ms;
        if (charging_is_enabled() && s_last_eve_rx_ms == 0) {
          Serial.println(F("[DOCK] Waiting for EVE init; charge ON, red flash"));
        } else {
          Serial.println(F("[DOCK] Listening for framed MSG_EVE_HELLO; charging OFF"));
        }
      }
      break;
    case ST_DOCKED:
      if (led_get_mode() != LED_MODE_BLUE_PULSE) led_set_mode(LED_MODE_BLUE_PULSE);
      break;
    case ST_CHARGING:
      if (led_get_mode() != LED_MODE_GREEN_SOLID) led_set_mode(LED_MODE_GREEN_SOLID);
      break;
    case ST_IDLE:
      if (led_get_mode() != LED_MODE_RED_BLINK) led_set_mode(LED_MODE_RED_BLINK);
      break;
  }
  led_update(now_ms);
}

DockFsmState dock_get_state(void) {
  return s_st;
}

uint32_t dock_last_eve_rx_ms(void) {
  return s_last_eve_rx_ms;
}

void dock_get_status_json(char* out, size_t out_len) {
  if (!out || out_len == 0) return;
  s_dockManager.serializeStatusJson(out, out_len, millis());
  size_t n = strlen(out);
  if (n > 0 && out[n - 1] == '}' && n + 96 < out_len) {
    out[n - 1] = '\0';
    snprintf(out + n - 1, out_len - (n - 1),
             ",\"charge_safety\":%s,\"charge_reason\":\"%s\"}",
             charging_is_safety_locked_out() ? "true" : "false",
             charging_safety_reason());
  }
}
