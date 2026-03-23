/**
 * event_router.h — Route events to ESP-NOW, no UART parsing
 */
#ifndef AUDIO_ESP_EVENT_ROUTER_H
#define AUDIO_ESP_EVENT_ROUTER_H

#include "config.h"
#include "system_state.h"
#include "espnow_manager.h"
#include "audio_player.h"
#include "mic_manager.h"
#include "voice_commands.h"
#include "ir_dock_receivers.h"
#include "diagnostics.h"
#include "debug_log.h"
#include <Arduino.h>

/* Initialize event router */
void eventRouterInit();

/* No UART — incoming handled by ESP-NOW callback */
void eventRouterProcessIncoming();

/* Emit heartbeat, mode, and state changes as needed */
void eventRouterTick();

/* Called by other modules to report events — triggers ESP-NOW status send */
void eventEmitVoiceCmd(const char* cmd);
void eventEmitMicDir(const char* dir);
void eventEmitDockIr(const char* state);
void eventEmitAudioBusy(bool busy);
void eventEmitAudioDone();
void eventEmitFault(const char* fault);
void eventEmitMode(const char* mode);

/* Send combined status (mic dir, dock IR, mode, etc.) via ESP-NOW */
void eventRouterSendStatus(void);

#endif
