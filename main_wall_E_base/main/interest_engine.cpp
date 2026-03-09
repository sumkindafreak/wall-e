// ============================================================
//  WALL-E Interest Engine Implementation
//  Gradual curiosity accumulation for organic behavior
// ============================================================

#include "interest_engine.h"
#include "personality_engine.h"  // For curiosity rate

// ============================================================
//  Internal State
// ============================================================

static InterestState s_interest = {
  .currentInterest = 0.0f,
  .peakInterest = 0.0f,
  .lastUpdateTime = 0,
  .objectPresent = false,
  .objectDistance = 0.0f,
  .objectHeading = 0.0f,
  .detectionStartTime = 0,
  .totalDetectionTime = 0
};

static float s_investigateThreshold = INTEREST_INVESTIGATE_THRESHOLD;

// ============================================================
//  Initialization
// ============================================================

void interestInit() {
  Serial.println("[Interest] Initializing...");
  s_interest.currentInterest = 0.0f;
  s_interest.peakInterest = 0.0f;
  s_interest.lastUpdateTime = millis();
  s_interest.objectPresent = false;
  Serial.println("[Interest] Ready");
}

// ============================================================
//  Update (called every loop with sonar data)
// ============================================================

void interestUpdate(uint32_t now, bool objectDetected, float distance, float heading) {
  // Calculate delta time
  uint32_t deltaMs = now - s_interest.lastUpdateTime;
  float deltaTime = deltaMs / 1000.0f;  // Convert to seconds
  s_interest.lastUpdateTime = now;
  
  // Update object tracking
  bool wasPresent = s_interest.objectPresent;
  s_interest.objectPresent = objectDetected;
  s_interest.objectDistance = distance;
  s_interest.objectHeading = heading;
  
  // Track detection timing
  if (objectDetected) {
    if (!wasPresent) {
      // New detection
      s_interest.detectionStartTime = now;
      Serial.printf("[Interest] Object detected at %.1fcm, %.0f°\n", distance, heading);
    }
    s_interest.totalDetectionTime = now - s_interest.detectionStartTime;
    
    // Accumulate interest based on personality
    float accumRate = personalityGetInterestRate();  // 0.1 - 1.0 per second
    
    // Closer objects are more interesting
    float distanceFactor = 1.0f;
    if (distance < 50.0f) {
      distanceFactor = 1.5f;  // 50% boost for close objects
    } else if (distance > 150.0f) {
      distanceFactor = 0.7f;  // 30% reduction for far objects
    }
    
    float interestGain = accumRate * distanceFactor * deltaTime;
    s_interest.currentInterest += interestGain;
    
    // Clamp to max
    if (s_interest.currentInterest > INTEREST_MAX) {
      s_interest.currentInterest = INTEREST_MAX;
    }
    
    // Track peak
    if (s_interest.currentInterest > s_interest.peakInterest) {
      s_interest.peakInterest = s_interest.currentInterest;
    }
    
  } else {
    // No object - decay interest
    if (wasPresent) {
      Serial.printf("[Interest] Object lost (was at %.1f)\n", s_interest.currentInterest);
    }
    
    float decayAmount = INTEREST_DECAY_RATE * deltaTime;
    s_interest.currentInterest -= decayAmount;
    
    // Clamp to min
    if (s_interest.currentInterest < INTEREST_MIN) {
      s_interest.currentInterest = INTEREST_MIN;
    }
    
    // Reset detection timing
    s_interest.totalDetectionTime = 0;
  }
}

// ============================================================
//  Getters
// ============================================================

float interestGetLevel() {
  return s_interest.currentInterest;
}

float interestGetPeak() {
  return s_interest.peakInterest;
}

bool interestShouldInvestigate() {
  return s_interest.currentInterest >= s_investigateThreshold;
}

uint32_t interestGetDetectionTime() {
  return s_interest.totalDetectionTime;
}

// ============================================================
//  Control
// ============================================================

void interestAddBoost(float amount) {
  s_interest.currentInterest += amount;
  s_interest.currentInterest = constrain(s_interest.currentInterest, INTEREST_MIN, INTEREST_MAX);
  Serial.printf("[Interest] Boosted by %.1f to %.1f\n", amount, s_interest.currentInterest);
}

void interestDecay(float deltaTime) {
  float decayAmount = INTEREST_DECAY_RATE * deltaTime;
  s_interest.currentInterest -= decayAmount;
  s_interest.currentInterest = constrain(s_interest.currentInterest, INTEREST_MIN, INTEREST_MAX);
}

void interestReset() {
  Serial.printf("[Interest] Reset (was %.1f)\n", s_interest.currentInterest);
  s_interest.currentInterest = 0.0f;
  s_interest.totalDetectionTime = 0;
}

void interestSetThreshold(float threshold) {
  s_investigateThreshold = constrain(threshold, 10.0f, 95.0f);
  Serial.printf("[Interest] Threshold set to %.1f\n", s_investigateThreshold);
}

// ============================================================
//  Object Tracking
// ============================================================

float interestGetObjectDistance() {
  return s_interest.objectDistance;
}

float interestGetObjectHeading() {
  return s_interest.objectHeading;
}

bool interestIsObjectPresent() {
  return s_interest.objectPresent;
}

// ============================================================
//  Debug
// ============================================================

void interestPrintDebug() {
  Serial.printf("[Interest] Level: %.1f/%.1f (threshold: %.1f)\n",
                s_interest.currentInterest,
                s_interest.peakInterest,
                s_investigateThreshold);
  if (s_interest.objectPresent) {
    Serial.printf("  Object: %.1fcm @ %.0f° (detected for %lums)\n",
                  s_interest.objectDistance,
                  s_interest.objectHeading,
                  s_interest.totalDetectionTime);
  } else {
    Serial.println("  No object detected");
  }
}
