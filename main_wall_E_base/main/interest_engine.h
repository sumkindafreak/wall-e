#pragma once

// ============================================================
//  WALL-E Interest Engine
//  Accumulation system that builds curiosity over time
//  Makes Wall-E's attention feel organic and realistic
// ============================================================

#include <Arduino.h>

// ============================================================
//  Interest Configuration
// ============================================================

#define INTEREST_MIN 0.0f
#define INTEREST_MAX 100.0f
#define INTEREST_INVESTIGATE_THRESHOLD 70.0f
#define INTEREST_DECAY_RATE 5.0f  // Points per second when object lost

// ============================================================
//  Interest State
// ============================================================

struct InterestState {
  float currentInterest;      // 0 - 100
  float peakInterest;         // Highest reached this session
  uint32_t lastUpdateTime;
  bool objectPresent;
  float objectDistance;       // Last detected distance
  float objectHeading;        // Last detected heading
  uint32_t detectionStartTime;
  uint32_t totalDetectionTime;
};

// ============================================================
//  API
// ============================================================

void interestInit();
void interestUpdate(uint32_t now, bool objectDetected, float distance, float heading);

// Getters
float interestGetLevel();           // Current interest (0-100)
float interestGetPeak();            // Peak this session
bool interestShouldInvestigate();   // Above threshold?
uint32_t interestGetDetectionTime(); // How long object has been present

// Control
void interestAddBoost(float amount);      // Instant interest boost
void interestDecay(float deltaTime);      // Manual decay
void interestReset();                     // Clear interest
void interestSetThreshold(float threshold); // Adjust investigation threshold

// Object tracking
float interestGetObjectDistance();
float interestGetObjectHeading();
bool interestIsObjectPresent();

// Debug
void interestPrintDebug();
