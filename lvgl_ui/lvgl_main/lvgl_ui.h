#pragma once

#include <Arduino.h>
#include "../../wall_e_master_controller/protocol.h"
#include "../../wall_e_master_controller/ads1115_input.h"

void lvglUiInit(void);
void lvglUiTick(uint32_t nowMs);

void lvglUiSetTelemetry(const TelemetryPacket* tm, bool valid);
void lvglUiSetJoystick(const JoystickState* js);
void lvglUiSetDriveState(const DriveState* ds);

// --- ADDED: Toast API ---
void lvglUiShowToast(const char* msg, uint16_t durationMs);

// --- ADDED: Keyboard API ---
typedef void (*LvglKeyboardCallback)(const char* text, void* userData);

void lvglUiShowKeyboard(const char* initialText,
                        LvglKeyboardCallback cb,
                        void* userData);

// --- ADDED: SD availability bridge ---
void lvglUiSetSdReady(bool ready);
bool lvglUiIsSdReady(void);

/** Modal: edit joystick / head tuning for the active profile (runtime). */
void lvglUiProfileEditorOpen(void);

/** Soundboard: broadcast WAV track index (1–255) to audio node via ESP-NOW. */
void lvglUiActionPlayAudioTrack(uint8_t track);
