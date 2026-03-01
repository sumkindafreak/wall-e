// ============================================================
//  WALL-E Master Controller — Profile System Implementation
// ============================================================

#include "profiles.h"
#include "ads1115_input.h"
#include "motion_engine.h"
#include "audio_system.h"
#include <Arduino.h>

// Current profile
uint8_t g_currentProfile = PROFILE_DEMO;

// Preferences storage
static Preferences prefs;

// Profile definitions
Profile profiles[PROFILE_COUNT] = {
  // ============================================================
  // PROFILE 0: KID MODE
  // Safe, slow, limited animations
  // ============================================================
  {
    .name = "Kid",
    .joystickDeadzone = 0.15f,     // Large deadzone
    .joystickExpo = 0.5f,          // Heavy expo (gentle)
    .joystickMaxSpeed = 0.5f,      // 50% max speed
    .headSensitivity = 0.5f,       // Slow head movements
    .headEnabled = true,
    .servoSpeedLimit = 0.6f,       // Slow servo movements
    .allowAllAnimations = false,   // Restricted
    .allowedAnimations = {0x07, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // Only 0,1,2
    .buttonAction = {
      BTN_ACTION_ANIMATION,    // Button 2 → Bootup
      BTN_ACTION_ANIMATION,    // Button 3 → Inquisitive
      BTN_ACTION_SOUND,        // Button 4 → Happy sound
      BTN_ACTION_NONE,
      BTN_ACTION_NONE,
      BTN_ACTION_ESTOP         // Button 7 → E-STOP
    },
    .buttonParam = {1, 2, 1, 0, 0, 0},  // Animation IDs / sound IDs
    .requireDeadman = true,
    .autoStopTimeout = 3000,   // 3 second timeout
    .neutralPositions = {90, 90, 90, 90, 90, 90, 90, 90, 90, 90},  // All servos at 90° (center)
    .favoriteAnimations = {2, 1, 5, 3, 4},  // Default: Inquisitive, Bootup, Surprised, BrowR, BrowL
    .autonomyCuriosity = 0.5f,
    .autonomyBravery = 0.3f,
    .autonomyEnergy = 0.4f,
    .autonomyRandomness = 0.3f,
    .autonomyEnabled = false     // Disabled for kid safety
  },
  
  // ============================================================
  // PROFILE 1: DEMO MODE
  // Balanced for demonstrations
  // ============================================================
  {
    .name = "Demo",
    .joystickDeadzone = 0.08f,     // Normal deadzone
    .joystickExpo = 0.2f,          // Light expo
    .joystickMaxSpeed = 0.8f,      // 80% max speed
    .headSensitivity = 1.0f,       // Normal head movements
    .headEnabled = true,
    .servoSpeedLimit = 1.0f,       // Full speed servos
    .allowAllAnimations = true,    // All animations allowed
    .allowedAnimations = {0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    .buttonAction = {
      BTN_ACTION_ANIMATION,    // Button 2 → Bootup
      BTN_ACTION_ANIMATION,    // Button 3 → Inquisitive  
      BTN_ACTION_ANIMATION,    // Button 4 → Surprised
      BTN_ACTION_SOUND,        // Button 5 → Sound effect
      BTN_ACTION_PROFILE_CYCLE,// Button 6 → Cycle profiles
      BTN_ACTION_NONE
    },
    .buttonParam = {1, 2, 5, 2, 0, 0},
    .requireDeadman = true,
    .autoStopTimeout = 5000,   // 5 second timeout
    .neutralPositions = {90, 90, 90, 90, 90, 90, 90, 90, 90, 90},
    .favoriteAnimations = {2, 1, 5, 3, 4},  // Default: Inquisitive, Bootup, Surprised, BrowR, BrowL
    .autonomyCuriosity = 0.7f,
    .autonomyBravery = 0.5f,
    .autonomyEnergy = 0.6f,
    .autonomyRandomness = 0.5f,
    .autonomyEnabled = true      // Enabled for demo
  },
  
  // ============================================================
  // PROFILE 2: ADVANCED MODE
  // Full control, no limits
  // ============================================================
  {
    .name = "Advanced",
    .joystickDeadzone = 0.03f,     // Minimal deadzone
    .joystickExpo = 0.0f,          // No expo (linear)
    .joystickMaxSpeed = 1.0f,      // 100% max speed
    .headSensitivity = 1.5f,       // Fast head movements
    .headEnabled = true,
    .servoSpeedLimit = 1.0f,       // Full speed servos
    .allowAllAnimations = true,    // All animations allowed
    .allowedAnimations = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    .buttonAction = {
      BTN_ACTION_ANIMATION,    // Button 2 → Bootup
      BTN_ACTION_ANIMATION,    // Button 3 → Inquisitive
      BTN_ACTION_ANIMATION,    // Button 4 → Surprised
      BTN_ACTION_ANIMATION,    // Button 5 → Reset
      BTN_ACTION_PROFILE_CYCLE,// Button 6 → Cycle profiles
      BTN_ACTION_NONE
    },
    .buttonParam = {1, 2, 5, 0, 0, 0},
    .requireDeadman = true,
    .autoStopTimeout = 0,      // No timeout
    .neutralPositions = {90, 90, 90, 90, 90, 90, 90, 90, 90, 90},
    .favoriteAnimations = {2, 1, 5, 3, 4},  // Default: Inquisitive, Bootup, Surprised, BrowR, BrowL
    .autonomyCuriosity = 0.9f,
    .autonomyBravery = 0.7f,
    .autonomyEnergy = 0.8f,
    .autonomyRandomness = 0.7f,
    .autonomyEnabled = true      // Full autonomy for advanced
  }
};

void profileInit() {
  prefs.begin("wall-e", false);
  
  // Load saved profile
  g_currentProfile = prefs.getUChar("profile", PROFILE_DEMO);
  if (g_currentProfile >= PROFILE_COUNT) {
    g_currentProfile = PROFILE_DEMO;
  }
  
  Serial.printf("[Profile] Loaded profile: %d (%s)\n", 
    g_currentProfile, profiles[g_currentProfile].name);
  
  profileApply();
}

Profile* profileGet() {
  return &profiles[g_currentProfile];
}

void profileSet(uint8_t profileId) {
  if (profileId >= PROFILE_COUNT) return;
  
  g_currentProfile = profileId;
  prefs.putUChar("profile", profileId);
  
  Serial.printf("[Profile] Switched to: %s\n", profiles[profileId].name);
  
  profileApply();
  playUISound(SOUND_MODE_CHANGE);
}

void profileCycleNext() {
  uint8_t next = (g_currentProfile + 1) % PROFILE_COUNT;
  profileSet(next);
}

void profileApply() {
  Profile* p = profileGet();
  
  // Apply joystick tuning
  setJoystickDeadzone(p->joystickDeadzone);
  setJoystickExpo(p->joystickExpo);
  setJoystickMaxOutput(p->joystickMaxSpeed);
  
  // Apply to motion engine
  motionSetHeadSensitivity(p->headSensitivity);
  motionSetServoSpeedLimit(p->servoSpeedLimit);
  
  // Load neutral positions
  profileLoadNeutralPositions();
  profileLoadFavoriteAnimations();
  
  Serial.printf("[Profile] Applied: %s\n", p->name);
  Serial.printf("  Deadzone: %.2f, Expo: %.2f, MaxSpeed: %.2f\n",
    p->joystickDeadzone, p->joystickExpo, p->joystickMaxSpeed);
  Serial.printf("  HeadSens: %.2f, ServoLimit: %.2f\n",
    p->headSensitivity, p->servoSpeedLimit);
}

void profileSave() {
  prefs.putUChar("profile", g_currentProfile);
  Serial.println(F("[Profile] Saved to flash"));
}

void profileResetDefaults() {
  prefs.clear();
  g_currentProfile = PROFILE_DEMO;
  profileApply();
  Serial.println(F("[Profile] Reset to defaults"));
}

void profileHandleButtonAction(uint8_t buttonIndex) {
  if (buttonIndex >= 6) return;  // Only handle extra buttons 2-7
  
  Profile* p = profileGet();
  uint8_t action = p->buttonAction[buttonIndex];
  uint8_t param = p->buttonParam[buttonIndex];
  
  switch (action) {
    case BTN_ACTION_NONE:
      break;
      
    case BTN_ACTION_ANIMATION:
      // Check if animation is allowed
      if (!p->allowAllAnimations) {
        uint8_t byte = param / 8;
        uint8_t bit = param % 8;
        if (!(p->allowedAnimations[byte] & (1 << bit))) {
          Serial.printf("[Profile] Animation %d not allowed in %s mode\n", 
            param, p->name);
          playUISound(SOUND_ERROR);
          return;
        }
      }
      motionTriggerAnimation(param);
      Serial.printf("[Profile] Button %d → Animation %d\n", buttonIndex, param);
      break;
      
    case BTN_ACTION_ESTOP:
      Serial.println(F("[Profile] Button → E-STOP"));
      // E-STOP is handled in main loop
      break;
      
    case BTN_ACTION_PROFILE_CYCLE:
      profileCycleNext();
      break;
      
    case BTN_ACTION_SOUND:
      playUISound((UISound)param);
      Serial.printf("[Profile] Button %d → Sound %d\n", buttonIndex, param);
      break;
      
    case BTN_ACTION_CUSTOM:
      Serial.printf("[Profile] Button %d → Custom action %d\n", buttonIndex, param);
      break;
      
    default:
      Serial.printf("[Profile] Unknown action: %d\n", action);
      break;
  }
}

// ============================================================
//  Servo Editor Value Adjustment
// ============================================================

void profileAdjustHeadSensitivity(float delta) {
  Profile* p = &profiles[g_currentProfile];
  p->headSensitivity = constrain(p->headSensitivity + delta, 0.5f, 2.0f);
  motionSetHeadSensitivity(p->headSensitivity);
}

void profileAdjustServoSpeed(float delta) {
  Profile* p = &profiles[g_currentProfile];
  p->servoSpeedLimit = constrain(p->servoSpeedLimit + delta, 0.0f, 1.0f);
  motionSetServoSpeedLimit(p->servoSpeedLimit);
}

void profileAdjustDeadzone(float delta) {
  Profile* p = &profiles[g_currentProfile];
  p->joystickDeadzone = constrain(p->joystickDeadzone + delta, 0.0f, 0.5f);
  setJoystickDeadzone(p->joystickDeadzone);
}

void profileAdjustExpo(float delta) {
  Profile* p = &profiles[g_currentProfile];
  p->joystickExpo = constrain(p->joystickExpo + delta, 0.0f, 1.0f);
  setJoystickExpo(p->joystickExpo);
}

void profileAdjustMaxSpeed(float delta) {
  Profile* p = &profiles[g_currentProfile];
  p->joystickMaxSpeed = constrain(p->joystickMaxSpeed + delta, 0.0f, 1.0f);
  setJoystickMaxOutput(p->joystickMaxSpeed);
}

// ============================================================
//  Servo Neutral Position Management
// ============================================================

void profileSaveNeutralPositions(const uint8_t* positions) {
  if (!positions) return;
  
  Profile* p = &profiles[g_currentProfile];
  
  // Copy positions to current profile
  for (int i = 0; i < 10; i++) {
    p->neutralPositions[i] = constrain(positions[i], 0, 180);
  }
  
  // Save to flash (store as blob)
  char key[16];
  snprintf(key, sizeof(key), "neutral_%d", g_currentProfile);
  prefs.putBytes(key, p->neutralPositions, 10);
  
  Serial.printf("[Profile] Saved neutral positions for %s profile\n", p->name);
}

void profileLoadNeutralPositions() {
  Profile* p = &profiles[g_currentProfile];
  
  // Try to load from flash
  char key[16];
  snprintf(key, sizeof(key), "neutral_%d", g_currentProfile);
  size_t len = prefs.getBytes(key, p->neutralPositions, 10);
  
  if (len == 10) {
    // Successfully loaded - apply to motion engine
    motionSetNeutralPositions(p->neutralPositions);
    Serial.printf("[Profile] Loaded neutral positions for %s\n", p->name);
  } else {
    // No saved positions - use defaults (already in struct)
    motionSetNeutralPositions(p->neutralPositions);
    Serial.printf("[Profile] Using default neutral for %s\n", p->name);
  }
}

// ============================================================
//  Favorite Animation Management
// ============================================================

void profileToggleFavoriteAnimation(uint8_t animId) {
  Profile* p = profileGet();
  
  // Check if animation is already in favorites
  int existingIndex = -1;
  for (int i = 0; i < 5; i++) {
    if (p->favoriteAnimations[i] == animId) {
      existingIndex = i;
      break;
    }
  }
  
  if (existingIndex >= 0) {
    // Remove from favorites - shift remaining animations
    for (int i = existingIndex; i < 4; i++) {
      p->favoriteAnimations[i] = p->favoriteAnimations[i + 1];
    }
    p->favoriteAnimations[4] = 0;  // Clear last slot
    Serial.printf("[Profile] Removed animation %d from favorites\n", animId);
  } else {
    // Add to first empty slot (or replace last if full)
    for (int i = 0; i < 5; i++) {
      if (p->favoriteAnimations[i] == 0 || i == 4) {
        p->favoriteAnimations[i] = animId;
        Serial.printf("[Profile] Added animation %d to favorites at slot %d\n", animId, i);
        break;
      }
    }
  }
  
  // Save to Preferences
  char key[20];
  snprintf(key, sizeof(key), "favs_%d", g_currentProfile);
  prefs.putBytes(key, p->favoriteAnimations, 5);
}

void profileLoadFavoriteAnimations() {
  Profile* p = profileGet();
  
  // Try to load from Preferences
  char key[20];
  snprintf(key, sizeof(key), "favs_%d", g_currentProfile);
  
  uint8_t loaded[5];
  size_t len = prefs.getBytes(key, loaded, 5);
  
  if (len == 5) {
    // Loaded successfully
    for (int i = 0; i < 5; i++) {
      p->favoriteAnimations[i] = loaded[i];
    }
    Serial.printf("[Profile] Loaded favorite animations from Preferences\n");
  } else {
    Serial.printf("[Profile] Using default favorite animations\n");
  }
}

