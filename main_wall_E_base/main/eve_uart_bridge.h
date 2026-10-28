#pragma once

#include <Arduino.h>

/**
 * UART link to the EVE companion (frame format: firmware_common/include/eve_link_protocol.h).
 * WALL-E base pins: TX to EVE RX (GPIO18), RX from EVE TX (GPIO17) — matches eve/config.h crossover.
 *
 * The link logic (handshake, heartbeats, supervision, EVE status) is in eve_peer_link.cpp;
 * this file owns the serial port.
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

void eveUartBridgeInit(void);
void eveUartBridgePoll(void);

/** /api/eve/status JSON: link health, EVE's reported state (stop, faults, battery...), last result. */
String eveUartBridgeGetJSON(void);

/** True while the handshake is complete and EVE is still heard from (UART link, not hand contact). */
bool eveUartBridgeIsLinkUp(void);
/** EVE reported a latched stop in her last heartbeat. */
bool eveUartBridgeEveStopped(void);

/** Send MSG_PLAY_SOUND with legacy track index on EVE (resolved to SD WAV path). */
bool eveUartBridgeSendPlaySound(uint8_t track);

/** CYD UI: head pan + right arm targets (JSON). EVE clamps to her calibrated limits. */
bool eveUartBridgeSendCydServo(uint8_t headPanDeg, uint8_t rightArmDeg);

/** Shared UART intelligence bus: send COMMAND JSON to EVE over the existing UART only. */
bool eveUartBridgeSendBusCommand(const char* jsonUtf8);

/** Companion behaviour token JSON (e.g. {"m":"EVE_LOOK_LEFT"}) to EVE UART bus. */
bool eveUartBridgeSendCompanionToEve(const char* jsonUtf8);

/** Debug/live monitor alias for WebUI. */
String eveUartBridgeGetMonitorJSON(void);

/** Diagnostic only: send a WALL_E_ACK for the given session (the handshake normally runs itself). */
bool eveUartBridgeSendWallEAck(uint32_t session);

// ---- Safety and behaviour commands ------------------------------------------------------------
/** Latch EVE's stop. Sent even before the handshake. */
bool eveUartBridgeSendStop(const char* src);
/** Explicit clear of EVE's latched stop (needs the link). */
bool eveUartBridgeSendClearStop(void);
/** Mode by name: escort, dock, idle, interact, standby, attach. */
bool eveUartBridgeSendModeByName(const char* name);
/** Operator routine: greet, scan (animation only), play, protect, wake. */
bool eveUartBridgeSendRoutine(const char* name);
bool eveUartBridgeSendEyes(const char* name, uint16_t holdMs);
bool eveUartBridgeSendGlow(const char* name);
/** Ask EVE to restart the handshake (her stop stays latched). */
bool eveUartBridgeSendReset(void);
