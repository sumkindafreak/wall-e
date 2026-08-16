#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================
// WALL-E Base radio transport
//
// ESP32-S3/other Wi-Fi ESPs: native ESP-NOW.
// ESP32-P4: framed UART link to a dedicated ESP32 radio gateway.
// ============================================================

#ifndef WALLE_RADIO_CHANNEL
#define WALLE_RADIO_CHANNEL 11
#endif

// P4 UART defaults. These are ordinary GPIO-matrix UART pins and can be
// changed here to match the two header pins chosen for the gateway link.
#ifndef WALLE_RADIO_UART_RX
#define WALLE_RADIO_UART_RX 24
#endif
#ifndef WALLE_RADIO_UART_TX
#define WALLE_RADIO_UART_TX 25
#endif
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
