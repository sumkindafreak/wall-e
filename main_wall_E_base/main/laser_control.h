#pragma once

#include <Arduino.h>
#include "base_board_pins.h"

// ============================================================
// Eye-mounted laser — Base GPIO, aimed by head pan + neck-top servos
// ============================================================

#define LASER_PIN           BASE_PIN_LASER
#define SERVO_PAN_CHANNEL   0
#define SERVO_TILT_CHANNEL  1

#define LASER_PWM_FREQ      5000
#define LASER_PWM_RES       8

#define LASER_TIMEOUT_MS    5000u
#define LASER_SETTLE_MS     200u

void laserInit();
void laserUpdate(uint32_t now);

void laserOn();
void laserOff();
void laserSetBrightness(uint8_t value);

void laserAim(int pan, int tilt);
void laserFire(int pan, int tilt, uint32_t durationMs);
void laserSmoothSetTarget(int pan, int tilt, uint16_t stepPeriodMs);
void laserScanSetEnabled(bool on);
void laserSetMoodMode(int8_t mood);

String laserGetStatusJSON();
