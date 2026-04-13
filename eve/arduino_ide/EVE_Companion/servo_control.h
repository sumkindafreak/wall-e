#pragma once

#include <Arduino.h>

void servoInit(void);
void servoTick(void);
void servoSetAngles(int16_t leftDeg, int16_t rightDeg);
/** Head pan servo (typically EVE_SERVO_L_PIN); degrees ~45–135, neutral 90 */
void servoSetHeadPanTarget(int16_t deg);
