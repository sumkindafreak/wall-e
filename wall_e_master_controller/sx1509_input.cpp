// ==========================================================
// SX1509 Implementation
// ==========================================================

#include "sx1509_input.h"
#include "i2c_devices.h"
#include <Wire.h>
#include <SparkFunSX1509.h>

SX1509 io;

const uint8_t buttonPins[BTN_COUNT] = {0, 1, 2, 3, 4, 5, 6};

#define DEBOUNCE_MS 30
static unsigned long lastChange[BTN_COUNT] = {0};
static bool rawState[BTN_COUNT] = {false};
static bool lastState[BTN_COUNT] = {false};

static ButtonState btnState = {};
static bool sx1509Ready = false;

bool sx1509Init() {
  Serial.println(F("[SX1509] Initializing..."));
  
  if (!io.begin(SX1509_ADDR)) {
    Serial.println(F("[SX1509] ❌ Not found"));
    sx1509Ready = false;
    return false;
  }
  
  Serial.println(F("[SX1509] ✓ Found"));
  
  for (int i = 0; i < BTN_COUNT; i++) {
    io.pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Diagnostic: read all pins once
  Serial.println(F("[SX1509] Initial pin states:"));
  for (int i = 0; i < BTN_COUNT; i++) {
    delay(10);
    bool state = (io.digitalRead(buttonPins[i]) == LOW);
    Serial.printf("  BTN%d (pin %d): %s\n", i, buttonPins[i], state ? "PRESSED" : "released");
  }
  
  sx1509Ready = true;
  Serial.println(F("[SX1509] ✓ Configured\n"));
  return true;
}

void sx1509Update() {
  if (!sx1509Ready) return; // Skip if device not initialized
  
  static unsigned long lastSuccessfulRead = millis();
  static int consecutiveFailures = 0;
  unsigned long now = millis();
  static int scanIndex = 0; // Only read ONE button per loop iteration
  
  // Emergency recovery if no successful reads for 1 second
  if (now - lastSuccessfulRead > 1000) {
    Serial.println(F("[SX1509] ⚠️  No reads for 1s - attempting recovery"));
    sx1509Ready = false;
    consecutiveFailures++;
    
    if (consecutiveFailures > 3) {
      Serial.println(F("[SX1509] ⚠️  Too many failures - DISABLING button input"));
      return;
    }
    
    // Try to recover
    delay(100);
    if (io.begin(SX1509_ADDR)) {
      Serial.println(F("[SX1509] ✓ Recovery successful"));
      for (int i = 0; i < BTN_COUNT; i++) {
        io.pinMode(buttonPins[i], INPUT_PULLUP);
      }
      sx1509Ready = true;
      lastSuccessfulRead = now;
    }
    return;
  }
  
  // Clear edge flags for ALL buttons every loop
  for (int i = 0; i < BTN_COUNT; i++) {
    btnState.pressed[i] = false;
    btnState.released[i] = false;
  }
  
  // Read ONLY ONE button this loop (spreads I2C load)
  int i = scanIndex;
  
  // Safety: skip button 4 if it's causing crashes
  static bool skipButton4 = false;
  if (i == 4 && skipButton4) {
    Serial.println(F("[SX1509] ⚠️  Skipping BTN4 (crash protection)"));
    scanIndex = (scanIndex + 1) % BTN_COUNT;
    return;
  }
  
  bool raw = false;
  
  // Try to read with timeout check
  unsigned long readStart = millis();
  
  // Safety: wrap in bounds check
  if (i >= 0 && i < BTN_COUNT) {
    raw = (io.digitalRead(buttonPins[i]) == LOW);
    lastSuccessfulRead = millis(); // Mark successful read
    consecutiveFailures = 0;
  } else {
    Serial.printf("[SX1509] ⚠️  Invalid scanIndex: %d\n", i);
    scanIndex = 0;
    return;
  }
  
  unsigned long readDuration = millis() - readStart;
  
  if (readDuration > 50) {
    Serial.printf("[SX1509] ⚠️  Slow I2C read on BTN%d (%lums)\n", i, readDuration);
    if (i == 4) {
      Serial.println(F("[SX1509] ⚠️  BTN4 causing slow reads - DISABLING"));
      skipButton4 = true;
    }
  }
  
  // Update state for this button
  if (raw != rawState[i]) {
    rawState[i] = raw;
    lastChange[i] = now;
  }
  
  if (now - lastChange[i] >= DEBOUNCE_MS) {
    bool newState = rawState[i];
    
    if (newState != lastState[i]) {
      if (newState) {
        btnState.pressed[i] = true;
        Serial.printf("[Button] BTN%d (pin %d) pressed\n", i, buttonPins[i]);
      } else {
        btnState.released[i] = true;
        Serial.printf("[Button] BTN%d (pin %d) released\n", i, buttonPins[i]);
      }
      lastState[i] = newState;
    }
    
    btnState.held[i] = newState;
  }
  
  // Move to next button
  scanIndex = (scanIndex + 1) % BTN_COUNT;
}

const ButtonState& getButtonState() {
  return btnState;
}

bool isBothJoystickButtonsHeld() {
  return btnState.held[BTN_JOY1] && btnState.held[BTN_JOY2];
}

bool isDeadmanButtonHeld() {
  return btnState.held[BTN_DEADMAN];
}
