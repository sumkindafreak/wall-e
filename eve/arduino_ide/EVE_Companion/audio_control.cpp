#include "audio_control.h"

#include "config.h"
#include "eve_asset_manager.h"
#include "eve_safety.h"
#include "walle_i2s_wav_player.h"

static uint32_t s_fails = 0;

#if EVE_ENABLE_AUDIO
static uint32_t s_lastRefuseLogMs = 0;
static bool s_refuseLogged = false;

static bool soundAllowed(const char* what) {
  if (eveSafetySoundAllowed()) return true;
  s_fails++;
  const uint32_t now = millis();
  if (!s_refuseLogged || (uint32_t)(now - s_lastRefuseLogMs) >= EVE_LOG_RATE_MS) {
    s_refuseLogged = true;
    s_lastRefuseLogMs = now;
    Serial.printf("[EVE][AUDIO] %s refused: %s\n", what, eveSafetyStopLatched() ? "stop latched" : "fault");
  }
  return false;
}
#endif

void audioInit(void) {
  eveAssetInit();
#if EVE_ENABLE_AUDIO
  const bool ready = walleI2sAudioIsReady() && eveAssetIsMounted();
  Serial.printf("[EVE][AUDIO] SD + I2S WAV engine: %s\n", ready ? "ready" : "NOT ready (EVE runs silently)");
  /* Not blocking: EVE behaves normally without sound. */
  eveSafetySetFault(EVE_FAULT_AUDIO, !ready);
#else
  Serial.println(F("[EVE][AUDIO] disabled — set EVE_ENABLE_AUDIO and SD/I2S pins"));
#endif
}

void audioTick(void) { eveAssetTick(millis()); }

bool audioPlayTrack(uint8_t track) {
#if EVE_ENABLE_AUDIO
  if (!soundAllowed("track")) return false;
  const bool ok = eveAssetPlayAudioTrack(track);
  if (!ok) s_fails++;
  return ok;
#else
  (void)track;
  return false;
#endif
}

bool audioPlayPath(const char* sdRelativePath) {
#if EVE_ENABLE_AUDIO
  if (!soundAllowed("file")) return false;
  const bool ok = eveAssetPlayAudioFile(sdRelativePath);
  if (!ok) s_fails++;
  return ok;
#else
  (void)sdRelativePath;
  return false;
#endif
}

bool audioQueuePath(const char* sdRelativePath) {
#if EVE_ENABLE_AUDIO
  if (!soundAllowed("queue")) return false;
  const bool ok = eveAssetQueueAudioFile(sdRelativePath);
  if (!ok) s_fails++;
  return ok;
#else
  (void)sdRelativePath;
  return false;
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
  if (!soundAllowed("resume")) return;
  walleI2sAudioResume();
#endif
}

void audioSetVolume(uint8_t pct) { eveAssetSetAudioVolume(pct); }

uint8_t audioGetVolume(void) { return eveAssetGetAudioVolume(); }

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

uint32_t audioFailCount(void) { return s_fails; }
