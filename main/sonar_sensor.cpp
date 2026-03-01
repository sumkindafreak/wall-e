// ============================================================
//  WALL-E Master Controller — Ultrasonic Sonar Implementation
// ============================================================

#include "sonar_sensor.h"

// Internal state
static float s_distanceCm = 0.0f;
static float s_rawDistanceCm = 0.0f;
static bool s_valid = false;
static uint32_t s_lastUpdateMs = 0;
static uint32_t s_lastTriggerMs = 0;

// Non-blocking measurement state
enum SonarState {
  SONAR_IDLE,
  SONAR_TRIGGERED,
  SONAR_WAITING_ECHO
};

static SonarState s_state = SONAR_IDLE;
static uint32_t s_echoStartTime = 0;

// Rolling average filter
static float s_filterBuffer[SONAR_FILTER_SIZE];
static int s_filterIndex = 0;
static bool s_filterFull = false;

void sonarInit() {
  pinMode(SONAR_TRIGGER_PIN, OUTPUT);
  pinMode(SONAR_ECHO_PIN, INPUT);
  digitalWrite(SONAR_TRIGGER_PIN, LOW);
  
  // Initialize filter
  for (int i = 0; i < SONAR_FILTER_SIZE; i++) {
    s_filterBuffer[i] = 0.0f;
  }
  
  Serial.println(F("[Sonar] Initialized"));
}

void sonarUpdate(uint32_t now) {
  // Rate limit: only measure every SONAR_UPDATE_MS
  if (now - s_lastUpdateMs < SONAR_UPDATE_MS) {
    return;
  }
  
  switch (s_state) {
    case SONAR_IDLE: {
      // Start new measurement
      digitalWrite(SONAR_TRIGGER_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(SONAR_TRIGGER_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(SONAR_TRIGGER_PIN, LOW);
      
      s_state = SONAR_TRIGGERED;
      s_lastTriggerMs = now;
      break;
    }
    
    case SONAR_TRIGGERED: {
      // Wait for echo pin to go HIGH
      if (digitalRead(SONAR_ECHO_PIN) == HIGH) {
        s_echoStartTime = micros();
        s_state = SONAR_WAITING_ECHO;
      } else if (now - s_lastTriggerMs > 10) {
        // Timeout waiting for echo start
        s_valid = false;
        s_state = SONAR_IDLE;
        s_lastUpdateMs = now;
      }
      break;
    }
    
    case SONAR_WAITING_ECHO: {
      // Wait for echo pin to go LOW
      if (digitalRead(SONAR_ECHO_PIN) == LOW) {
        uint32_t echoEndTime = micros();
        uint32_t pulseDuration = echoEndTime - s_echoStartTime;
        
        // Convert to distance (cm)
        // distance = (time * speed_of_sound) / 2
        // time in microseconds, speed in cm/us
        s_rawDistanceCm = (pulseDuration * 0.0343f) / 2.0f;
        
        // Validate range
        if (s_rawDistanceCm >= SONAR_MIN_CM && s_rawDistanceCm <= SONAR_MAX_CM) {
          // Add to filter
          s_filterBuffer[s_filterIndex] = s_rawDistanceCm;
          s_filterIndex = (s_filterIndex + 1) % SONAR_FILTER_SIZE;
          if (s_filterIndex == 0) s_filterFull = true;
          
          // Calculate filtered average
          float sum = 0.0f;
          int count = s_filterFull ? SONAR_FILTER_SIZE : s_filterIndex;
          for (int i = 0; i < count; i++) {
            sum += s_filterBuffer[i];
          }
          s_distanceCm = sum / (float)count;
          
          s_valid = true;
        } else {
          s_valid = false;
        }
        
        s_state = SONAR_IDLE;
        s_lastUpdateMs = now;
        
      } else if (micros() - s_echoStartTime > SONAR_TIMEOUT_US) {
        // Timeout waiting for echo end
        s_valid = false;
        s_state = SONAR_IDLE;
        s_lastUpdateMs = now;
      }
      break;
    }
  }
  
  // Check for stale data
  if (now - s_lastUpdateMs > (SONAR_UPDATE_MS * 2)) {
    s_valid = false;
  }
}

float sonarGetDistanceCm() {
  return s_distanceCm;
}

bool sonarIsValid() {
  return s_valid;
}

float sonarGetRawDistanceCm() {
  return s_rawDistanceCm;
}

uint32_t sonarGetLastUpdateMs() {
  return s_lastUpdateMs;
}
