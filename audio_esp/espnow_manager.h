/**
 * espnow_manager.h — ESP-NOW link to Base ESP32-S3
 * Receives WalleAudioCommandPacket_t, sends mic telem + status packets.
 */
#ifndef AUDIO_ESP_ESPNOW_MANAGER_H
#define AUDIO_ESP_ESPNOW_MANAGER_H

#include "config.h"
#include "debug_log.h"
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

/* Initialize WiFi STA + ESP-NOW. Returns true on success. */
bool espnowManagerInit();

/* Returns true if ESP-NOW initialized */
bool espnowManagerIsReady();

/* Call every loop — no blocking receive; callback handles packets */
void espnowManagerTick();

/* Send mic telem (ear_l, ear_r, voice_active) — rate-limited internally */
void espnowManagerSendMicTelem(uint16_t ear_l, uint16_t ear_r, uint8_t voice_active);

/* Send status packet (mic_dir, dock_ir, voice_cmd, mode, fault, audio_busy) */
void espnowManagerSendStatus(uint8_t mic_dir, uint8_t dock_ir, uint8_t voice_cmd,
                             uint8_t mode, uint8_t fault, uint8_t audio_busy);

#endif
