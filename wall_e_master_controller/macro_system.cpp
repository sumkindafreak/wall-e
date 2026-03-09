// ============================================================
//  WALL-E CYD Master Controller — Macro System Implementation
// ============================================================

#include "macro_system.h"

// ============================================================
//  Internal State
// ============================================================

static MacroState s_state = MACRO_IDLE;

// IMPORTANT: Use heap allocation to avoid DRAM overflow
static MacroFrame* s_frames = nullptr;
static uint16_t s_frameCapacity = MAX_MACRO_FRAMES;

static uint16_t s_frameCount = 0;
static uint16_t s_currentFrame = 0;
static uint32_t s_recordStartMs = 0;
static uint32_t s_playbackStartMs = 0;
static uint32_t s_lastCaptureMs = 0;

// Current data for recording
static float s_currentTrackLeft = 0.0f;
static float s_currentTrackRight = 0.0f;
static float s_currentServos[9] = {50, 50, 50, 50, 50, 50, 50, 50, 50};

// ============================================================
//  Initialization
// ============================================================

void macroInit() {
  s_state = MACRO_IDLE;
  s_frameCount = 0;
  s_currentFrame = 0;
  
  // Allocate frame buffer from heap (avoids DRAM overflow)
  s_frames = (MacroFrame*)malloc(sizeof(MacroFrame) * MAX_MACRO_FRAMES);
  
  if (s_frames == nullptr) {
    Serial.println("[Macro] ⚠️  Failed to allocate frame buffer");
    s_frameCapacity = 0;
  } else {
    s_frameCapacity = MAX_MACRO_FRAMES;
    Serial.printf("[Macro] System ready (%d frames, %.1f KB)\n", 
      s_frameCapacity, (s_frameCapacity * sizeof(MacroFrame)) / 1024.0f);
  }
}

// ============================================================
//  Recording
// ============================================================

void macroStartRecording() {
  if (s_state != MACRO_IDLE) {
    Serial.println("[Macro] ⚠️  Already busy");
    return;
  }
  
  if (s_frames == nullptr) {
    Serial.println("[Macro] ⚠️  No frame buffer available");
    return;
  }
  
  s_state = MACRO_RECORDING;
  s_frameCount = 0;
  s_recordStartMs = millis();
  s_lastCaptureMs = s_recordStartMs;
  
  Serial.println("[Macro] 🔴 RECORDING");
}

bool macroStopRecordingAndSave(uint8_t slot) {
  if (s_state != MACRO_RECORDING) return false;
  
  s_state = MACRO_IDLE;
  
  if (s_frameCount == 0) {
    Serial.println("[Macro] ⚠️  No frames captured");
    return false;
  }
  
  Serial.printf("[Macro] ⏹️  Stopped - %d frames\n", s_frameCount);
  
  // Save to SD
  if (sdSaveMacro(slot, s_frames, s_frameCount)) {
    Serial.printf("[Macro] ✓ Saved to slot %d\n", slot);
    return true;
  } else {
    Serial.println("[Macro] ⚠️  Save failed");
    return false;
  }
}

void macroCancelRecording() {
  if (s_state == MACRO_RECORDING) {
    s_state = MACRO_IDLE;
    s_frameCount = 0;
    Serial.println("[Macro] ❌ Recording cancelled");
  }
}

// ============================================================
//  Playback
// ============================================================

bool macroStartPlayback(uint8_t slot) {
  if (s_state != MACRO_IDLE) {
    Serial.println("[Macro] ⚠️  Already busy");
    return false;
  }
  
  if (s_frames == nullptr) {
    Serial.println("[Macro] ⚠️  No frame buffer available");
    return false;
  }
  
  // Load from SD
  s_frameCount = sdLoadMacro(slot, s_frames, s_frameCapacity);
  
  if (s_frameCount == 0) {
    Serial.printf("[Macro] ⚠️  Slot %d is empty\n", slot);
    return false;
  }
  
  s_state = MACRO_PLAYING;
  s_currentFrame = 0;
  s_playbackStartMs = millis();
  
  Serial.printf("[Macro] ▶️  Playing slot %d (%d frames)\n", slot, s_frameCount);
  return true;
}

void macroStopPlayback() {
  if (s_state == MACRO_PLAYING) {
    s_state = MACRO_IDLE;
    s_currentFrame = 0;
    Serial.println("[Macro] ⏹️  Playback stopped");
  }
}

// ============================================================
//  Update (Non-Blocking)
// ============================================================

void macroUpdate(uint32_t now) {
  if (s_frames == nullptr) return;  // No buffer available
  
  if (s_state == MACRO_RECORDING) {
    // Capture frame at specified rate
    if (now - s_lastCaptureMs >= MACRO_CAPTURE_MS) {
      if (s_frameCount < s_frameCapacity) {
        MacroFrame* frame = &s_frames[s_frameCount];
        frame->timeOffset = now - s_recordStartMs;
        frame->trackLeft = s_currentTrackLeft;
        frame->trackRight = s_currentTrackRight;
        for (int i = 0; i < 9; i++) {
          frame->servo[i] = s_currentServos[i];
        }
        s_frameCount++;
        s_lastCaptureMs = now;
      } else {
        // Buffer full - auto-stop
        Serial.println("[Macro] ⚠️  Max frames reached - auto-stopping");
        s_state = MACRO_IDLE;
      }
    }
  }
  else if (s_state == MACRO_PLAYING) {
    // Time-based playback
    uint32_t elapsed = now - s_playbackStartMs;
    
    // Find current frame based on elapsed time
    while (s_currentFrame < s_frameCount && 
           s_frames[s_currentFrame].timeOffset <= elapsed) {
      s_currentFrame++;
    }
    
    // Check if finished
    if (s_currentFrame >= s_frameCount) {
      Serial.println("[Macro] ✓ Playback complete");
      s_state = MACRO_IDLE;
      s_currentFrame = 0;
    }
  }
}

// ============================================================
//  State Queries
// ============================================================

MacroState macroGetState() {
  return s_state;
}

bool macroIsRecording() {
  return s_state == MACRO_RECORDING;
}

bool macroIsPlaying() {
  return s_state == MACRO_PLAYING;
}

uint16_t macroGetCurrentFrame() {
  return s_currentFrame;
}

uint16_t macroGetTotalFrames() {
  return s_frameCount;
}

float macroGetProgress() {
  if (s_frameCount == 0) return 0.0f;
  return (float)s_currentFrame / (float)s_frameCount;
}

// ============================================================
//  Data Input/Output
// ============================================================

void macroSetCurrentData(float trackLeft, float trackRight, const float servos[9]) {
  s_currentTrackLeft = trackLeft;
  s_currentTrackRight = trackRight;
  for (int i = 0; i < 9; i++) {
    s_currentServos[i] = servos[i];
  }
}

bool macroGetPlaybackData(float* trackLeft, float* trackRight, float servos[9]) {
  if (s_state != MACRO_PLAYING || s_currentFrame == 0) return false;
  
  // Get previous frame (current frame is the next one to reach)
  uint16_t frameIdx = s_currentFrame - 1;
  if (frameIdx >= s_frameCount) return false;
  
  const MacroFrame* frame = &s_frames[frameIdx];
  *trackLeft = frame->trackLeft;
  *trackRight = frame->trackRight;
  for (int i = 0; i < 9; i++) {
    servos[i] = frame->servo[i];
  }
  
  return true;
}

// ============================================================
//  Joystick Override
// ============================================================

void macroCheckJoystickOverride(bool joystickActive) {
  if (s_state == MACRO_PLAYING && joystickActive) {
    Serial.println("[Macro] ⚠️  Joystick override - stopping playback");
    macroStopPlayback();
  }
}

// ============================================================
//  Export as Animation
// ============================================================

bool macroExportAsAnimation(uint8_t slot, const char* animName) {
  if (s_frames == nullptr) return false;
  
  // Load macro
  uint16_t frameCount = sdLoadMacro(slot, s_frames, s_frameCapacity);
  if (frameCount == 0) return false;
  
  // Convert to animation keyframes
  AnimKeyframe* keyframes = new AnimKeyframe[frameCount];
  if (!keyframes) return false;
  
  for (uint16_t i = 0; i < frameCount; i++) {
    keyframes[i].timeMs = (uint16_t)(s_frames[i].timeOffset);
    for (int j = 0; j < 9; j++) {
      // Convert absolute position to offset from neutral (50)
      keyframes[i].offset[j] = s_frames[i].servo[j] - 50.0f;
    }
  }
  
  // Save as animation
  bool success = sdSaveAnimation(animName, keyframes, frameCount);
  delete[] keyframes;
  
  if (success) {
    Serial.printf("[Macro] ✓ Exported slot %d as '%s'\n", slot, animName);
  } else {
    Serial.println("[Macro] ⚠️  Export failed");
  }
  
  return success;
}
