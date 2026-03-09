// ============================================================
//  WALL-E CYD Performance Console — Gesture Library
//  Prebuilt expressive animations
// ============================================================

#ifndef GESTURE_LIBRARY_H
#define GESTURE_LIBRARY_H

#include <Arduino.h>
#include "sd_manager.h"

// ============================================================
//  Gesture Types
// ============================================================

enum GestureType {
  GESTURE_CURIOUS,
  GESTURE_HAPPY,
  GESTURE_CONFUSED,
  GESTURE_SHY,
  GESTURE_EXCITED,
  GESTURE_WAVE,
  GESTURE_SAD,
  GESTURE_WONDER,
  GESTURE_NOD_YES,
  GESTURE_SHAKE_NO,
  GESTURE_LEAN_FORWARD,
  GESTURE_LEAN_BACK,
  GESTURE_COUNT
};

// ============================================================
//  Gesture Structure
// ============================================================

struct Gesture {
  const char* name;
  uint16_t durationMs;
  uint8_t keyframeCount;
  const AnimKeyframe* keyframes;
};

// ============================================================
//  Public API
// ============================================================

// Initialize gesture library
void gestureInit();

// Get gesture by type
const Gesture* gestureGet(GestureType type);

// Get gesture name
const char* gestureGetName(GestureType type);

// Play gesture (returns duration in ms)
uint16_t gesturePlay(GestureType type);

// Check if gesture is currently playing
bool gestureIsPlaying();

// Update gesture playback (call every loop)
void gestureUpdate(uint32_t now);

// Get current gesture output (servo offsets)
bool gestureGetOutput(float offsets[9]);

// Cancel current gesture
void gestureCancel();

#endif // GESTURE_LIBRARY_H
