#pragma once

// ============================================================
//  WALL-E Servo Manager
//  PCA9685 16-channel PWM via Adafruit PWM Servo Driver library
//  I2C: SDA=GPIO21, SCL=GPIO17 (change I2C_SCL if your board uses different pins)
//
//  REQUIRED LIBRARY: "Adafruit PWM Servo Driver" via Library Manager
// ============================================================

#include <Arduino.h>

// --- I2C (shared by PCA9685 servos + MPU6050 IMU) ---
#define I2C_SDA         21
#define I2C_SCL         17   // was 22; use 17 if 22 not available (or try 18, 19, 20)
#define PCA9685_ADDR    0x40   // default address, A0-A5 all low

// --- PCA9685 oscillator frequency ---
#define PCA_OSC_FREQ    27000000   // trim if servos drift

// --- Servo channel assignments ---
#define SERVO_HEAD_PAN      0   // head rotation left/right
#define SERVO_NECK_TOP      1   // neck upper tilt
#define SERVO_NECK_BOT      2   // neck lower tilt
#define SERVO_EYE_RIGHT     3   // right eye zoom
#define SERVO_EYE_LEFT      4   // left eye zoom
#define SERVO_ARM_LEFT      5   // left arm
#define SERVO_ARM_RIGHT     6   // right arm
#define SERVO_BROW_LEFT     7   // left eyebrow
#define SERVO_BROW_RIGHT    8   // right eyebrow
#define SERVO_COUNT         9

// --- Calibration: pulse widths in microseconds ---
// These match chillibasket's LOW/HIGH format.
// Run wall-e_calibration.ino to get your exact values,
// then paste them here.
//                              LOW     HIGH
#define CAL_HEAD_PAN_LO     410
#define CAL_HEAD_PAN_HI     120
#define CAL_NECK_TOP_LO     532
#define CAL_NECK_TOP_HI     178
#define CAL_NECK_BOT_LO     120
#define CAL_NECK_BOT_HI     310
#define CAL_EYE_RIGHT_LO    465
#define CAL_EYE_RIGHT_HI    271
#define CAL_EYE_LEFT_LO     278
#define CAL_EYE_LEFT_HI     479
#define CAL_ARM_LEFT_LO     340
#define CAL_ARM_LEFT_HI     135
#define CAL_ARM_RIGHT_LO    150
#define CAL_ARM_RIGHT_HI    360
#define CAL_BROW_LEFT_LO    300   // calibrate these with wall-e_calibration.ino
#define CAL_BROW_LEFT_HI    500
#define CAL_BROW_RIGHT_LO   500   // calibrate these with wall-e_calibration.ino
#define CAL_BROW_RIGHT_HI   300

// --- Servo speed (degrees/second equivalent, 0-100 scale) ---
#define SERVO_DEFAULT_SPEED  50   // used for all servos on startup
#define SERVO_FAST_SPEED     80
#define SERVO_SLOW_SPEED     20

// --- Neutral positions (0-100 scale, 50 = centre) ---
#define NEUTRAL_HEAD_PAN     50
#define NEUTRAL_NECK_TOP     50
#define NEUTRAL_NECK_BOT     50
#define NEUTRAL_EYE_RIGHT    50
#define NEUTRAL_EYE_LEFT     50
#define NEUTRAL_ARM_LEFT     0    // arms down
#define NEUTRAL_ARM_RIGHT    0
#define NEUTRAL_BROW_LEFT    50
#define NEUTRAL_BROW_RIGHT   50

// Public API
void servoInit();
void servoHandle();                              // Call in loop() — drives velocity interpolation

void servoSet(uint8_t ch, int pos, int speed);  // pos 0-100, speed 0-100
void servoSetAll(int positions[SERVO_COUNT], int speed);
void servoNeutral(int speed);                   // All servos to neutral

int  servoGetPos(uint8_t ch);                   // Returns current target 0-100
bool servoIsMoving();                           // True if any servo still interpolating
String servoGetStatusJSON();
