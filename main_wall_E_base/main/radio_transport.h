#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================
// WALL-E Base radio transport — ESP32-S3 production target
//
// The S3 owns Wi-Fi/ESP-NOW directly. CYD, audio, vision and dock packets all
// pass through this one abstraction, but there is no external radio gateway.
// ============================================================

#ifndef WALLE_RADIO_CHANNEL
#define WALLE_RADIO_CHANNEL 11
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
