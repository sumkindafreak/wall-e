#include "dock_control.h"
#include "charging_control.h"
#include "dock_eve_link.h"
#include "led_status.h"
#include <esp_system.h>
#include <stdio.h>
static DockFsmState s_st = ST_WAIT_FOR_DOCK;
static uint32_t s_wait_t0_ms = 0, s_last_eve_rx_ms = 0, s_last_eve_heartbeat_ms = 0, s_session_id = 0;
static uint32_t make_session_id_(void) { uint32_t sid = ((uint32_t)(uint16_t)esp_random() << 16) ^ (uint32_t)millis(); return sid == 0 ? 1u : sid; }
static void send_dock_ack_(void) { if (s_session_id == 0) s_session_id = make_session_id_(); char json[96]; snprintf(json, sizeof(json), "{\"session\":%lu,\"peer\":\"eve_dock_c3\"}", (unsigned long)s_session_id); dockEveLinkSendJson(MSG_WALL_E_ACK, json); Serial.println(F("[DOCK] -> WALL_E_ACK")); }
static void send_mode_dock_(void) { char json[64]; snprintf(json, sizeof(json), "{\"session\":%lu}", (unsigned long)s_session_id); dockEveLinkSendJson(MSG_MODE_DOCK, json); Serial.println(F("[DOCK] -> MSG_MODE_DOCK")); }
static void enter_charging_(uint32_t now_ms) { s_last_eve_heartbeat_ms = now_ms; s_st = ST_DOCKED; send_mode_dock_(); if (charging_voltage_ok_to_enable()) { charging_set_enable(true); s_st = ST_CHARGING; led_set_mode(LED_MODE_GREEN_SOLID); } }
static void on_eve_frame_(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) { (void)payload; (void)len; (void)seq; uint32_t now_ms = millis(); s_last_eve_rx_ms = now_ms; switch (type) { case MSG_EVE_HELLO: Serial.println(F("[DOCK] RX MSG_EVE_HELLO")); if (s_st == ST_CHARGING || s_st == ST_DOCKED) charging_emergency_off(); s_st = ST_WAIT_FOR_DOCK; s_session_id = 0; send_dock_ack_(); break; case MSG_EVE_READY: Serial.println(F("[DOCK] RX MSG_EVE_READY")); enter_charging_(now_ms); break; case MSG_EVE_HEARTBEAT: s_last_eve_heartbeat_ms = now_ms; if (s_st == ST_WAIT_FOR_DOCK && s_session_id != 0) enter_charging_(now_ms); break; case MSG_EVE_LOW_BATTERY: Serial.println(F("[DOCK] RX MSG_EVE_LOW_BATTERY")); break; case MSG_EVE_ERROR: Serial.println(F("[DOCK] RX MSG_EVE_ERROR")); break; default: break; } }
void dock_init(void) { s_st = ST_WAIT_FOR_DOCK; s_wait_t0_ms = millis(); s_last_eve_rx_ms = 0; s_last_eve_heartbeat_ms = 0; s_session_id = 0; dockEveLinkInit(); dockEveLinkSetFrameCallback(on_eve_frame_); led_set_mode(LED_MODE_BLUE_PULSE); charging_emergency_off(); }
const char* dock_state_name(DockFsmState s) { switch (s) { case ST_IDLE: return "IDLE"; case ST_WAIT_FOR_DOCK: return "WAIT_FOR_DOCK"; case ST_DOCKED: return "DOCKED"; case ST_CHARGING: return "CHARGING"; default: return "?"; } }
static void fail_lost_link_(void) { Serial.println(F("[DOCK] FAIL-SAFE: no EVE heartbeat")); charging_emergency_off(); led_set_mode(LED_MODE_RED_BLINK); s_st = ST_IDLE; s_last_eve_rx_ms = 0; s_last_eve_heartbeat_ms = 0; s_session_id = 0; }
void dock_update(uint32_t now_ms) { dockEveLinkPoll(DOCK_UART_MAX_BYTES_PER_LOOP); if ((s_st == ST_DOCKED || s_st == ST_CHARGING) && s_last_eve_heartbeat_ms != 0 && (uint32_t)(now_ms - s_last_eve_heartbeat_ms) > DOCK_LOST_NO_RX_MS) fail_lost_link_(); switch (s_st) { case ST_WAIT_FOR_DOCK: if (led_get_mode() != LED_MODE_BLUE_PULSE) led_set_mode(LED_MODE_BLUE_PULSE); if ((uint32_t)(now_ms - s_wait_t0_ms) > DOCK_MAX_WAIT_MS) { s_wait_t0_ms = now_ms; Serial.println(F("[DOCK] Listening for framed MSG_EVE_HELLO; charging OFF")); } break; case ST_DOCKED: if (led_get_mode() != LED_MODE_BLUE_PULSE) led_set_mode(LED_MODE_BLUE_PULSE); break; case ST_CHARGING: if (led_get_mode() != LED_MODE_GREEN_SOLID) led_set_mode(LED_MODE_GREEN_SOLID); break; case ST_IDLE: if (led_get_mode() != LED_MODE_RED_BLINK) led_set_mode(LED_MODE_RED_BLINK); break; } led_update(now_ms); }
DockFsmState dock_get_state(void) { return s_st; }
uint32_t dock_last_eve_rx_ms(void) { return s_last_eve_rx_ms; }
