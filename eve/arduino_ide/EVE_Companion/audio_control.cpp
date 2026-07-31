#include "audio_control.h"
#include "config.h"
#include "eve_asset_manager.h"
#include "walle_i2s_wav_player.h"

void audioInit(void) {
  eveAssetInit();
#if EVE_ENABLE_AUDIO
  Serial.println(F("[EVE][AUDIO] SD + I2S WAV engine"));
#else
  Serial.println(F("[EVE][AUDIO] disabled — set EVE_ENABLE_AUDIO and SD/I2S pins"));
#endif
}

void audioTick(void) {
  eveAssetTick(millis());
}

void audioPlayTrack(uint8_t track) {
#if EVE_ENABLE_AUDIO
  eveAssetPlayAudioTrack(track);
#else
  (void)track;
#endif
}

void audioPlayPath(const char* sdRelativePath) {
#if EVE_ENABLE_AUDIO
  eveAssetPlayAudioFile(sdRelativePath);
#else
  (void)sdRelativePath;
#endif
}

void audioQueuePath(const char* sdRelativePath) {
#if EVE_ENABLE_AUDIO
  eveAssetQueueAudioFile(sdRelativePath);
#else
  (void)sdRelativePath;
#endif
}

void audioStop(void) {
#if EVE_ENABLE_AUDIO
  walleI2sAudioStop();
#endif
}

void audioPause(void) {
#if EVE_ENABLE_AUDIO
  walleI2sAudioPause();
#endif
}

void audioResume(void) {
#if EVE_ENABLE_AUDIO
  walleI2sAudioResume();
#endif
}

void audioSetVolume(uint8_t pct) {
  eveAssetSetAudioVolume(pct);
}

uint8_t audioGetVolume(void) {
  return eveAssetGetAudioVolume();
}

bool audioIsPlaying(void) {
#if EVE_ENABLE_AUDIO
  return walleI2sAudioIsPlaying();
#else
  return false;
#endif
}

bool audioIsReady(void) {
#if EVE_ENABLE_AUDIO
  return walleI2sAudioIsReady() && eveAssetIsMounted();
#else
  return false;
#endif
}
