// ============================================================
//  WALL-E Master Controller — Motion Engine Implementation
// ============================================================

#include "motion_engine.h"
#include "animation_data.h"
#include <Arduino.h>

// Internal state
static ServoState s_state;
static AnimationTrack s_tracks[MAX_ANIMATION_TRACKS];

// Head velocity integration
static float s_headPanPos = 90.0f;   // Current pan position (degrees)
static float s_headTiltPos = 90.0f;  // Current tilt position (degrees)

// Default neutral positions (degrees) — 9 servos
static uint8_t s_neutralPos[SERVO_COUNT] = {
  90,  // HEAD_PAN
  90,  // HEAD_TILT
  90,  // EYE_LEFT
  90,  // EYE_RIGHT
  90,  // NECK_TOP
  90,  // NECK_BOTTOM
  90,  // LEFT_ARM
  90,  // RIGHT_ARM
  90   // EYEBROW_RIGHT
};

// Smoothing parameters
#define SERVO_SMOOTH_FACTOR 0.2f  // 0.0 = instant, 1.0 = no movement
#define HEAD_VELOCITY_SCALE 60.0f  // degrees per second at full stick

// Profile tuning (set by profile system)
static float s_headSensitivity = 1.0f;   // 0.5 to 2.0 multiplier
static float s_servoSpeedLimit = 1.0f;   // 0.0 to 1.0 max speed

void motionInit() {
  // Initialize state
  for (int i = 0; i < SERVO_COUNT; i++) {
    s_state.basePosition[i] = s_neutralPos[i];
    s_state.animationOffset[i] = 0.0f;
    s_state.joystickOffset[i] = 0.0f;
    s_state.joystickInfluence[i] = 0.0f;
    s_state.targetPosition[i] = s_neutralPos[i];
    s_state.currentPosition[i] = s_neutralPos[i];
  }
  
  // Initialize animation tracks
  for (int i = 0; i < MAX_ANIMATION_TRACKS; i++) {
    s_tracks[i].active = false;
    s_tracks[i].startTime = 0;
    s_tracks[i].fadeWeight = 0.0f;
    s_tracks[i].animId = 0;
    s_tracks[i].currentFrame = 0;
    s_tracks[i].frameStartTime = 0;
  }
  
  s_headPanPos = s_neutralPos[SERVO_HEAD_PAN];
  s_headTiltPos = s_neutralPos[SERVO_HEAD_TILT];
  
  Serial.println(F("[Motion] Engine initialized"));
}

void motionUpdate(unsigned long now) {
  static unsigned long lastUpdate = 0;
  float dt = (now - lastUpdate) / 1000.0f;  // seconds
  if (dt > 0.1f) dt = 0.02f;  // Cap at 100ms (startup or lag)
  lastUpdate = now;
  
  // Clear animation offsets (will be recalculated from active tracks)
  for (int i = 0; i < SERVO_COUNT; i++) {
    s_state.animationOffset[i] = 0.0f;
  }
  
  // Update animation tracks
  for (int t = 0; t < MAX_ANIMATION_TRACKS; t++) {
    if (!s_tracks[t].active) continue;
    
    AnimationTrack* track = &s_tracks[t];
    
    // Update crossfade weight
    unsigned long elapsed = now - track->startTime;
    if (elapsed < ANIMATION_CROSSFADE_MS) {
      track->fadeWeight = (float)elapsed / (float)ANIMATION_CROSSFADE_MS;
    } else {
      track->fadeWeight = 1.0f;
    }
    
    // Get animation definition
    if (track->animId >= ANIMATION_COUNT) continue;
    
    AnimationDef animDef;
    memcpy_P(&animDef, &animationLibrary[track->animId], sizeof(AnimationDef));
    
    // Check if current frame time expired
    unsigned long frameElapsed = now - track->frameStartTime;
    AnimationFrame frame;
    memcpy_P(&frame, &animDef.frames[track->currentFrame], sizeof(AnimationFrame));
    
    if (frameElapsed >= frame.timeMs) {
      // Advance to next frame
      track->currentFrame++;
      if (track->currentFrame >= animDef.frameCount) {
        // Animation finished
        track->active = false;
        continue;
      }
      track->frameStartTime = now;
      memcpy_P(&frame, &animDef.frames[track->currentFrame], sizeof(AnimationFrame));
    }
    
    // Apply frame to animation offsets (0-100 → -90 to +90 degrees offset)
    float weight = track->fadeWeight;
    
    if (frame.head >= 0) {
      float offset = (frame.head - 50) * 1.8f;  // 0-100 → -90 to +90
      s_state.animationOffset[SERVO_HEAD_PAN] += offset * weight;
    }
    if (frame.neckTop >= 0) {
      float offset = (frame.neckTop - 50) * 1.8f;
      s_state.animationOffset[SERVO_NECK_TOP] += offset * weight;
    }
    if (frame.neckBottom >= 0) {
      float offset = (frame.neckBottom - 50) * 1.8f;
      s_state.animationOffset[SERVO_NECK_BOTTOM] += offset * weight;
    }
    if (frame.eyeRight >= 0) {
      float offset = (frame.eyeRight - 50) * 1.8f;
      s_state.animationOffset[SERVO_EYE_RIGHT] += offset * weight;
    }
    if (frame.eyeLeft >= 0) {
      float offset = (frame.eyeLeft - 50) * 1.8f;
      s_state.animationOffset[SERVO_EYE_LEFT] += offset * weight;
    }
    if (frame.armLeft >= 0) {
      float offset = (frame.armLeft - 50) * 1.8f;
      s_state.animationOffset[SERVO_LEFT_ARM] += offset * weight;
    }
    if (frame.armRight >= 0) {
      float offset = (frame.armRight - 50) * 1.8f;
      s_state.animationOffset[SERVO_RIGHT_ARM] += offset * weight;
    }
    // NEW: Eyebrow servos (use indices 8/9 or add new defines)
    if (frame.eyebrowRight >= 0) {
      float offset = (frame.eyebrowRight - 50) * 1.8f;
      s_state.animationOffset[8] += offset * weight;  // SERVO_EYEBROW_RIGHT
    }
    if (frame.eyebrowLeft >= 0) {
      float offset = (frame.eyebrowLeft - 50) * 1.8f;
      s_state.animationOffset[9] += offset * weight;  // SERVO_EYEBROW_LEFT
    }
  }
  
  // Blend: base + animation + joystick (per servo)
  for (int i = 0; i < SERVO_COUNT; i++) {
    float base = s_state.basePosition[i];
    float anim = s_state.animationOffset[i];
    float joy = s_state.joystickOffset[i];
    float joyInfluence = s_state.joystickInfluence[i];
    
    // Animation influence is muted by joystick
    float effectiveAnim = anim * (1.0f - joyInfluence);
    
    // Blend: base + animation + joystick
    float target = base + effectiveAnim + joy;
    
    // Clamp to servo limits
    target = constrain(target, SERVO_MIN, SERVO_MAX);
    s_state.targetPosition[i] = target;
    
    // Smooth to current (apply speed limit from profile)
    float smoothFactor = SERVO_SMOOTH_FACTOR * s_servoSpeedLimit;
    s_state.currentPosition[i] += (target - s_state.currentPosition[i]) * smoothFactor;
  }
}

void motionSetHeadPanVelocity(float vel) {
  // Velocity-based control: integrate velocity to position
  static unsigned long lastTime = millis();
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  lastTime = now;
  
  if (dt > 0.1f) dt = 0.02f;  // Cap
  
  // Apply profile sensitivity
  vel *= s_headSensitivity;
  
  // Integrate velocity
  s_headPanPos += vel * HEAD_VELOCITY_SCALE * dt;
  s_headPanPos = constrain(s_headPanPos, SERVO_MIN, SERVO_MAX);
  
  // Set as joystick offset (relative to base)
  float offset = s_headPanPos - s_state.basePosition[SERVO_HEAD_PAN];
  s_state.joystickOffset[SERVO_HEAD_PAN] = offset;
  
  // Set influence (1.0 if joystick active, fade out when centered)
  float influence = constrain(fabsf(vel) * 2.0f, 0.0f, 1.0f);
  s_state.joystickInfluence[SERVO_HEAD_PAN] = influence;
}

void motionSetHeadTiltVelocity(float vel) {
  static unsigned long lastTime = millis();
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  lastTime = now;
  
  if (dt > 0.1f) dt = 0.02f;
  
  // Apply profile sensitivity
  vel *= s_headSensitivity;
  
  s_headTiltPos += vel * HEAD_VELOCITY_SCALE * dt;
  s_headTiltPos = constrain(s_headTiltPos, SERVO_MIN, SERVO_MAX);
  
  float offset = s_headTiltPos - s_state.basePosition[SERVO_HEAD_TILT];
  s_state.joystickOffset[SERVO_HEAD_TILT] = offset;
  
  float influence = constrain(fabsf(vel) * 2.0f, 0.0f, 1.0f);
  s_state.joystickInfluence[SERVO_HEAD_TILT] = influence;
}

void motionSetJoystickOverride(uint8_t servoIndex, float offset, float influence) {
  if (servoIndex >= SERVO_COUNT) return;
  s_state.joystickOffset[servoIndex] = offset;
  s_state.joystickInfluence[servoIndex] = constrain(influence, 0.0f, 1.0f);
}

void motionGetServoTargets(uint8_t* targets) {
  if (!targets) return;
  for (int i = 0; i < SERVO_COUNT; i++) {
    targets[i] = (uint8_t)constrain(s_state.currentPosition[i], 0, 180);
  }
}

void motionTriggerAnimation(uint8_t animId) {
  if (animId >= ANIMATION_COUNT) {
    Serial.printf("[Motion] Invalid animation ID: %d\n", animId);
    return;
  }
  
  // Find next available track (or oldest track if all busy)
  int targetTrack = -1;
  for (int i = 0; i < MAX_ANIMATION_TRACKS; i++) {
    if (!s_tracks[i].active) {
      targetTrack = i;
      break;
    }
  }
  
  // If all tracks busy, use track 1 (keep track 0 as base)
  if (targetTrack == -1) {
    targetTrack = 1;
  }
  
  // Start animation on selected track
  s_tracks[targetTrack].active = true;
  s_tracks[targetTrack].startTime = millis();
  s_tracks[targetTrack].fadeWeight = 0.0f;
  s_tracks[targetTrack].animId = animId;
  s_tracks[targetTrack].currentFrame = 0;
  s_tracks[targetTrack].frameStartTime = millis();
  
  AnimationDef animDef;
  memcpy_P(&animDef, &animationLibrary[animId], sizeof(AnimationDef));
  char nameBuf[20];
  strcpy_P(nameBuf, animDef.name);
  
  Serial.printf("[Motion] Playing animation %d: %s (%d frames on track %d)\n", 
    animId, nameBuf, animDef.frameCount, targetTrack);
}

// Profile tuning setters
void motionSetHeadSensitivity(float sensitivity) {
  s_headSensitivity = constrain(sensitivity, 0.5f, 2.0f);
  Serial.printf("[Motion] Head sensitivity: %.2f\n", s_headSensitivity);
}

void motionSetServoSpeedLimit(float limit) {
  s_servoSpeedLimit = constrain(limit, 0.0f, 1.0f);
  Serial.printf("[Motion] Servo speed limit: %.2f\n", s_servoSpeedLimit);
}

void motionStopAllAnimations() {
  for (int i = 0; i < MAX_ANIMATION_TRACKS; i++) {
    s_tracks[i].active = false;
    s_tracks[i].fadeWeight = 0.0f;
  }
  for (int i = 0; i < SERVO_COUNT; i++) {
    s_state.animationOffset[i] = 0.0f;
  }
}

void motionEmergencyStop() {
  // Set all to neutral
  for (int i = 0; i < SERVO_COUNT; i++) {
    s_state.basePosition[i] = s_neutralPos[i];
    s_state.joystickOffset[i] = 0.0f;
    s_state.joystickInfluence[i] = 0.0f;
    s_state.targetPosition[i] = s_neutralPos[i];
    s_state.currentPosition[i] = s_neutralPos[i];
  }
  motionStopAllAnimations();
  Serial.println(F("[Motion] EMERGENCY STOP"));
}

void motionSetNeutralPositions(const uint8_t* neutral) {
  if (!neutral) return;
  for (int i = 0; i < SERVO_COUNT; i++) {
    s_neutralPos[i] = constrain(neutral[i], 0, 180);
    s_state.basePosition[i] = s_neutralPos[i];
  }
}

// ============================================================
//  Direct Servo Control (for servo test page)
// ============================================================

void motionSetServoDirect(uint8_t servoIndex, uint8_t degrees) {
  if (servoIndex >= SERVO_COUNT) return;
  
  degrees = constrain(degrees, 0, 180);
  s_state.basePosition[servoIndex] = degrees;
  s_state.targetPosition[servoIndex] = degrees;
  s_state.currentPosition[servoIndex] = degrees;
  
  // Clear any offsets for this servo
  s_state.animationOffset[servoIndex] = 0.0f;
  s_state.joystickOffset[servoIndex] = 0.0f;
  s_state.joystickInfluence[servoIndex] = 0.0f;
}

void motionSetAllNeutral() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    motionSetServoDirect(i, 90);
  }
  Serial.println(F("[Motion] All servos set to neutral (90°)"));
}

void motionTestPose1() {
  // Test pose: Head left, arms out (9 servos)
  motionSetServoDirect(SERVO_HEAD_PAN, 45);
  motionSetServoDirect(SERVO_HEAD_TILT, 90);
  motionSetServoDirect(SERVO_EYE_LEFT, 90);
  motionSetServoDirect(SERVO_EYE_RIGHT, 90);
  motionSetServoDirect(SERVO_NECK_TOP, 90);
  motionSetServoDirect(SERVO_NECK_BOTTOM, 90);
  motionSetServoDirect(SERVO_LEFT_ARM, 45);
  motionSetServoDirect(SERVO_RIGHT_ARM, 135);
  motionSetServoDirect(SERVO_EYEBROW_RIGHT, 90);
  Serial.println(F("[Motion] Test pose 1 applied"));
}

void motionTestPose2() {
  // Test pose: Head right, arms up (9 servos)
  motionSetServoDirect(SERVO_HEAD_PAN, 135);
  motionSetServoDirect(SERVO_HEAD_TILT, 90);
  motionSetServoDirect(SERVO_EYE_LEFT, 45);
  motionSetServoDirect(SERVO_EYE_RIGHT, 135);
  motionSetServoDirect(SERVO_NECK_TOP, 90);
  motionSetServoDirect(SERVO_NECK_BOTTOM, 90);
  motionSetServoDirect(SERVO_LEFT_ARM, 135);
  motionSetServoDirect(SERVO_RIGHT_ARM, 45);
  motionSetServoDirect(SERVO_EYEBROW_RIGHT, 135);
  Serial.println(F("[Motion] Test pose 2 applied"));
}
