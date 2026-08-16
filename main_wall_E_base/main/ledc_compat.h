#pragma once

// ============================================================
// WALL-E LEDC compatibility
//
// Arduino-ESP32 2.x addresses PWM by channel:
//   ledcSetup(channel, freq, bits)
//   ledcAttachPin(pin, channel)
//   ledcWrite(channel, duty)
//
// Arduino-ESP32 3.x addresses PWM by pin:
//   ledcAttach(pin, freq, bits)
//   ledcWrite(pin, duty)
//
// Keep explicit legacy channel numbers so the ESP32-S3 regression build and
// ESP32-P4 production build exercise the same higher-level motor/effect code.
// ============================================================

#include <Arduino.h>
#include <esp_arduino_version.h>

static inline bool walleLedcAttach(uint8_t pin,
                                   uint8_t legacyChannel,
                                   uint32_t frequency,
                                   uint8_t resolutionBits) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)legacyChannel;
  return ledcAttach(pin, frequency, resolutionBits);
#else
  const double actual = ledcSetup(legacyChannel, frequency, resolutionBits);
  if (actual <= 0.0) return false;
  ledcAttachPin(pin, legacyChannel);
  return true;
#endif
}

static inline bool walleLedcWrite(uint8_t pin,
                                  uint8_t legacyChannel,
                                  uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)legacyChannel;
  return ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(legacyChannel, duty);
  return true;
#endif
}

// Stable legacy channels used only by Arduino-ESP32 2.x.
#define WALLE_LEDC_CH_MOTOR_LEFT   0
#define WALLE_LEDC_CH_MOTOR_RIGHT  1
#define WALLE_LEDC_CH_DISPLAY_BL   2
#define WALLE_LEDC_CH_DOCK_IR_L    3
#define WALLE_LEDC_CH_DOCK_IR_R    4
#define WALLE_LEDC_CH_LASER        5
