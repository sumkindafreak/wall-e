// ==========================================================
// SX1509 Implementation
//
// WALL-E physical-button safety rules:
// - Active-low buttons with 30 ms debounce.
// - Loss of the SX1509 immediately clears every held state.
// - The deadman/accelerator can never remain logically held after an I2C fault.
// - After an I2C fault the deadman must be physically RELEASED once before it
//   can arm again.  This prevents motion resuming merely because the expander
//   recovered while the operator was still holding the button.
// - Recovery is non-blocking and retried periodically.
// ==========================================================

#include "sx1509_input.h"
#include "i2c_devices.h"
#include <Wire.h>
#include <SparkFunSX1509.h>

SX1509 io;

const uint8_t buttonPins[BTN_COUNT] = {0, 1, 2, 3, 4, 5, 6};

static constexpr uint32_t DEBOUNCE_MS = 30;
static constexpr uint32_t HEALTH_CHECK_MS = 500;
static constexpr uint32_t RECOVERY_RETRY_MS = 1000;

static uint32_t lastChange[BTN_COUNT] = {0};
static bool rawState[BTN_COUNT] = {false};
static bool lastState[BTN_COUNT] = {false};

static ButtonState btnState = {};
static bool sx1509Ready = false;
static bool deadmanRearmRequired = true;
static uint32_t lastHealthCheckMs = 0;
static uint32_t nextRecoveryAttemptMs = 0;
static uint8_t scanIndex = 0;

static void clearEdgeFlags() {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    btnState.pressed[i] = false;
    btnState.released[i] = false;
  }
}

static bool sx1509Present() {
  Wire.beginTransmission(SX1509_ADDR);
  return Wire.endTransmission() == 0;
}

static void forceSafeReleasedState(uint32_t now, bool createReleaseEdges) {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    if (createReleaseEdges && btnState.held[i]) {
      btnState.released[i] = true;
    }
    btnState.held[i] = false;
    rawState[i] = false;
    lastState[i] = false;
    lastChange[i] = now;
  }

  // A recovered input expander must never make WALL-E start moving again just
  // because the accelerator happened to remain physically held during fault.
  deadmanRearmRequired = true;
  scanIndex = 0;
}

static bool configureExpander(uint32_t now) {
  if (!sx1509Present()) {
    return false;
  }

  if (!io.begin(SX1509_ADDR)) {
    return false;
  }

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    io.pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Start logically released. The normal debounce scan will establish the
  // actual states, with the deadman remaining blocked until it is released.
  forceSafeReleasedState(now, false);
  lastHealthCheckMs = now;
  return true;
}

bool sx1509Init() {
  Serial.println(F("[SX1509] Initializing..."));

  const uint32_t now = millis();
  sx1509Ready = configureExpander(now);

  if (!sx1509Ready) {
    Serial.println(F("[SX1509] Not found - buttons disabled safely; retrying in background"));
    nextRecoveryAttemptMs = now + RECOVERY_RETRY_MS;
    return false;
  }

  Serial.println(F("[SX1509] Found and configured"));
  Serial.println(F("[SX1509] Deadman requires a released state before it can arm"));
  return true;
}

void sx1509Update() {
  const uint32_t now = millis();
  clearEdgeFlags();

  // Non-blocking recovery path.  The previous implementation returned forever
  // once sx1509Ready became false, so a transient I2C problem could never heal.
  if (!sx1509Ready) {
    if ((int32_t)(now - nextRecoveryAttemptMs) < 0) {
      return;
    }

    nextRecoveryAttemptMs = now + RECOVERY_RETRY_MS;
    Serial.println(F("[SX1509] Attempting I2C recovery..."));

    if (configureExpander(now)) {
      sx1509Ready = true;
      Serial.println(F("[SX1509] Recovery successful; inputs remain safe until rescanned"));
    } else {
      Serial.println(F("[SX1509] Recovery failed; buttons remain disabled"));
    }
    return;
  }

  // digitalRead() from this library does not provide a reliable transport
  // error result, so probe the device explicitly.  Most importantly this makes
  // a disconnected deadman fail RELEASED rather than staying latched true.
  if (now - lastHealthCheckMs >= HEALTH_CHECK_MS) {
    lastHealthCheckMs = now;
    if (!sx1509Present()) {
      Serial.println(F("[SX1509] I2C link lost - forcing all buttons RELEASED"));
      forceSafeReleasedState(now, true);
      sx1509Ready = false;
      nextRecoveryAttemptMs = now + RECOVERY_RETRY_MS;
      return;
    }
  }

  const uint8_t i = scanIndex;
  const bool rawPressed = (io.digitalRead(buttonPins[i]) == LOW);

  if (rawPressed != rawState[i]) {
    rawState[i] = rawPressed;
    lastChange[i] = now;
  }

  if (now - lastChange[i] >= DEBOUNCE_MS) {
    bool debouncedPressed = rawState[i];

    // Safety interlock: after boot or bus recovery, seeing the deadman held is
    // not enough.  It has to be observed released once before it can arm.
    if (i == BTN_DEADMAN && deadmanRearmRequired) {
      if (!debouncedPressed) {
        deadmanRearmRequired = false;
        Serial.println(F("[Button] Deadman re-armed after release"));
      } else {
        debouncedPressed = false;
      }
    }

    if (debouncedPressed != lastState[i]) {
      if (debouncedPressed) {
        btnState.pressed[i] = true;
        Serial.printf("[Button] BTN%u (SX1509 pin %u) pressed\n",
                      (unsigned)i, (unsigned)buttonPins[i]);
      } else {
        btnState.released[i] = true;
        Serial.printf("[Button] BTN%u (SX1509 pin %u) released\n",
                      (unsigned)i, (unsigned)buttonPins[i]);
      }
      lastState[i] = debouncedPressed;
    }

    btnState.held[i] = debouncedPressed;
  }

  scanIndex = (uint8_t)((scanIndex + 1U) % BTN_COUNT);
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
