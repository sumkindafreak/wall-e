#pragma once

#include <Arduino.h>

void servoInit(void);
void servoTick(void);
void servoSetAngles(int16_t leftDeg, int16_t rightDeg);
