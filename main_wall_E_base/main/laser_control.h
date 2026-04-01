#pragma once

#include <Arduino.h>

// ============================================================
//  Eye-mounted laser — GPIO on BASE (not PCA9685)
//  Aim via head pan + neck top tilt (same 0–100 scale as servoSet)
// ============================================================

/* Many ESP32-S3 modules do not break out GPIO25; 18 is free here (ex–IR beam pin). */
#define LASER_PIN           18
#define SERVO_PAN_CHANNEL   0   // SERVO_HEAD_PAN
#define SERVO_TILT_CHANNEL  1   // SERVO_NECK_TOP (upper neck tilt)

#define LASER_PWM_FREQ      5000
#define LASER_PWM_RES       8

#define LASER_TIMEOUT_MS    5000u
#define LASER_SETTLE_MS     200u

void laserInit();
void laserUpdate(uint32_t now);

void laserOn();
void laserOff();
void laserSetBrightness(uint8_t value);

/** Aim laser (0–100 each, matches PCA9685 position scale). */
void laserAim(int pan, int tilt);

/** Non-blocking timed pulse (schedules internal state machine). */
void laserFire(int pan, int tilt, uint32_t durationMs);

/** Smooth move toward target; call laserUpdate from loop. */
void laserSmoothSetTarget(int pan, int tilt, uint16_t stepPeriodMs);

/** Sweep pan between min/max while enabled (non-blocking). */
void laserScanSetEnabled(bool on);

/** 0 = curious (slow), 1 = angry (fast jitter), 2 = happy (medium sweep). */
void laserSetMoodMode(int8_t mood);

String laserGetStatusJSON();
