// ============================================================
//  WALL-E CYD Performance Console — Gesture Library Implementation
// ============================================================

#include "gesture_library.h"

// ============================================================
//  Gesture Keyframe Data (Prebuilt Animations)
// ============================================================

// CURIOUS - Head tilt + eyes wide
static const AnimKeyframe g_curiousFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},           // Start neutral
  {200,  {0, 15, 0, 0, 10, -10, 0, 0, 0}},       // Tilt head, eyes up
  {600,  {0, 20, 0, 0, 15, -15, 0, 0, 0}},       // Hold
  {1000, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return neutral
};

// HAPPY - Eyebrows up + slight bounce
static const AnimKeyframe g_happyFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, -15, -15, 0}},       // Eyebrows up
  {150,  {0, -5, 0, 0, 5, -5, -20, -20, 0}},     // Slight down
  {300,  {0, 5, 0, 0, 8, -8, -15, -15, 0}},      // Bounce up
  {450,  {0, -3, 0, 0, 5, -5, -20, -20, 0}},     // Down
  {600,  {0, 0, 0, 0, 0, 0, -15, -15, 0}},       // Up
  {800,  {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Neutral
};

// CONFUSED - Head shake + eyebrows asymmetric
static const AnimKeyframe g_confusedFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {200,  {-15, 0, 0, 0, 0, 0, 10, -5, 0}},       // Turn left, eyebrow up
  {400,  {15, 0, 0, 0, 0, 0, -5, 10, 0}},        // Turn right
  {600,  {-10, 0, 0, 0, 0, 0, 10, -5, 0}},       // Left again
  {800,  {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Center
};

// SHY - Look down + away
static const AnimKeyframe g_shyFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {400,  {-20, 10, 0, 0, -10, 5, 5, 8, 0}},      // Look down-left
  {1000, {-20, 10, 0, 0, -10, 5, 5, 8, 0}},      // Hold
  {1400, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return
};

// EXCITED - Rapid movements + eyes wide
static const AnimKeyframe g_excitedFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {100,  {-10, -10, 0, 0, 15, -15, -20, -20, 0}},
  {200,  {10, -8, 0, 0, 12, -12, -18, -18, 0}},
  {300,  {-8, -10, 0, 0, 15, -15, -20, -20, 0}},
  {400,  {8, -8, 0, 0, 12, -12, -18, -18, 0}},
  {500,  {0, 0, 0, 0, 10, -10, -15, -15, 0}},
  {700,  {0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

// WAVE - Arm wave
static const AnimKeyframe g_waveFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {300,  {0, 0, 0, 0, 0, 0, 0, 0, 30}},          // Raise arm
  {500,  {0, 0, 0, 0, 0, 0, 0, 0, 40}},          // Higher
  {650,  {0, 0, 0, 0, 0, 0, 0, 0, 30}},          // Wave down
  {800,  {0, 0, 0, 0, 0, 0, 0, 0, 40}},          // Wave up
  {950,  {0, 0, 0, 0, 0, 0, 0, 0, 30}},          // Wave down
  {1200, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Lower
};

// SAD - Head down + eyebrows down
static const AnimKeyframe g_sadFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {600,  {0, 20, 0, 0, -10, 10, 15, 15, 0}},     // Head down, eyebrows sad
  {1500, {0, 20, 0, 0, -10, 10, 15, 15, 0}},     // Hold
  {2100, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return slow
};

// WONDER - Look up slowly
static const AnimKeyframe g_wonderFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {800,  {0, -25, 0, 0, 20, -20, -10, -10, 0}},  // Look up, eyes wide
  {2000, {0, -25, 0, 0, 20, -20, -10, -10, 0}},  // Hold gaze
  {2400, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return
};

// NOD YES - Vertical nod
static const AnimKeyframe g_nodYesFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {200,  {0, -15, 0, 0, 0, 0, 0, 0, 0}},         // Up
  {400,  {0, 15, 0, 0, 0, 0, 0, 0, 0}},          // Down
  {600,  {0, -10, 0, 0, 0, 0, 0, 0, 0}},         // Up
  {800,  {0, 10, 0, 0, 0, 0, 0, 0, 0}},          // Down
  {1000, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Center
};

// SHAKE NO - Horizontal shake
static const AnimKeyframe g_shakeNoFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {200,  {-20, 0, 0, 0, 0, 0, 0, 0, 0}},         // Left
  {400,  {20, 0, 0, 0, 0, 0, 0, 0, 0}},          // Right
  {600,  {-15, 0, 0, 0, 0, 0, 0, 0, 0}},         // Left
  {800,  {15, 0, 0, 0, 0, 0, 0, 0, 0}},          // Right
  {1000, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Center
};

// LEAN FORWARD - Curious lean
static const AnimKeyframe g_leanForwardFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {500,  {0, -10, 0, 0, 10, -10, -8, -8, 0}},    // Lean forward
  {1200, {0, -10, 0, 0, 10, -10, -8, -8, 0}},    // Hold
  {1700, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return
};

// LEAN BACK - Surprised/defensive
static const AnimKeyframe g_leanBackFrames[] = {
  {0,    {0, 0, 0, 0, 0, 0, 0, 0, 0}},
  {300,  {0, 15, 0, 0, -15, 15, -15, -15, 0}},   // Lean back quick
  {1000, {0, 15, 0, 0, -15, 15, -15, -15, 0}},   // Hold
  {1400, {0, 0, 0, 0, 0, 0, 0, 0, 0}}            // Return
};

// ============================================================
//  Gesture Definitions
// ============================================================

static const Gesture g_gestures[GESTURE_COUNT] = {
  {"Curious",      1000, 4,  g_curiousFrames},
  {"Happy",        800,  6,  g_happyFrames},
  {"Confused",     800,  5,  g_confusedFrames},
  {"Shy",          1400, 4,  g_shyFrames},
  {"Excited",      700,  7,  g_excitedFrames},
  {"Wave",         1200, 7,  g_waveFrames},
  {"Sad",          2100, 4,  g_sadFrames},
  {"Wonder",       2400, 4,  g_wonderFrames},
  {"Nod Yes",      1000, 6,  g_nodYesFrames},
  {"Shake No",     1000, 6,  g_shakeNoFrames},
  {"Lean Forward", 1700, 4,  g_leanForwardFrames},
  {"Lean Back",    1400, 4,  g_leanBackFrames}
};

// ============================================================
//  Playback State
// ============================================================

static bool s_playing = false;
static GestureType s_currentGesture = GESTURE_CURIOUS;
static uint32_t s_startTimeMs = 0;
static float s_currentOffsets[9] = {0};

// ============================================================
//  Initialization
// ============================================================

void gestureInit() {
  s_playing = false;
  Serial.println("[Gesture] Library loaded - 12 gestures available");
}

// ============================================================
//  Gesture Access
// ============================================================

const Gesture* gestureGet(GestureType type) {
  if (type >= GESTURE_COUNT) return nullptr;
  return &g_gestures[type];
}

const char* gestureGetName(GestureType type) {
  if (type >= GESTURE_COUNT) return "Unknown";
  return g_gestures[type].name;
}

// ============================================================
//  Playback Control
// ============================================================

uint16_t gesturePlay(GestureType type) {
  if (type >= GESTURE_COUNT) return 0;
  
  s_playing = true;
  s_currentGesture = type;
  s_startTimeMs = millis();
  
  Serial.printf("[Gesture] Playing: %s\n", g_gestures[type].name);
  return g_gestures[type].durationMs;
}

bool gestureIsPlaying() {
  return s_playing;
}

void gestureCancel() {
  if (s_playing) {
    s_playing = false;
    memset(s_currentOffsets, 0, sizeof(s_currentOffsets));
    Serial.println("[Gesture] Cancelled");
  }
}

// ============================================================
//  Update & Interpolation
// ============================================================

void gestureUpdate(uint32_t now) {
  if (!s_playing) return;
  
  const Gesture* gesture = &g_gestures[s_currentGesture];
  uint32_t elapsed = now - s_startTimeMs;
  
  // Check if finished
  if (elapsed >= gesture->durationMs) {
    s_playing = false;
    memset(s_currentOffsets, 0, sizeof(s_currentOffsets));
    Serial.println("[Gesture] Complete");
    return;
  }
  
  // Find current keyframe pair for interpolation
  const AnimKeyframe* prevFrame = &gesture->keyframes[0];
  const AnimKeyframe* nextFrame = &gesture->keyframes[1];
  
  for (uint8_t i = 0; i < gesture->keyframeCount - 1; i++) {
    if (elapsed >= gesture->keyframes[i].timeMs && 
        elapsed < gesture->keyframes[i + 1].timeMs) {
      prevFrame = &gesture->keyframes[i];
      nextFrame = &gesture->keyframes[i + 1];
      break;
    }
  }
  
  // Linear interpolation
  uint16_t frameDuration = nextFrame->timeMs - prevFrame->timeMs;
  if (frameDuration == 0) frameDuration = 1;
  
  uint32_t frameElapsed = elapsed - prevFrame->timeMs;
  float t = (float)frameElapsed / (float)frameDuration;
  t = constrain(t, 0.0f, 1.0f);
  
  // Interpolate each servo offset
  for (int i = 0; i < 9; i++) {
    s_currentOffsets[i] = prevFrame->offset[i] + 
                          (nextFrame->offset[i] - prevFrame->offset[i]) * t;
  }
}

bool gestureGetOutput(float offsets[9]) {
  if (!s_playing) return false;
  
  memcpy(offsets, s_currentOffsets, sizeof(s_currentOffsets));
  return true;
}
