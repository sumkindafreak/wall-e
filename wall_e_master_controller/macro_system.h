// ============================================================
//  WALL-E CYD Master Controller — Macro Recording & Playback
//  Professional real-time macro system with SD persistence
// ============================================================

#ifndef MACRO_SYSTEM_H
#define MACRO_SYSTEM_H

#include <Arduino.h>
#include "sd_manager.h"

// ============================================================
//  Configuration
// ============================================================

#define MAX_MACRO_FRAMES  500    // ~15 seconds at 33Hz (was 2000)
#define MACRO_CAPTURE_MS  30     // Capture every 30ms

// ============================================================
//  Macro State
// ============================================================

enum MacroState {
  MACRO_IDLE,
  MACRO_RECORDING,
  MACRO_PLAYING
};

// ============================================================
//  Public API
// ============================================================

// Initialize macro system
void macroInit();

// Start recording
void macroStartRecording();

// Stop recording and save to slot
bool macroStopRecordingAndSave(uint8_t slot);

// Cancel recording without saving
void macroCancelRecording();

// Load and start playback
bool macroStartPlayback(uint8_t slot);

// Stop playback
void macroStopPlayback();

// Update (call every loop) - handles recording/playback timing
void macroUpdate(uint32_t now);

// Get current state
MacroState macroGetState();
bool macroIsRecording();
bool macroIsPlaying();

// Get recording/playback info
uint16_t macroGetCurrentFrame();
uint16_t macroGetTotalFrames();
float macroGetProgress();  // 0.0 - 1.0

// Set current servo and track data for recording
void macroSetCurrentData(float trackLeft, float trackRight, const float servos[9]);

// Get playback output (for motion engine)
bool macroGetPlaybackData(float* trackLeft, float* trackRight, float servos[9]);

// Check if joystick override occurred (cancels playback)
void macroCheckJoystickOverride(bool joystickActive);

// Export recorded macro as animation
bool macroExportAsAnimation(uint8_t slot, const char* animName);

#endif // MACRO_SYSTEM_H
