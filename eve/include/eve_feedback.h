#pragma once

#include <Arduino.h>

void eveFeedbackInit(void);

/** Lightweight operator feedback: Serial when no display bridge is present. */
void showToast(const char* message);
