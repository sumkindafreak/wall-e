#include "eve_asset_manager.h"
#include "eve_storage_paths.h"
#include "config.h"
#include "walle_i2s_wav_player.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <stdio.h>
#include <string.h>

static bool s_mounted = false;
static uint8_t s_volume = 80;
static char s_trackMap[32][48];

static bool sdPinsConfigured(void) {
  return EVE_SD_SPI_CS >= 0 && EVE_SD_SPI_MOSI >= 0 && EVE_SD_SPI_SCK >= 0;
}

static bool loadJsonFromSd(const char* path, DynamicJsonDocument& doc) {
  if (!s_mounted || !path) {
    return false;
  }
  if (!SD.exists(path)) {
    eveAssetReportMissing(path);
    return false;
  }
  File f = SD.open(path, FILE_READ);
  if (!f) {
    eveAssetReportMissing(path);
    return false;
  }
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
}

void eveAssetReportMissing(const char* sdRelativePath) {
  Serial.print(F("[EVE][ASSET] missing: "));
  Serial.println(sdRelativePath ? sdRelativePath : "(null)");
}

bool eveAssetInit(void) {
  memset(s_trackMap, 0, sizeof(s_trackMap));
  s_volume = 80;

  if (!sdPinsConfigured()) {
    Serial.println(F("[EVE][ASSET] SD pins not configured — storage disabled"));
    s_mounted = false;
  } else {
    SPI.begin(EVE_SD_SPI_SCK, EVE_SD_SPI_MISO, EVE_SD_SPI_MOSI, EVE_SD_SPI_CS);
    s_mounted = SD.begin(EVE_SD_SPI_CS, SPI, 20000000);
    if (!s_mounted) {
      Serial.println(F("[EVE][ASSET] SD mount failed"));
    } else {
      Serial.println(F("[EVE][ASSET] SD mounted"));
    }
  }

#if EVE_ENABLE_AUDIO
  WalleI2sPinConfig pins = {EVE_I2S_BCLK_PIN, EVE_I2S_LRCK_PIN, EVE_I2S_DOUT_PIN, EVE_I2S_PORT_INDEX};
  if (walleI2sAudioInit(&pins)) {
    walleI2sAudioSetVolume(s_volume);
    Serial.println(F("[EVE][ASSET] I2S audio ready"));
  } else {
    Serial.println(F("[EVE][ASSET] I2S audio disabled (check pins / EVE_ENABLE_AUDIO)"));
  }
#endif

  if (s_mounted) {
    eveAssetReloadConfigs();
  }
  return s_mounted;
}

bool eveAssetIsMounted(void) {
  return s_mounted;
}

void eveAssetTick(uint32_t nowMs) {
  (void)nowMs;
#if EVE_ENABLE_AUDIO
  walleI2sAudioTick();
#endif
}

bool eveAssetExists(const char* sdRelativePath) {
  return s_mounted && sdRelativePath && SD.exists(sdRelativePath);
}

bool eveAssetBuildPath(const char* category, const char* filename, char* out, size_t outLen) {
  if (!out || outLen == 0 || !category || !filename) {
    return false;
  }
  int n = snprintf(out, outLen, "/%s/%s", category, filename);
  return n > 0 && (size_t)n < outLen;
}

bool eveAssetReloadConfigs(void) {
  if (!s_mounted) {
    return false;
  }
  DynamicJsonDocument doc(2048);
  if (loadJsonFromSd(EVE_SD_FILE_AUDIO_CFG, doc)) {
    s_volume = doc["volume"] | s_volume;
#if EVE_ENABLE_AUDIO
    walleI2sAudioSetVolume(s_volume);
#endif
    JsonObject tracks = doc["tracks"].as<JsonObject>();
    if (!tracks.isNull()) {
      for (JsonPair kv : tracks) {
        int idx = atoi(kv.key().c_str());
        if (idx >= 0 && idx < 32) {
          strncpy(s_trackMap[idx], kv.value().as<const char*>(), sizeof(s_trackMap[0]) - 1);
        }
      }
    }
    Serial.println(F("[EVE][ASSET] audio.json loaded"));
    return true;
  }
  Serial.println(F("[EVE][ASSET] audio.json not found — using /audio/NNN.wav fallback"));
  return false;
}

bool eveAssetAudioPathForTrack(uint8_t track, char* out, size_t outLen) {
  if (!out || outLen == 0) {
    return false;
  }
  if (track < 32 && s_trackMap[track][0] != '\0') {
    return eveAssetBuildPath("audio", s_trackMap[track], out, outLen);
  }
  int n = snprintf(out, outLen, EVE_SD_PATH_AUDIO "/%03u.wav", (unsigned)track);
  if (n <= 0 || (size_t)n >= outLen) {
    return false;
  }
  return true;
}

uint8_t eveAssetGetAudioVolume(void) {
  return s_volume;
}

void eveAssetSetAudioVolume(uint8_t pct) {
  s_volume = pct > 100 ? 100 : pct;
#if EVE_ENABLE_AUDIO
  walleI2sAudioSetVolume(s_volume);
#endif
}

bool eveAssetPlayAudioFile(const char* sdRelativePath) {
#if !EVE_ENABLE_AUDIO
  (void)sdRelativePath;
  return false;
#else
  if (!sdRelativePath || !walleI2sAudioIsReady()) {
    if (sdRelativePath) {
      eveAssetReportMissing(sdRelativePath);
    }
    return false;
  }
  if (!eveAssetExists(sdRelativePath)) {
    eveAssetReportMissing(sdRelativePath);
    return false;
  }
  return walleI2sAudioPlayFile(sdRelativePath);
#endif
}

bool eveAssetQueueAudioFile(const char* sdRelativePath) {
#if !EVE_ENABLE_AUDIO
  (void)sdRelativePath;
  return false;
#else
  if (!sdRelativePath || !walleI2sAudioIsReady()) {
    return false;
  }
  if (!eveAssetExists(sdRelativePath)) {
    eveAssetReportMissing(sdRelativePath);
    return false;
  }
  return walleI2sAudioQueueFile(sdRelativePath);
#endif
}

bool eveAssetPlayAudioTrack(uint8_t track) {
  char path[96];
  if (!eveAssetAudioPathForTrack(track, path, sizeof(path))) {
    return false;
  }
  Serial.printf("[EVE][ASSET] play track %u -> %s\n", (unsigned)track, path);
  return eveAssetPlayAudioFile(path);
}
