#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "base_board_pins.h"

// ============================================================
// WALL-E Base radio transport
//
// Radio-capable ESP32 regression builds: native ESP-NOW.
// ESP32-P4 production Base: framed UART link to ESP32-C6/C3/S3 gateway.
// ============================================================

#ifndef WALLE_RADIO_CHANNEL
#define WALLE_RADIO_CHANNEL 11
#endif

#define WALLE_RADIO_UART_RX  BASE_PIN_RADIO_UART_RX
#define WALLE_RADIO_UART_TX  BASE_PIN_RADIO_UART_TX

#ifndef WALLE_RADIO_UART_BAUD
#define WALLE_RADIO_UART_BAUD 921600UL
#endif

using WalleRadioReceiveCallback = void (*)(const uint8_t sourceMac[6],
                                            const uint8_t* data,
                                            size_t length,
                                            int8_t rssi,
                                            uint8_t channel);

bool radioTransportInit(void);
void radioTransportPoll(void);
void radioTransportSetReceiveCallback(WalleRadioReceiveCallback callback);

bool radioTransportSend(const uint8_t destinationMac[6],
                        const void* data,
                        size_t length);
bool radioTransportBroadcast(const void* data, size_t length);

bool radioTransportIsReady(void);
uint8_t radioTransportGetChannel(void);
uint32_t radioTransportGetLastRxMs(void);
uint32_t radioTransportGetRxCount(void);
uint32_t radioTransportGetTxCount(void);
uint32_t radioTransportGetTxFailureCount(void);
