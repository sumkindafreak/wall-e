// ============================================================
//  WALL-E Personality Engine Implementation
//  Behavioral traits that define Wall-E's character
// ============================================================

#include "personality_engine.h"

// ============================================================
//  Internal State
// ============================================================

static Personality s_personality = {
  .curiosity = 0.7f,   // Default: quite curious
  .bravery = 0.5f,     // Default: moderately brave
  .energy = 0.6f,      // Default: moderate energy
  .randomness = 0.4f   // Default: somewhat predictable
};

static PersonalityPreset s_currentPreset = PRESET_BALANCED;
static Preferences s_prefs;

// ============================================================
//  Preset Definitions
// ============================================================

static const Personality PRESETS[] = {
  // CAUTIOUS - shy, careful, low energy
  { .curiosity = 0.3f, .bravery = 0.2f, .energy = 0.3f, .randomness = 0.2f },
  
  // BALANCED - moderate everything
  { .curiosity = 0.5f, .bravery = 0.5f, .energy = 0.5f, .randomness = 0.5f },
  
  // ADVENTUROUS - high curiosity and energy
  { .curiosity = 0.9f, .bravery = 0.7f, .energy = 0.8f, .randomness = 0.6f },
  
  // CHAOTIC - unpredictable and energetic
  { .curiosity = 0.6f, .bravery = 0.6f, .energy = 0.9f, .randomness = 0.9f }
};

// ============================================================
//  Initialization
// ============================================================

void personalityInit() {
  Serial.println("[Personality] Initializing...");
  
  // Try to load saved personality
  s_prefs.begin("personality", false);
  if (s_prefs.isKey("preset")) {
    s_currentPreset = (PersonalityPreset)s_prefs.getUChar("preset", PRESET_BALANCED);
    
    if (s_currentPreset == PRESET_CUSTOM) {
      // Load custom values
      s_personality.curiosity = s_prefs.getFloat("curiosity", 0.5f);
      s_personality.bravery = s_prefs.getFloat("bravery", 0.5f);
      s_personality.energy = s_prefs.getFloat("energy", 0.5f);
      s_personality.randomness = s_prefs.getFloat("randomness", 0.5f);
      Serial.println("[Personality] Loaded custom personality");
    } else {
      // Load preset
      s_personality = PRESETS[s_currentPreset];
      Serial.printf("[Personality] Loaded preset: %d\n", s_currentPreset);
    }
  } else {
    // First boot - use balanced preset
    personalityLoadPreset(PRESET_BALANCED);
    Serial.println("[Personality] First boot - using BALANCED preset");
  }
  
  s_prefs.end();
  personalityPrintDebug();
}

void personalityUpdate(uint32_t now) {
  // Personality doesn't change dynamically, but we can add
  // learning/adaptation here in the future
  // For now, this is a placeholder for future expansion
}

// ============================================================
//  Getters
// ============================================================

const Personality* personalityGet() {
  return &s_personality;
}

float personalityGetCuriosity() {
  return s_personality.curiosity;
}

float personalityGetBravery() {
  return s_personality.bravery;
}

float personalityGetEnergy() {
  return s_personality.energy;
}

float personalityGetRandomness() {
  return s_personality.randomness;
}

// ============================================================
//  Setters
// ============================================================

void personalitySetCuriosity(float value) {
  s_personality.curiosity = constrain(value, 0.0f, 1.0f);
  s_currentPreset = PRESET_CUSTOM;
  Serial.printf("[Personality] Curiosity set to %.2f\n", s_personality.curiosity);
}

void personalitySetBravery(float value) {
  s_personality.bravery = constrain(value, 0.0f, 1.0f);
  s_currentPreset = PRESET_CUSTOM;
  Serial.printf("[Personality] Bravery set to %.2f\n", s_personality.bravery);
}

void personalitySetEnergy(float value) {
  s_personality.energy = constrain(value, 0.0f, 1.0f);
  s_currentPreset = PRESET_CUSTOM;
  Serial.printf("[Personality] Energy set to %.2f\n", s_personality.energy);
}

void personalitySetRandomness(float value) {
  s_personality.randomness = constrain(value, 0.0f, 1.0f);
  s_currentPreset = PRESET_CUSTOM;
  Serial.printf("[Personality] Randomness set to %.2f\n", s_personality.randomness);
}

// ============================================================
//  Presets
// ============================================================

void personalityLoadPreset(PersonalityPreset preset) {
  if (preset >= PRESET_CAUTIOUS && preset <= PRESET_CHAOTIC) {
    s_personality = PRESETS[preset];
    s_currentPreset = preset;
    Serial.printf("[Personality] Loaded preset: %d\n", preset);
    personalityPrintDebug();
  }
}

void personalitySave() {
  s_prefs.begin("personality", false);
  s_prefs.putUChar("preset", s_currentPreset);
  
  if (s_currentPreset == PRESET_CUSTOM) {
    s_prefs.putFloat("curiosity", s_personality.curiosity);
    s_prefs.putFloat("bravery", s_personality.bravery);
    s_prefs.putFloat("energy", s_personality.energy);
    s_prefs.putFloat("randomness", s_personality.randomness);
  }
  
  s_prefs.end();
  Serial.println("[Personality] Saved to flash");
}

void personalityLoad() {
  personalityInit();  // Reload from Preferences
}

// ============================================================
//  Behavioral Modifiers
// ============================================================

float personalityGetInterestRate() {
  // Curiosity directly affects how fast interest accumulates
  // Range: 0.1 to 1.0 per second
  return 0.1f + (s_personality.curiosity * 0.9f);
}

float personalityGetApproachDistance() {
  // Bravery affects how close to approach objects
  // Range: 20cm (cautious) to 50cm (brave)
  return 20.0f + (s_personality.bravery * 30.0f);
}

float personalityGetExplorationSpeed() {
  // Energy affects movement speed
  // Range: 0.3x to 1.0x of max speed
  return 0.3f + (s_personality.energy * 0.7f);
}

float personalityGetPauseFrequency() {
  // Randomness affects how often Wall-E pauses to "think"
  // Range: every 30s (predictable) to every 5s (chaotic)
  return 30.0f - (s_personality.randomness * 25.0f);
}

float personalityGetReactionIntensity() {
  // Combined traits affect reaction drama
  // High energy + high randomness = dramatic reactions
  float intensity = (s_personality.energy * 0.5f) + (s_personality.randomness * 0.5f);
  return constrain(intensity, 0.2f, 1.0f);
}

// ============================================================
//  Debug
// ============================================================

void personalityPrintDebug() {
  Serial.println("[Personality] Current traits:");
  Serial.printf("  Curiosity:  %.2f\n", s_personality.curiosity);
  Serial.printf("  Bravery:    %.2f\n", s_personality.bravery);
  Serial.printf("  Energy:     %.2f\n", s_personality.energy);
  Serial.printf("  Randomness: %.2f\n", s_personality.randomness);
  Serial.printf("  Preset:     %d\n", s_currentPreset);
  Serial.println("[Personality] Derived behaviors:");
  Serial.printf("  Interest rate:    %.2f/s\n", personalityGetInterestRate());
  Serial.printf("  Approach dist:    %.1fcm\n", personalityGetApproachDistance());
  Serial.printf("  Speed mult:       %.2fx\n", personalityGetExplorationSpeed());
  Serial.printf("  Pause freq:       %.1fs\n", personalityGetPauseFrequency());
  Serial.printf("  Reaction intens:  %.2f\n", personalityGetReactionIntensity());
}
