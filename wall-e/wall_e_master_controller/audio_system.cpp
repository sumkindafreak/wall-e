// ============================================================
//  WALL-E Master Controller — Audio System Implementation
//  Non-blocking: uses millis() to schedule tone off
// ============================================================

#include "audio_system.h"
#include <Arduino.h>

static unsigned long s_toneEndMs = 0;
static bool s_playing = false;

void audioInit(void) {
  pinMode(AUDIO_PIN, OUTPUT);
  digitalWrite(AUDIO_PIN, LOW);
  s_playing = false;
}

static void startTone(uint16_t freq, unsigned long durationMs) {
  tone(AUDIO_PIN, freq, (unsigned int)durationMs);
  s_toneEndMs = millis() + durationMs;
  s_playing = true;
}

void playUISound(UISound soundType) {
  if (s_playing) return;  // Don't interrupt

  switch (soundType) {
    case SOUND_CLICK:       startTone(800, 30);  break;
    case SOUND_WARNING:     startTone(1200, 80); break;
    case SOUND_CONFIRM:     startTone(1100, 60); break;
    case SOUND_ERROR:       startTone(400, 150); break;
    case SOUND_MODE_CHANGE: startTone(600, 40);  break;
    case SOUND_ESTOP:       startTone(800, 100); break;
  }
}

void audioUpdate(unsigned long now) {
  if (s_playing && now >= s_toneEndMs) {
    noTone(AUDIO_PIN);
    s_playing = false;
  }
}
