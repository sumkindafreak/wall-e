#pragma once

// ============================================================
//  WALL-E Emotion Engine  
//  Defines HOW Wall-E feels - dynamic emotional state
// ============================================================

#include <Arduino.h>

// ============================================================
//  Emotion States
// ============================================================

enum Emotion {
  EMOTION_CALM,       // Neutral, content, idle
  EMOTION_CURIOUS,    // Interested, investigating
  EMOTION_EXCITED,    // High energy, discovered something
  EMOTION_NERVOUS,    // Cautious, uncertain, obstacle detected
  EMOTION_LOW_POWER,  // Tired, need to return home
  EMOTION_COUNT
};

// ============================================================
//  Emotion Modifiers (applied to animations and behavior)
// ============================================================

struct EmotionState {
  Emotion current;
  float intensity;        // 0.0 (mild) - 1.0 (intense)
  uint32_t stateStartTime;
  uint32_t minStateDuration;  // Minimum time before emotion can change
};

// ============================================================
//  API
// ============================================================

void emotionInit();
void emotionUpdate(uint32_t now, float batteryPercent, bool objectDetected, float interestLevel);

// Getters
Emotion emotionGetCurrent();
float emotionGetIntensity();
const char* emotionGetName();
const char* emotionGetName(Emotion e);

// State transitions
void emotionSetState(Emotion newEmotion, float intensity);
void emotionTransitionTo(Emotion newEmotion, float intensity, uint32_t minDuration);

// Behavioral modifiers (used by motion and autonomy engines)
float emotionGetAnimationAmplitude();   // 0.5x - 1.5x animation scale
float emotionGetHeadSpeed();            // 0.5x - 2.0x head movement speed
float emotionGetIdleIntensity();        // 0.3x - 1.0x idle animation amplitude
float emotionGetMovementSpeed();        // 0.4x - 1.2x track speed

// Triggers (called by autonomy engine)
void emotionTriggerDiscovery();         // Object detected
void emotionTriggerInvestigateStart();  // Starting investigation
void emotionTriggerInvestigateEnd();    // Finished investigation
void emotionTriggerObstacleNear();      // Close obstacle
void emotionTriggerBatteryLow();        // Low battery warning
void emotionTriggerReturnHome();        // Returning to base
void emotionTriggerIdle();              // Nothing interesting

// Debug
void emotionPrintDebug();
uint32_t emotionGetTimeSinceChange();
