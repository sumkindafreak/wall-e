// ============================================================
//  WALL-E Emotion Engine Implementation
//  Dynamic emotional state that affects behavior
// ============================================================

#include "emotion_engine.h"

// ============================================================
//  Internal State
// ============================================================

static EmotionState s_emotion = {
  .current = EMOTION_CALM,
  .intensity = 0.5f,
  .stateStartTime = 0,
  .minStateDuration = 2000  // At least 2 seconds per emotion
};

static const char* EMOTION_NAMES[] = {
  "Calm",
  "Curious",
  "Excited",
  "Nervous",
  "LowPower"
};

// Rate limiting for debug prints
static uint32_t s_lastDebugPrint = 0;
static const uint32_t DEBUG_INTERVAL = 10000; // 10s

// ============================================================
//  Initialization
// ============================================================

void emotionInit() {
  Serial.println("[Emotion] Initializing...");
  s_emotion.current = EMOTION_CALM;
  s_emotion.intensity = 0.5f;
  s_emotion.stateStartTime = millis();
  Serial.println("[Emotion] Starting in CALM state");
}

// ============================================================
//  Update (called every loop)
// ============================================================

void emotionUpdate(uint32_t now, float batteryPercent, bool objectDetected, float interestLevel) {
  // Auto-transition logic based on context
  
  // Priority 1: Low battery
  if (batteryPercent < 20.0f && s_emotion.current != EMOTION_LOW_POWER) {
    float intensity = 1.0f - (batteryPercent / 20.0f); // More intense as battery drops
    emotionTransitionTo(EMOTION_LOW_POWER, intensity, 5000);
    return;
  }
  
  // Priority 2: High interest + object detected = Excited
  if (interestLevel > 80.0f && objectDetected && s_emotion.current != EMOTION_EXCITED) {
    emotionTransitionTo(EMOTION_EXCITED, 0.8f, 3000);
    return;
  }
  
  // Priority 3: Moderate interest + object = Curious
  if (interestLevel > 30.0f && objectDetected && s_emotion.current == EMOTION_CALM) {
    emotionTransitionTo(EMOTION_CURIOUS, 0.6f, 2000);
    return;
  }
  
  // Priority 4: Object too close = Nervous
  // (This will be triggered by sonar in autonomy engine via emotionTriggerObstacleNear)
  
  // Gradual return to calm after minimum duration
  uint32_t timeSinceChange = now - s_emotion.stateStartTime;
  if (timeSinceChange > s_emotion.minStateDuration) {
    // If no longer interested and not in special state, return to calm
    if (interestLevel < 10.0f && !objectDetected && 
        s_emotion.current != EMOTION_CALM && 
        s_emotion.current != EMOTION_LOW_POWER) {
      emotionTransitionTo(EMOTION_CALM, 0.5f, 5000);
    }
  }
  
  // Debug print (rate limited)
  if (now - s_lastDebugPrint > DEBUG_INTERVAL) {
    emotionPrintDebug();
    s_lastDebugPrint = now;
  }
}

// ============================================================
//  Getters
// ============================================================

Emotion emotionGetCurrent() {
  return s_emotion.current;
}

float emotionGetIntensity() {
  return s_emotion.intensity;
}

const char* emotionGetName() {
  return EMOTION_NAMES[s_emotion.current];
}

const char* emotionGetName(Emotion e) {
  if (e < EMOTION_COUNT) {
    return EMOTION_NAMES[e];
  }
  return "Unknown";
}

// ============================================================
//  State Transitions
// ============================================================

void emotionSetState(Emotion newEmotion, float intensity) {
  if (newEmotion != s_emotion.current) {
    Serial.printf("[Emotion] %s -> %s (intensity: %.2f)\n", 
                  EMOTION_NAMES[s_emotion.current],
                  EMOTION_NAMES[newEmotion],
                  intensity);
  }
  s_emotion.current = newEmotion;
  s_emotion.intensity = constrain(intensity, 0.0f, 1.0f);
  s_emotion.stateStartTime = millis();
}

void emotionTransitionTo(Emotion newEmotion, float intensity, uint32_t minDuration) {
  // Only transition if enough time has passed since last change
  uint32_t now = millis();
  if (now - s_emotion.stateStartTime >= s_emotion.minStateDuration || 
      newEmotion == EMOTION_LOW_POWER) {  // Low power can interrupt anything
    s_emotion.minStateDuration = minDuration;
    emotionSetState(newEmotion, intensity);
  }
}

// ============================================================
//  Behavioral Modifiers
// ============================================================

float emotionGetAnimationAmplitude() {
  // Emotion affects animation scale
  switch (s_emotion.current) {
    case EMOTION_EXCITED:
      return 1.0f + (s_emotion.intensity * 0.5f);  // 1.0 - 1.5x
    case EMOTION_CURIOUS:
      return 1.0f + (s_emotion.intensity * 0.2f);  // 1.0 - 1.2x
    case EMOTION_NERVOUS:
      return 0.7f + (s_emotion.intensity * 0.3f);  // 0.7 - 1.0x (jittery)
    case EMOTION_LOW_POWER:
      return 0.5f - (s_emotion.intensity * 0.2f);  // 0.5 - 0.3x (weak)
    case EMOTION_CALM:
    default:
      return 1.0f;
  }
}

float emotionGetHeadSpeed() {
  // Emotion affects head movement speed
  switch (s_emotion.current) {
    case EMOTION_EXCITED:
      return 1.5f + (s_emotion.intensity * 0.5f);  // 1.5 - 2.0x (quick)
    case EMOTION_CURIOUS:
      return 1.0f + (s_emotion.intensity * 0.3f);  // 1.0 - 1.3x (attentive)
    case EMOTION_NERVOUS:
      return 1.2f + (s_emotion.intensity * 0.3f);  // 1.2 - 1.5x (twitchy)
    case EMOTION_LOW_POWER:
      return 0.5f - (s_emotion.intensity * 0.3f);  // 0.5 - 0.2x (slow)
    case EMOTION_CALM:
    default:
      return 1.0f;
  }
}

float emotionGetIdleIntensity() {
  // Emotion affects idle animation amplitude
  switch (s_emotion.current) {
    case EMOTION_EXCITED:
      return 1.0f;  // Full idle animations
    case EMOTION_CURIOUS:
      return 0.7f;  // Moderate idle (focused)
    case EMOTION_NERVOUS:
      return 0.5f;  // Reduced idle (alert)
    case EMOTION_LOW_POWER:
      return 0.3f;  // Minimal idle (conserve energy)
    case EMOTION_CALM:
    default:
      return 0.8f;
  }
}

float emotionGetMovementSpeed() {
  // Emotion affects track movement speed
  switch (s_emotion.current) {
    case EMOTION_EXCITED:
      return 1.0f + (s_emotion.intensity * 0.2f);  // 1.0 - 1.2x (eager)
    case EMOTION_CURIOUS:
      return 0.8f + (s_emotion.intensity * 0.2f);  // 0.8 - 1.0x (careful)
    case EMOTION_NERVOUS:
      return 0.6f + (s_emotion.intensity * 0.2f);  // 0.6 - 0.8x (cautious)
    case EMOTION_LOW_POWER:
      return 0.4f - (s_emotion.intensity * 0.2f);  // 0.4 - 0.2x (conserve)
    case EMOTION_CALM:
    default:
      return 0.7f;  // Moderate exploration speed
  }
}

// ============================================================
//  Triggers (called by autonomy engine)
// ============================================================

void emotionTriggerDiscovery() {
  emotionTransitionTo(EMOTION_CURIOUS, 0.7f, 3000);
}

void emotionTriggerInvestigateStart() {
  emotionTransitionTo(EMOTION_CURIOUS, 0.9f, 2000);
}

void emotionTriggerInvestigateEnd() {
  // Return to calm after investigation
  emotionTransitionTo(EMOTION_CALM, 0.5f, 5000);
}

void emotionTriggerObstacleNear() {
  emotionTransitionTo(EMOTION_NERVOUS, 0.8f, 2000);
}

void emotionTriggerBatteryLow() {
  emotionTransitionTo(EMOTION_LOW_POWER, 0.9f, 10000);
}

void emotionTriggerReturnHome() {
  emotionTransitionTo(EMOTION_LOW_POWER, 0.6f, 5000);
}

void emotionTriggerIdle() {
  if (s_emotion.current != EMOTION_CALM && s_emotion.current != EMOTION_LOW_POWER) {
    emotionTransitionTo(EMOTION_CALM, 0.5f, 5000);
  }
}

// ============================================================
//  Debug
// ============================================================

void emotionPrintDebug() {
  Serial.printf("[Emotion] State: %s (intensity: %.2f, duration: %lums)\n",
                EMOTION_NAMES[s_emotion.current],
                s_emotion.intensity,
                millis() - s_emotion.stateStartTime);
  Serial.printf("  Modifiers: Anim=%.2fx Speed=%.2fx Head=%.2fx Idle=%.2fx\n",
                emotionGetAnimationAmplitude(),
                emotionGetMovementSpeed(),
                emotionGetHeadSpeed(),
                emotionGetIdleIntensity());
}

uint32_t emotionGetTimeSinceChange() {
  return millis() - s_emotion.stateStartTime;
}
