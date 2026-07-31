/**
 * EVE — SD card asset manager: mount, verify, load config/audio/graphics paths.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

bool eveAssetInit(void);
bool eveAssetIsMounted(void);
void eveAssetTick(uint32_t nowMs);

bool eveAssetExists(const char* sdRelativePath);

/** category examples: "audio", "images" — joins /{category}/{name} */
bool eveAssetBuildPath(const char* category, const char* filename, char* out, size_t outLen);

/** Legacy UART track index → WAV path using /config/audio.json or /audio/NNN.wav fallback. */
bool eveAssetAudioPathForTrack(uint8_t track, char* out, size_t outLen);

bool eveAssetPlayAudioTrack(uint8_t track);
bool eveAssetPlayAudioFile(const char* sdRelativePath);
bool eveAssetQueueAudioFile(const char* sdRelativePath);

uint8_t eveAssetGetAudioVolume(void);
void eveAssetSetAudioVolume(uint8_t pct);

/** Reload JSON configs from SD (safe to call after card hot-insert). */
bool eveAssetReloadConfigs(void);

/** Log missing paths without crashing. */
void eveAssetReportMissing(const char* sdRelativePath);
