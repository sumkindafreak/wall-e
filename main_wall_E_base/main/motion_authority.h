#pragma once

#include <stdint.h>

/** Who may command tank drive: CYD (ESP-NOW) vs browser/sequences (HTTP). */
typedef enum : uint8_t {
  MOTION_AUTH_ANY = 0,      /**< Both sources; last command wins (legacy). */
  MOTION_AUTH_CYD_ONLY = 1, /**< Physical CYD only; HTTP drive rejected. */
  MOTION_AUTH_WEB_ONLY = 2, /**< LROS/sequences only; CYD drive ignored (E-STOP still works). */
} MotionAuthorityMode;

void motionAuthorityInit(void);
MotionAuthorityMode motionAuthorityGet(void);
void motionAuthoritySet(MotionAuthorityMode m);

bool motionAuthorityAllowCyd(void);
bool motionAuthorityAllowWeb(void);

const char* motionAuthorityModeName(MotionAuthorityMode m);
