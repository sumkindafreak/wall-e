# Master Controller Customization Guide

This file lists user-facing settings that are safe to tune.

Look for `USER-CUSTOMIZABLE` markers in code.

## Quick Index

- `profiles.cpp`  
  - Default profiles (`Kid`, `Demo`, `Advanced`)  
  - Joystick feel, head sensitivity, servo speed limit  
  - Favorite animation defaults for button mapping  
  - Autonomy personality defaults

- `profiles.h`  
  - `Profile` structure fields and ranges

- `wall_e_master_controller.ino`  
  - Physical button mapping to favorite slots  
  - Toast text/duration behavior for button presses

- `ads1115_input.cpp`  
  - Joystick ADC tuning defaults (`deadzone`, `expo`, `maxOutput`)

- `command_input.h`  
  - Optional legacy macro mode (`USE_CMD_BUTTON_MACROS`)

## Current Physical Button Mapping (customizable from touchscreen)

- `JOY1` -> favorite slot `0`
- `JOY2` -> favorite slot `1`
- `EXTRA1` -> favorite slot `2`
- `EXTRA2` -> favorite slot `3`
- `EXTRA3` -> favorite slot `4`
- `EXTRA4` -> favorite slot `5`

These favorites are assigned in the Behaviour page (long-press animation tile).

## Safety-Critical (do not casually change)

- Deadman behavior (`BTN_DEADMAN`) in `wall_e_master_controller.ino`
- E-STOP behavior (joystick-button combo and touch E-STOP)

If you change safety behavior, re-test on blocks/stands before floor driving.
