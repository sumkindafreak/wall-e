#include "audio_control.h"
#include "config.h"

void audioInit(void) {
#if EVE_ENABLE_AUDIO
  Serial.println(F("[EVE][AUDIO] init DFPlayer UART"));
#else
  Serial.println(F("[EVE][AUDIO] disabled — set EVE_ENABLE_AUDIO and DFPlayer pins"));
#endif
}

void audioTick(void) {
}

void audioPlayTrack(uint8_t track) {
  (void)track;
#if EVE_ENABLE_AUDIO
#endif
}
