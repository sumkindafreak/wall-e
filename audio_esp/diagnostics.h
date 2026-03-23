/**
 * diagnostics.h — Print mode, mics, IR, audio, heartbeat, faults
 */
#ifndef AUDIO_ESP_DIAGNOSTICS_H
#define AUDIO_ESP_DIAGNOSTICS_H

#include "config.h"
#include "system_state.h"
#include "audio_player.h"
#include "mic_manager.h"
#include "ir_dock_receivers.h"
#include "debug_log.h"
#include <Arduino.h>

/* Initialize diagnostics (timing) */
void diagnosticsInit();

/* Call periodically to print diagnostic block to Serial */
void diagnosticsTick();

#endif
