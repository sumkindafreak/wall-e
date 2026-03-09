#pragma once

// ============================================================
//  WALL-E Personality Engine
//  Defines WHO Wall-E is - behavioral traits that persist
// ============================================================

#include <Arduino.h>
#include <Preferences.h>

// ============================================================
//  Personality Traits (0.0 - 1.0 scale)
// ============================================================

struct Personality {
  float curiosity;    // 0.0 (apathetic) - 1.0 (extremely curious)
  float bravery;      // 0.0 (cautious) - 1.0 (fearless)
  float energy;       // 0.0 (lethargic) - 1.0 (hyperactive)
  float randomness;   // 0.0 (predictable) - 1.0 (chaotic)
};

// ============================================================
//  Personality Presets
// ============================================================

enum PersonalityPreset {
  PRESET_CAUTIOUS,     // Low curiosity, high caution, low energy
  PRESET_BALANCED,     // Moderate all traits
  PRESET_ADVENTUROUS,  // High curiosity, moderate bravery, high energy
  PRESET_CHAOTIC,      // High randomness, high energy, moderate curiosity
  PRESET_CUSTOM        // User-defined
};

// ============================================================
//  API
// ============================================================

void personalityInit();
void personalityUpdate(uint32_t now);

// Getters
const Personality* personalityGet();
float personalityGetCuriosity();
float personalityGetBravery();
float personalityGetEnergy();
float personalityGetRandomness();

// Setters (for WebUI control)
void personalitySetCuriosity(float value);
void personalitySetBravery(float value);
void personalitySetEnergy(float value);
void personalitySetRandomness(float value);

// Presets
void personalityLoadPreset(PersonalityPreset preset);
void personalitySave();   // Save to Preferences
void personalityLoad();   // Load from Preferences

// Behavioral modifiers (used by other engines)
float personalityGetInterestRate();      // How fast interest accumulates
float personalityGetApproachDistance();  // How close to approach objects
float personalityGetExplorationSpeed();  // Movement speed multiplier
float personalityGetPauseFrequency();    // How often to pause and think
float personalityGetReactionIntensity(); // How dramatic reactions are

// Debug
void personalityPrintDebug();
