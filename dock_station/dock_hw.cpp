/*******************************************************************************
 * dock_hw.cpp
 * Runtime guards for legacy pin maps used on ESP32-S3 builds
 ******************************************************************************/

#include "dock_hw.h"
#include "dock_config.h"
#include <Arduino.h>

static bool s_warned_invalid_gpio[64] = {false};
static bool s_warned_invalid_output[64] = {false};
static bool s_warned_invalid_analog[64] = {false};
static bool s_warned_oled_sda = false;
static bool s_warned_oled_scl = false;

static void warnPinOnce(bool *bucket, uint8_t pin, const char *label, const char *reason) {
  if (pin < 64 && bucket[pin]) {
    return;
  }
  if (pin < 64) {
    bucket[pin] = true;
  }
  Serial.printf("[DOCK] %s disabled: GPIO%u %s\n", label, pin, reason);
}

static bool isKnownAdcCapablePin(uint8_t pin) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return (pin >= 1 && pin <= 20);
#else
  (void)pin;
  return true;
#endif
}

bool dockConfigureInputPin(uint8_t pin, uint8_t mode, const char *label) {
  if (!digitalPinIsValid(pin)) {
    warnPinOnce(s_warned_invalid_gpio, pin, label, "is not a valid GPIO on ESP32-S3");
    return false;
  }
  pinMode(pin, mode);
  return true;
}

bool dockConfigureOutputPin(uint8_t pin, uint8_t initial_level, const char *label) {
  if (!digitalPinIsValid(pin)) {
    warnPinOnce(s_warned_invalid_gpio, pin, label, "is not a valid GPIO on ESP32-S3");
    return false;
  }
  if (!digitalPinCanOutput(pin)) {
    warnPinOnce(s_warned_invalid_output, pin, label, "cannot be used as an output on ESP32-S3");
    return false;
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, initial_level);
  return true;
}

void dockWriteOutputPin(uint8_t pin, uint8_t level, const char *label) {
  if (!digitalPinIsValid(pin)) {
    warnPinOnce(s_warned_invalid_gpio, pin, label, "is not a valid GPIO on ESP32-S3");
    return;
  }
  if (!digitalPinCanOutput(pin)) {
    warnPinOnce(s_warned_invalid_output, pin, label, "cannot be used as an output on ESP32-S3");
    return;
  }
  digitalWrite(pin, level);
}

int dockDigitalReadSafe(uint8_t pin, const char *label, int fallback) {
  if (!digitalPinIsValid(pin)) {
    warnPinOnce(s_warned_invalid_gpio, pin, label, "is not a valid GPIO on ESP32-S3");
    return fallback;
  }
  return digitalRead(pin);
}

int dockAnalogReadSafe(uint8_t pin, const char *label, int fallback) {
  if (!digitalPinIsValid(pin)) {
    warnPinOnce(s_warned_invalid_gpio, pin, label, "is not a valid GPIO on ESP32-S3");
    return fallback;
  }
  if (!isKnownAdcCapablePin(pin)) {
    warnPinOnce(s_warned_invalid_analog, pin, label, "is not in the ESP32-S3 ADC pin range (GPIO1-20)");
    return fallback;
  }
  if (digitalPinToAnalogChannel(pin) < 0) {
    warnPinOnce(s_warned_invalid_analog, pin, label, "is not ADC-capable on ESP32-S3");
    return fallback;
  }
  return analogRead(pin);
}

bool dockChargeGateAvailable(void) {
  if (!digitalPinIsValid(PIN_MOSFET_GATE)) {
    warnPinOnce(s_warned_invalid_gpio, PIN_MOSFET_GATE, "charge gate", "is not a valid GPIO on ESP32-S3");
    return false;
  }
  if (!digitalPinCanOutput(PIN_MOSFET_GATE)) {
    warnPinOnce(s_warned_invalid_output, PIN_MOSFET_GATE, "charge gate", "cannot be used as an output on ESP32-S3");
    return false;
  }
  return true;
}

void dockChargeGateWrite(bool enabled) {
  // Hardware uses an active-LOW relay/MOSFET for charging:
  // enabled = LOW (on), disabled = HIGH (off)
  dockWriteOutputPin(PIN_MOSFET_GATE, enabled ? LOW : HIGH, "charge gate");
}

bool dockCurrentSenseAvailable(void) {
  if (!digitalPinIsValid(PIN_ACS712_ADC)) {
    warnPinOnce(s_warned_invalid_gpio, PIN_ACS712_ADC, "ACS712 current sense", "is not a valid GPIO on ESP32-S3");
    return false;
  }
  if (!isKnownAdcCapablePin(PIN_ACS712_ADC)) {
    warnPinOnce(s_warned_invalid_analog, PIN_ACS712_ADC, "ACS712 current sense", "is not in the ESP32-S3 ADC pin range (GPIO1-20)");
    return false;
  }
  if (digitalPinToAnalogChannel(PIN_ACS712_ADC) < 0) {
    warnPinOnce(s_warned_invalid_analog, PIN_ACS712_ADC, "ACS712 current sense", "is not ADC-capable on ESP32-S3");
    return false;
  }
  return true;
}

bool dockOledPinsAvailable(void) {
  // OLED has been removed in favour of an SPI TFT on this S3 dock build.
  // Always report OLED as unavailable so any legacy callers safely no-op.
  return false;
}