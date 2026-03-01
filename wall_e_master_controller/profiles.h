// ============================================================
//  WALL-E Master Controller — Profile System
//  Kid/Demo/Advanced modes with different tuning + permissions
// ============================================================

#ifndef PROFILES_H
#define PROFILES_H

#include <Arduino.h>
#include <Preferences.h>

// Profile IDs
#define PROFILE_KID       0
#define PROFILE_DEMO      1
#define PROFILE_ADVANCED  2
#define PROFILE_COUNT     3

// Button action types
#define BTN_ACTION_NONE           0
#define BTN_ACTION_ANIMATION      1  // Trigger animation ID
#define BTN_ACTION_ESTOP          2
#define BTN_ACTION_PROFILE_CYCLE  3
#define BTN_ACTION_SOUND          4
#define BTN_ACTION_CUSTOM         5

// Profile structure
struct Profile {
  const char* name;
  
  // Joystick tuning
  float joystickDeadzone;      // 0.0 to 0.5
  float joystickExpo;          // 0.0 to 1.0
  float joystickMaxSpeed;      // 0.0 to 1.0
  
  // Head control tuning
  float headSensitivity;       // 0.5 to 2.0 multiplier
  bool headEnabled;            // Allow head control
  
  // Servo limits (per-profile speed/range restrictions)
  float servoSpeedLimit;       // 0.0 to 1.0
  
  // Animation permissions
  bool allowAllAnimations;     // If false, only allow safe animations
  uint8_t allowedAnimations[16];  // Bitmask of allowed animation IDs
  
  // Button mappings (6 extra buttons after joystick buttons)
  uint8_t buttonAction[6];     // Action type
  uint8_t buttonParam[6];      // Parameter (animation ID, sound ID, etc.)
  
  // Safety
  bool requireDeadman;         // Must hold deadman button
  uint16_t autoStopTimeout;    // ms, 0 = disabled
  
  // Servo neutral positions (0-180 degrees for each servo)
  uint8_t neutralPositions[10];  // Default neutral position for each servo
  
  // Favorite animations for main screen (5 slots)
  uint8_t favoriteAnimations[5];  // Animation IDs to display on main screen
  
  // Autonomous personality
  float autonomyCuriosity;         // 0.0-1.0
  float autonomyBravery;           // 0.0-1.0
  float autonomyEnergy;            // 0.0-1.0
  float autonomyRandomness;        // 0.0-1.0
  bool autonomyEnabled;            // Allow autonomous mode
};

// Profile definitions
extern Profile profiles[PROFILE_COUNT];

// Current profile
extern uint8_t g_currentProfile;

extern Profile profiles[PROFILE_COUNT];
extern uint8_t g_currentProfile;

// Init (loads from Preferences)
void profileInit();

// Get current profile
Profile* profileGet();

// Set profile (saves to Preferences)
void profileSet(uint8_t profileId);

// Cycle to next profile
void profileCycleNext();

// Apply profile settings to systems
void profileApply();

// Save current settings to Preferences
void profileSave();

// Reset to defaults
void profileResetDefaults();

// Button action handler
void profileHandleButtonAction(uint8_t buttonIndex);

// Servo editor value adjustment
void profileAdjustHeadSensitivity(float delta);
void profileAdjustServoSpeed(float delta);
void profileAdjustDeadzone(float delta);
void profileAdjustExpo(float delta);
void profileAdjustMaxSpeed(float delta);

// Servo neutral position management
void profileSaveNeutralPositions(const uint8_t* positions);  // Save current servo positions as neutral
void profileLoadNeutralPositions();  // Load neutral positions to motion engine

// Favorite animation management
void profileToggleFavoriteAnimation(uint8_t animId);  // Add/remove animation from favorites
void profileLoadFavoriteAnimations();  // Load favorite animations from Preferences

#endif // PROFILES_H
