#pragma once

// ============================================================
// WALL-E Servo Manager
// PCA9685 16-channel PWM via shared Base I2C bus
// ============================================================

#include <Arduino.h>
#include "base_board_pins.h"

#define I2C_SDA         BASE_PIN_I2C_SDA
#define I2C_SCL         BASE_PIN_I2C_SCL
#define PCA9685_ADDR    0x40

#define PCA_OSC_FREQ    27000000

#define SERVO_HEAD_PAN      0
#define SERVO_NECK_TOP      1
#define SERVO_NECK_BOT      2
#define SERVO_EYE_RIGHT     3
#define SERVO_EYE_LEFT      4
#define SERVO_ARM_LEFT      5
#define SERVO_ARM_RIGHT     6
#define SERVO_BROW_LEFT     7
#define SERVO_BROW_RIGHT    8
#define SERVO_COUNT         9

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
#define CAL_BROW_LEFT_LO    300
#define CAL_BROW_LEFT_HI    500
#define CAL_BROW_RIGHT_LO   500
#define CAL_BROW_RIGHT_HI   300

#define SERVO_DEFAULT_SPEED  50
#define SERVO_FAST_SPEED     80
#define SERVO_SLOW_SPEED     20

#define NEUTRAL_HEAD_PAN     50
#define NEUTRAL_NECK_TOP     50
#define NEUTRAL_NECK_BOT     50
#define NEUTRAL_EYE_RIGHT    50
#define NEUTRAL_EYE_LEFT     50
#define NEUTRAL_ARM_LEFT     0
#define NEUTRAL_ARM_RIGHT    0
#define NEUTRAL_BROW_LEFT    50
#define NEUTRAL_BROW_RIGHT   50

void servoInit();
void servoHandle();

void servoSet(uint8_t ch, int pos, int speed);
void servoSetAll(int positions[SERVO_COUNT], int speed);
void servoNeutral(int speed);

int  servoGetPos(uint8_t ch);
bool servoIsMoving();
String servoGetStatusJSON();
