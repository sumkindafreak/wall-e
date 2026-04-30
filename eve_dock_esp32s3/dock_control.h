#pragma once
#include <Arduino.h>
enum DockFsmState : uint8_t { ST_IDLE = 0, ST_WAIT_FOR_DOCK, ST_DOCKED, ST_CHARGING };
void dock_init(void);
void dock_update(uint32_t now_ms);
DockFsmState dock_get_state(void);
const char* dock_state_name(DockFsmState s);
uint32_t dock_last_eve_rx_ms(void);
void dock_get_status_json(char* out, size_t out_len);
