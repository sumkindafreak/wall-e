#pragma once

#include <Arduino.h>

/**
 * Optional UART link to EVE companion (same framing as eve/uart_link + eve_protocol).
 * WALL-E base pins: TX to EVE RX (GPIO18), RX from EVE TX (GPIO17) — matches eve/config.h crossover.
 */
#ifndef EVE_BRIDGE_UART_TX
#define EVE_BRIDGE_UART_TX 18
#endif
#ifndef EVE_BRIDGE_UART_RX
#define EVE_BRIDGE_UART_RX 17
#endif
#ifndef EVE_BRIDGE_BAUD
#define EVE_BRIDGE_BAUD 115200
#endif

/** Milliseconds before EVE power telemetry is considered stale / EVE offline. */
#ifndef EVE_POWER_TELEM_STALE_MS
#define EVE_POWER_TELEM_STALE_MS 5000
#endif

// ---------------------------------------------------------------------------
// EVE battery state mirror (matches eve/include/power_monitor.h BatteryState)
// ---------------------------------------------------------------------------
enum class EveBatteryState : uint8_t {
  EVE_BAT_OK       = 0,
  EVE_BAT_LOW      = 1,
  EVE_BAT_CRITICAL = 2,
  EVE_BAT_CHARGING = 3,
  EVE_BAT_FULL     = 4
};

/** Live power telemetry received from EVE over UART. */
struct EvePowerTelemetry {
  float           voltage;        /* battery voltage (V) */
  float           current;        /* current draw (A): positive = discharging */
  uint8_t         percent;        /* 0-100 % */
  EveBatteryState state;          /* named battery state */
  bool            charging;       /* true while charger detected */
  uint32_t        heartbeat;      /* EVE heartbeat counter */
  uint32_t        eveTimestamp_ms;/* millis() on EVE side at send */
  uint32_t        receivedAt_ms;  /* millis() on WALL-E side when frame arrived */
  bool            valid;          /* true once at least one packet received */
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void   eveUartBridgeInit(void);
void   eveUartBridgePoll(void);

String eveUartBridgeGetJSON(void);

/** Return the latest EVE power telemetry snapshot. */
const EvePowerTelemetry& eveGetPowerTelemetry(void);

/** True if a power packet was received recently (within EVE_POWER_TELEM_STALE_MS). */
bool   eveIsPowerTelemetryFresh(void);

/** Serialise the current EVE power telemetry as a JSON string. */
String eveGetPowerTelemetryJSON(void);

// ---------------------------------------------------------------------------
// WALL-E reaction hooks — called when EVE battery state changes.
// Implement sounds / expressions / motion / docking logic here (or leave them
// as the default log-only stubs defined in eve_uart_bridge.cpp).
// ---------------------------------------------------------------------------
void onEveBatteryOk(void);
void onEveBatteryLow(void);
void onEveBatteryCritical(void);
void onEveCharging(void);
void onEveBatteryFull(void);
