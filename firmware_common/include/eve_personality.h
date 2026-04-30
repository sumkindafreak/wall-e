#pragma once

/*
 * EVE desktop buddy personality layer.
 *
 * This is an additive behavior engine. It does not own pins or hardware drivers;
 * sketches provide callbacks that map to the existing eyes, servo, NeoPixel,
 * audio, and ToF APIs.
 */

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

enum EvePersonalityMode : uint8_t {
  EVE_PERSONALITY_OFF = 0,
  EVE_PERSONALITY_DESKTOP_BUDDY = 1,
  EVE_PERSONALITY_WATCHING_WORK = 2,
  EVE_PERSONALITY_SLEEPY = 3
};

enum EveEmotion : uint8_t {
  EVE_EMOTION_CALM = 0,
  EVE_EMOTION_CURIOUS = 1,
  EVE_EMOTION_COMFORT = 2,
  EVE_EMOTION_EXCITEMENT = 3,
  EVE_EMOTION_SLEEPINESS = 4
};

struct EvePersonalityConfig {
  uint8_t curiosity = 60;
  uint8_t comfort = 55;
  uint8_t excitement = 35;
  uint8_t sleepiness = 20;
  uint8_t responsiveness = 60;
  uint8_t activity = 45;
};

struct EvePersonalityInput {
  uint16_t tofLeftMm = 0;
  uint16_t tofCenterMm = 0;
  uint16_t tofRightMm = 0;
  bool charging = false;
  bool docked = false;
  bool notificationPending = false;
};

struct EvePersonalityOutput {
  EveEmotion emotion = EVE_EMOTION_CALM;
  uint8_t eyesMode = 1;
  uint8_t glowPattern = 1;
  uint8_t audioTrack = 0;
  int16_t headPanDeg = 90;
  int16_t rightArmDeg = 90;
  bool playAudio = false;
};

typedef void (*EveEyesCallback)(uint8_t mode);
typedef void (*EveGlowCallback)(uint8_t pattern);
typedef void (*EveServoCallback)(int16_t headPanDeg, int16_t rightArmDeg);
typedef void (*EveAudioCallback)(uint8_t track);

struct EvePersonalityHardware {
  EveEyesCallback setEyes = nullptr;
  EveGlowCallback setGlow = nullptr;
  EveServoCallback setServo = nullptr;
  EveAudioCallback playAudio = nullptr;
};

class EvePersonalityEngine {
 public:
  void begin(const EvePersonalityHardware& hw) {
    hw_ = hw;
    mode_ = EVE_PERSONALITY_OFF;
    config_ = EvePersonalityConfig();
    lastTickMs_ = 0;
    lastPresenceMs_ = 0;
    lastActionMs_ = 0;
    lastAudioMs_ = 0;
    boredomLevel_ = 0;
    output_ = EvePersonalityOutput();
  }

  void setConfig(const EvePersonalityConfig& config) { config_ = clampConfig(config); }
  EvePersonalityConfig config() const { return config_; }
  EvePersonalityMode mode() const { return mode_; }
  EvePersonalityOutput output() const { return output_; }

  void setActive(bool active, bool charging, uint32_t nowMs) {
    EvePersonalityMode next = active ? EVE_PERSONALITY_DESKTOP_BUDDY : EVE_PERSONALITY_OFF;
    if (active && config_.sleepiness > 75u) next = EVE_PERSONALITY_SLEEPY;
    if (next == mode_) return;

    mode_ = next;
    output_ = EvePersonalityOutput();
    output_.glowPattern = charging ? 4u : 1u;
    lastActionMs_ = nowMs;
    applyOutput(false);
  }

  bool isActive() const { return mode_ != EVE_PERSONALITY_OFF; }

  void tick(uint32_t nowMs, const EvePersonalityInput& input) {
    if (!isActive()) return;
    if (lastTickMs_ != 0 && (uint32_t)(nowMs - lastTickMs_) < 40u) return;
    lastTickMs_ = nowMs;

    const bool present = presenceDetected(input);
    if (present) {
      if (lastPresenceMs_ == 0 || (uint32_t)(nowMs - lastPresenceMs_) > 8000u) {
        happyReturn(nowMs, input);
      } else {
        watchMovement(nowMs, input);
      }
      lastPresenceMs_ = nowMs;
      boredomLevel_ = 0;
      return;
    }

    if (input.notificationPending) {
      notificationPulse(nowMs);
      return;
    }

    const uint32_t awayMs = lastPresenceMs_ ? (uint32_t)(nowMs - lastPresenceMs_) : nowMs;
    if (awayMs > 90000u) {
      sleepyIdle(nowMs, input);
    } else if (awayMs > 25000u) {
      boredIdle(nowMs, input);
    } else {
      calmIdle(nowMs, input);
    }
  }

 private:
  static uint8_t clampPercent(uint8_t value) {
    return value > 100u ? 100u : value;
  }

  static EvePersonalityConfig clampConfig(EvePersonalityConfig c) {
    c.curiosity = clampPercent(c.curiosity);
    c.comfort = clampPercent(c.comfort);
    c.excitement = clampPercent(c.excitement);
    c.sleepiness = clampPercent(c.sleepiness);
    c.responsiveness = clampPercent(c.responsiveness);
    c.activity = clampPercent(c.activity);
    return c;
  }

  bool presenceDetected(const EvePersonalityInput& input) const {
    const uint16_t nearMm = (uint16_t)(900u + (uint16_t)config_.responsiveness * 6u);
    return validNear(input.tofLeftMm, nearMm) || validNear(input.tofCenterMm, nearMm) ||
           validNear(input.tofRightMm, nearMm);
  }

  static bool validNear(uint16_t mm, uint16_t nearMm) {
    return mm > 0u && mm <= nearMm;
  }

  int16_t headTargetFromTof(const EvePersonalityInput& input) const {
    if (input.tofLeftMm > 0u && (input.tofCenterMm == 0u || input.tofLeftMm < input.tofCenterMm) &&
        (input.tofRightMm == 0u || input.tofLeftMm < input.tofRightMm)) {
      return 60;
    }
    if (input.tofRightMm > 0u && (input.tofCenterMm == 0u || input.tofRightMm < input.tofCenterMm)) {
      return 120;
    }
    return 90;
  }

  void calmIdle(uint32_t nowMs, const EvePersonalityInput& input) {
    const uint8_t activity = config_.activity > 80u ? 80u : config_.activity;
    if ((uint32_t)(nowMs - lastActionMs_) < (uint32_t)(2800u - (uint32_t)activity * 20u)) {
      return;
    }
    output_.emotion = EVE_EMOTION_COMFORT;
    output_.eyesMode = 1;
    output_.glowPattern = input.charging ? 4u : 1u;
    output_.headPanDeg = 86 + (int16_t)((nowMs / 1700u) % 9u);
    output_.rightArmDeg = 82 + (int16_t)((nowMs / 2200u) % 14u);
    applyOutput(false);
    lastActionMs_ = nowMs;
  }

  void watchMovement(uint32_t nowMs, const EvePersonalityInput& input) {
    if ((uint32_t)(nowMs - lastActionMs_) < 250u) return;
    output_.emotion = EVE_EMOTION_CURIOUS;
    output_.eyesMode = 2;
    output_.glowPattern = 5;
    output_.headPanDeg = headTargetFromTof(input);
    output_.rightArmDeg = 95 + (int16_t)(config_.curiosity / 5u);
    applyOutput(false);
    lastActionMs_ = nowMs;
  }

  void happyReturn(uint32_t nowMs, const EvePersonalityInput& input) {
    output_.emotion = EVE_EMOTION_EXCITEMENT;
    output_.eyesMode = 2;
    output_.glowPattern = 5;
    output_.headPanDeg = headTargetFromTof(input);
    output_.rightArmDeg = 145;
    output_.audioTrack = 1;
    output_.playAudio = (uint32_t)(nowMs - lastAudioMs_) > 6000u;
    applyOutput(output_.playAudio);
    if (output_.playAudio) lastAudioMs_ = nowMs;
    output_.playAudio = false;
    lastActionMs_ = nowMs;
  }

  void boredIdle(uint32_t nowMs, const EvePersonalityInput& input) {
    const uint8_t activity = config_.activity > 90u ? 90u : config_.activity;
    const uint32_t cadence = 8000u - (uint32_t)activity * 45u;
    if ((uint32_t)(nowMs - lastActionMs_) < cadence) return;
    if (boredomLevel_ < 100u) boredomLevel_ += 10u;
    output_.emotion = EVE_EMOTION_CURIOUS;
    output_.eyesMode = 1;
    output_.glowPattern = input.charging ? 4u : 3u;
    output_.headPanDeg = (boredomLevel_ & 0x10u) ? 68 : 112;
    output_.rightArmDeg = 100 + (int16_t)(boredomLevel_ / 3u);
    applyOutput(false);
    lastActionMs_ = nowMs;
  }

  void sleepyIdle(uint32_t nowMs, const EvePersonalityInput& input) {
    const uint32_t cadence = 12000u + (uint32_t)config_.sleepiness * 60u;
    if ((uint32_t)(nowMs - lastActionMs_) < cadence) return;
    mode_ = EVE_PERSONALITY_SLEEPY;
    output_.emotion = EVE_EMOTION_SLEEPINESS;
    output_.eyesMode = 0;
    output_.glowPattern = input.charging ? 4u : 0u;
    output_.headPanDeg = 90;
    output_.rightArmDeg = 75;
    applyOutput(false);
    lastActionMs_ = nowMs;
  }

  void notificationPulse(uint32_t nowMs) {
    if ((uint32_t)(nowMs - lastActionMs_) < 1000u) return;
    output_.emotion = EVE_EMOTION_EXCITEMENT;
    output_.eyesMode = 2;
    output_.glowPattern = 5;
    output_.audioTrack = 2;
    output_.playAudio = (uint32_t)(nowMs - lastAudioMs_) > 8000u;
    applyOutput(output_.playAudio);
    if (output_.playAudio) lastAudioMs_ = nowMs;
    output_.playAudio = false;
    lastActionMs_ = nowMs;
  }

  void applyOutput(bool playAudio) {
    if (hw_.setEyes) hw_.setEyes(output_.eyesMode);
    if (hw_.setGlow) hw_.setGlow(output_.glowPattern);
    if (hw_.setServo) hw_.setServo(output_.headPanDeg, output_.rightArmDeg);
    if (playAudio && hw_.playAudio && output_.audioTrack != 0u) hw_.playAudio(output_.audioTrack);
  }

  EvePersonalityHardware hw_;
  EvePersonalityConfig config_;
  EvePersonalityMode mode_ = EVE_PERSONALITY_OFF;
  EvePersonalityOutput output_;
  uint32_t lastTickMs_ = 0;
  uint32_t lastPresenceMs_ = 0;
  uint32_t lastActionMs_ = 0;
  uint32_t lastAudioMs_ = 0;
  uint8_t boredomLevel_ = 0;
};

