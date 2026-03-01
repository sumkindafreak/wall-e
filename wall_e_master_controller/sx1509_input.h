// ==========================================================
// SX1509 Button Input
// Joystick buttons + 4 extra programmable buttons
// ==========================================================

#ifndef SX1509_INPUT_H
#define SX1509_INPUT_H

#include <Arduino.h>

// Button indices
#define BTN_JOY1      0  // Right eyebrow (future)
#define BTN_JOY2      1  // Left eyebrow (future)
#define BTN_EXTRA1    2  // Programmable
#define BTN_EXTRA2    3  // Programmable
#define BTN_EXTRA3    4  // Programmable
#define BTN_EXTRA4    5  // Programmable
#define BTN_DEADMAN   6  // ACCELERATOR - Must be held to move (SAFETY)
#define BTN_COUNT     7  // Total 7 buttons

struct ButtonState {
  bool held[BTN_COUNT];
  bool pressed[BTN_COUNT];  // Edge
  bool released[BTN_COUNT]; // Edge
};

bool sx1509Init();
void sx1509Update();
const ButtonState& getButtonState();

bool isBothJoystickButtonsHeld();
bool isDeadmanButtonHeld();  // Check if accelerator button is held

#endif
