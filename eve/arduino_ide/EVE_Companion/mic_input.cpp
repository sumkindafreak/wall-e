#include "mic_input.h"

#include <math.h>

#if __has_include("../../firmware_common/include/learning_engine.h")
#include "../../firmware_common/include/learning_engine.h"
#define EVE_MIC_HAVE_LEARNING 1
#else
#define EVE_MIC_HAVE_LEARNING 0
#endif

#if MIC_ENABLED && (MIC_TYPE == MIC_TYPE_I2S)
#include <driver/i2s.h>
#include <esp_intr_alloc.h>
#endif

static bool s_configured = false;
static bool s_running = false;
static bool s_reactionsEnabled = true;
static float s_level = 0.0f;
static float s_ambient = 0.0f;
static float s_peak = 0.0f;
static float s_spikeThreshold = SOUND_SPIKE_THRESHOLD;
static float s_clapThreshold = CLAP_THRESHOLD;
static float s_quietThreshold = QUIET_THRESHOLD;
static uint32_t s_cooldownMs = MIC_REACTION_COOLDOWN_MS;
static uint32_t s_lastUpdateMs = 0;
static uint32_t s_lastEventMs = 0;
static uint32_t s_quietSinceMs = 0;
static uint32_t s_lastSpikeMs = 0;
static uint8_t s_spikeTrain = 0;
static EveMicEvent s_pendingEvent = EVE_MIC_EVENT_NONE;
static EveMicEvent s_lastEvent = EVE_MIC_EVENT_NONE;

#if EVE_MIC_HAVE_LEARNING
static LearningEngine<24> s_soundLearning;
#endif

#if MIC_ENABLED && (MIC_TYPE == MIC_TYPE_I2S)
static const i2s_port_t MIC_I2S_PORT = I2S_NUM_0;
#endif

static void queueEvent(EveMicEvent event, const char* action, uint16_t score) {
  if (event == EVE_MIC_EVENT_NONE) return;
  s_pendingEvent = event;
  s_lastEvent = event;
  s_lastEventMs = millis();
  Serial.print(F("[EVE][MIC] event="));
  Serial.print(micEventName(event));
  Serial.print(F(" level="));
  Serial.print(s_level, 1);
  Serial.print(F(" ambient="));
  Serial.println(s_ambient, 1);
#if EVE_MIC_HAVE_LEARNING
  s_soundLearning.logExperience(micEventName(event), action ? action : "sound_response", "pending", score, millis());
#else
  (void)action;
  (void)score;
#endif
}

static bool cooldownReady(uint32_t nowMs) {
  return s_lastEventMs == 0 || (uint32_t)(nowMs - s_lastEventMs) >= s_cooldownMs;
}

void initMic(void) {
#if EVE_MIC_HAVE_LEARNING
  s_soundLearning.begin(LEARNING_SOURCE_EVE);
#endif

#if !MIC_ENABLED
  s_configured = false;
  s_running = false;
  Serial.println(F("[EVE][MIC] disabled: set MIC_ENABLED=1 and provide real mic pins"));
  return;
#elif MIC_TYPE == MIC_TYPE_I2S
  if (MIC_I2S_BCLK_PIN < 0 || MIC_I2S_WS_PIN < 0 || MIC_I2S_DATA_PIN < 0) {
    s_configured = false;
    s_running = false;
    Serial.println(F("[EVE][MIC] I2S mic not configured: BCLK/WS/DATA pins are still -1"));
    return;
  }

  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = MIC_SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
#if defined(I2S_COMM_FORMAT_STAND_I2S)
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#else
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
#endif
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 128;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.bck_io_num = MIC_I2S_BCLK_PIN;
  pins.ws_io_num = MIC_I2S_WS_PIN;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = MIC_I2S_DATA_PIN;

  esp_err_t err = i2s_driver_install(MIC_I2S_PORT, &cfg, 0, nullptr);
  if (err != ESP_OK) {
    Serial.print(F("[EVE][MIC] i2s_driver_install failed err="));
    Serial.println((int)err);
    s_configured = false;
    s_running = false;
    return;
  }
  err = i2s_set_pin(MIC_I2S_PORT, &pins);
  if (err != ESP_OK) {
    Serial.print(F("[EVE][MIC] i2s_set_pin failed err="));
    Serial.println((int)err);
    i2s_driver_uninstall(MIC_I2S_PORT);
    s_configured = false;
    s_running = false;
    return;
  }
  i2s_zero_dma_buffer(MIC_I2S_PORT);
  s_configured = true;
  s_running = true;
  Serial.print(F("[EVE][MIC] I2S ready BCLK="));
  Serial.print(MIC_I2S_BCLK_PIN);
  Serial.print(F(" WS="));
  Serial.print(MIC_I2S_WS_PIN);
  Serial.print(F(" DATA="));
  Serial.print(MIC_I2S_DATA_PIN);
  Serial.print(F(" rate="));
  Serial.println(MIC_SAMPLE_RATE);
#else
  s_configured = false;
  s_running = false;
  Serial.println(F("[EVE][MIC] selected mic type is not implemented; use I2S for INMP441/SPH0645/ICS-43434"));
#endif
}

void updateMic(void) {
  const uint32_t now = millis();
  if ((uint32_t)(now - s_lastUpdateMs) < 20u) return;
  s_lastUpdateMs = now;

#if MIC_ENABLED && (MIC_TYPE == MIC_TYPE_I2S)
  if (!s_running) return;
  int32_t samples[64];
  size_t bytesRead = 0;
  esp_err_t err = i2s_read(MIC_I2S_PORT, samples, sizeof(samples), &bytesRead, 0);
  if (err != ESP_OK || bytesRead == 0) return;

  const size_t count = bytesRead / sizeof(samples[0]);
  uint64_t sum = 0;
  for (size_t i = 0; i < count; i++) {
    int32_t v = samples[i] >> 14;
    if (v < 0) v = -v;
    sum += (uint32_t)v;
  }
  const float instant = count ? (float)sum / (float)count : 0.0f;
  s_level = (s_level * 0.70f) + (instant * 0.30f);
  s_ambient = (s_ambient <= 1.0f) ? s_level : (s_ambient * 0.98f) + (s_level * 0.02f);
  if (s_level > s_peak) s_peak = s_level;
  else s_peak *= 0.985f;

  const float aboveAmbient = s_level - s_ambient;
  const bool spike = aboveAmbient > s_spikeThreshold;
  const bool clap = s_level > s_clapThreshold && aboveAmbient > (s_spikeThreshold * 0.75f);
  const bool loud = s_ambient > (s_spikeThreshold * 0.80f);
  const bool quiet = s_ambient < s_quietThreshold && s_level < (s_quietThreshold * 1.35f);

  if (quiet) {
    if (s_quietSinceMs == 0) s_quietSinceMs = now;
  } else {
    s_quietSinceMs = 0;
  }

  if (!s_reactionsEnabled || !cooldownReady(now)) return;

  if (clap) {
    queueEvent(EVE_MIC_EVENT_CLAP, "happy_response", 850);
    s_lastSpikeMs = now;
    s_spikeTrain = 0;
  } else if (spike) {
    if (s_lastSpikeMs != 0 && (uint32_t)(now - s_lastSpikeMs) < 1600u) {
      if (s_spikeTrain < 255) s_spikeTrain++;
    } else {
      s_spikeTrain = 1;
    }
    s_lastSpikeMs = now;
    queueEvent(s_spikeTrain >= 3 ? EVE_MIC_EVENT_REPEATED_PATTERN : EVE_MIC_EVENT_SOUND_SPIKE,
               s_spikeTrain >= 3 ? "alert_response" : "curious_look",
               s_spikeTrain >= 3 ? 650 : 760);
  } else if (loud) {
    queueEvent(EVE_MIC_EVENT_LOUD_ENVIRONMENT, "overwhelmed_idle", 520);
  } else if (s_quietSinceMs != 0 && (uint32_t)(now - s_quietSinceMs) > 45000u) {
    queueEvent(EVE_MIC_EVENT_QUIET_ROOM, "sleepy_idle", 780);
    s_quietSinceMs = now;
  }
#endif
}

float getMicLevel(void) { return s_level; }
float getMicAmbientAverage(void) { return s_ambient; }
bool detectSoundSpike(void) { return (s_level - s_ambient) > s_spikeThreshold; }
bool detectClap(void) { return s_level > s_clapThreshold; }
bool isRoomQuiet(void) { return s_ambient < s_quietThreshold; }
bool isMicConfigured(void) { return s_configured; }
bool isMicRunning(void) { return s_running; }

EveMicEvent micConsumeEvent(void) {
  EveMicEvent e = s_pendingEvent;
  s_pendingEvent = EVE_MIC_EVENT_NONE;
  return e;
}

const char* micEventName(EveMicEvent event) {
  switch (event) {
    case EVE_MIC_EVENT_SOUND_SPIKE: return "sound_spike";
    case EVE_MIC_EVENT_CLAP: return "clap_detected";
    case EVE_MIC_EVENT_QUIET_ROOM: return "quiet_room";
    case EVE_MIC_EVENT_LOUD_ENVIRONMENT: return "loud_environment";
    case EVE_MIC_EVENT_REPEATED_PATTERN: return "repeated_sound_pattern";
    default: return "none";
  }
}

String getMicStatusJson(void) {
  String j = "{\"enabled\":";
  j += MIC_ENABLED ? "true" : "false";
  j += ",\"configured\":"; j += s_configured ? "true" : "false";
  j += ",\"running\":"; j += s_running ? "true" : "false";
  j += ",\"reactions\":"; j += s_reactionsEnabled ? "true" : "false";
  j += ",\"level\":"; j += String(s_level, 1);
  j += ",\"ambient\":"; j += String(s_ambient, 1);
  j += ",\"peak\":"; j += String(s_peak, 1);
  j += ",\"spike\":"; j += detectSoundSpike() ? "true" : "false";
  j += ",\"clap\":"; j += detectClap() ? "true" : "false";
  j += ",\"quiet\":"; j += isRoomQuiet() ? "true" : "false";
  j += ",\"last_event\":\""; j += micEventName(s_lastEvent); j += "\"";
  j += ",\"spike_threshold\":"; j += String(s_spikeThreshold, 1);
  j += ",\"clap_threshold\":"; j += String(s_clapThreshold, 1);
  j += ",\"quiet_threshold\":"; j += String(s_quietThreshold, 1);
  j += ",\"cooldown_ms\":"; j += (uint32_t)s_cooldownMs;
  j += "}";
  return j;
}

EveMicSettings micGetSettings(void) {
  EveMicSettings s;
  s.reactionsEnabled = s_reactionsEnabled;
  s.spikeThreshold = s_spikeThreshold;
  s.clapThreshold = s_clapThreshold;
  s.quietThreshold = s_quietThreshold;
  s.reactionCooldownMs = s_cooldownMs;
  return s;
}

void micSetSettings(const EveMicSettings& settings) {
  s_reactionsEnabled = settings.reactionsEnabled;
  s_spikeThreshold = settings.spikeThreshold > 1.0f ? settings.spikeThreshold : SOUND_SPIKE_THRESHOLD;
  s_clapThreshold = settings.clapThreshold > 1.0f ? settings.clapThreshold : CLAP_THRESHOLD;
  s_quietThreshold = settings.quietThreshold > 1.0f ? settings.quietThreshold : QUIET_THRESHOLD;
  s_cooldownMs = settings.reactionCooldownMs >= 250u ? settings.reactionCooldownMs : MIC_REACTION_COOLDOWN_MS;
  Serial.println(F("[EVE][MIC] settings updated"));
}

void micSetReactionsEnabled(bool enabled) {
  s_reactionsEnabled = enabled;
}

bool micReactionsEnabled(void) {
  return s_reactionsEnabled;
}

