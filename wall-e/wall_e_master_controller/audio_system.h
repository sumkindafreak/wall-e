// ============================================================
//  WALL-E Master Controller — Audio System (Non-Blocking)
//  tone() based, never blocks packet loop
// ============================================================

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <stdint.h>

// CYD: Buzzer typically on GPIO 4 or speaker pin
#ifndef AUDIO_PIN
#define AUDIO_PIN 4
#endif

// ------------------------------------------------------------
//  Sound Types
// ------------------------------------------------------------
typedef enum {
  SOUND_CLICK,
  SOUND_WARNING,
  SOUND_CONFIRM,
  SOUND_ERROR,
  SOUND_MODE_CHANGE,
  SOUND_ESTOP
} UISound;

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void audioInit(void);
void playUISound(UISound soundType);
void audioUpdate(unsigned long now);  // Call each loop — manages non-blocking playback

#endif // AUDIO_SYSTEM_H
