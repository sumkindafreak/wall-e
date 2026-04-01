// ============================================================
//  Scripted commands: USB Serial + optional SX1509 EXTRA buttons
// ============================================================

#pragma once

#include <Arduino.h>
#include "sx1509_input.h"

/* 1 = EXTRA1–4 map to queue macros (see command_input.cpp). Profile button
 *     actions for those indices are skipped — set 0 to restore profile-only.
 */
#ifndef USE_CMD_BUTTON_MACROS
#define USE_CMD_BUTTON_MACROS 1
#endif

void commandInputInit(void);

/** Non-blocking: read Serial lines and push to commandQueue */
void commandInputPollSerial(void);

#if USE_CMD_BUTTON_MACROS
/** Edge-triggered scripted moves on EXTRA1–4; EXTRA3+EXTRA4 same frame = STOP_ALL */
void commandInputPollButtonMacros(const ButtonState& btn);
#endif
