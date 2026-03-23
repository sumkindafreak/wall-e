/**
 * audio_esp.ino — WALL-E Audio / Voice / Dock Sensor Brain
 *
 * This ESP32-S3 module handles:
 * - DFPlayer audio playback
 * - Left/right mic direction (LEFT, RIGHT, CENTER, UNKNOWN)
 * - Voice commands (from dedicated voice module if present)
 * - IR dock receivers (NONE, LEFT, RIGHT, BOTH, UNSTABLE)
 * - ESP-NOW communication with main Base ESP32-S3
 *
 * RULE: This board NEVER controls drive motors. It only senses, plays, and reports.
 */
#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include "system_state.h"
#include "audio_player.h"
#include "mic_manager.h"
#include "voice_commands.h"
#include "ir_dock_receivers.h"
#include "espnow_manager.h"
#include "event_router.h"
#include "diagnostics.h"

/* Last values for change detection */
static MicDirection s_lastMicDir = MIC_DIR_UNKNOWN;
static DockIrState s_lastDockIr = DOCK_IR_NONE;
static bool s_lastAudioBusy = false;
static unsigned long s_lastHeartbeat = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[AUDIO_ESP] WALL-E Audio/Voice/Dock Brain starting..."));

  setSystemMode(MODE_BOOT);

  /* Initialize status LED if configured */
#if PIN_STATUS_LED >= 0
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);
#endif

  /* ESP-NOW first (receives commands from Base) */
  if (!espnowManagerInit()) {
    DEBUG_LOG("ESP-NOW init failed - retry on next boot");
  }
  eventRouterInit();

  /* Audio — optional; boot continues without if not connected */
  (void)audioPlayerInit();

  /* Sensors */
  micManagerInit();
  voiceCommandsInit();
  irDockInit();

  diagnosticsInit();

  setSystemMode(MODE_IDLE);
  DEBUG_LOG("Ready");

#if PIN_STATUS_LED >= 0
  digitalWrite(PIN_STATUS_LED, HIGH);
#endif
}

void loop() {
  unsigned long now = millis();

  /* Sensor updates */
  micManagerTick();
  irDockTick();
  audioPlayerTick();

  espnowManagerTick();

  /* Voice from dedicated UART (if used) */
  VoiceCmdId vc = voiceCommandsTick();
  if (vc != VOICE_CMD_NONE) {
    eventEmitVoiceCmd(voiceCmdToString(vc));
    setSystemMode(MODE_VOICE_COMMAND);
    audioPlayAck();
  }

  /* Update mode from audio state */
  bool busy = audioIsBusy();
  if (busy != s_lastAudioBusy) {
    s_lastAudioBusy = busy;
    if (!busy) {
      if (getSystemMode() == MODE_PLAYING_AUDIO)
        setSystemMode(MODE_IDLE);
    } else {
      if (getSystemMode() == MODE_IDLE || getSystemMode() == MODE_LISTENING)
        setSystemMode(MODE_PLAYING_AUDIO);
    }
  }

  /* Send mic telem (rate-limited in espnow_manager) */
  espnowManagerSendMicTelem(
    (uint16_t)micGetLeftLevel(),
    (uint16_t)micGetRightLevel(),
    micHasNoiseSurge() ? 1 : 0
  );

  /* Send status (mic dir, dock IR, mode, fault) — rate-limited */
  eventRouterSendStatus();

  /* Heartbeat — status packet acts as heartbeat */
  if (now - s_lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
    s_lastHeartbeat = now;
    /* Status sent above */
  }

  /* Diagnostics to Serial (USB) */
  diagnosticsTick();

#if ALIVE_PRINT_ENABLE
  /* Minimal "alive" print when no hardware — confirms loop is running */
  {
    static unsigned long s_lastAlive = 0;
    bool hasAudio = audioIsReady();
    bool hasBase = espnowManagerIsReady();
    if (!hasAudio || !hasBase) {
      if (now - s_lastAlive >= ALIVE_PRINT_INTERVAL_MS) {
        s_lastAlive = now;
        Serial.print(F("[AUDIO_ESP] "));
        Serial.print(modeToString(getSystemMode()));
        Serial.print(F(" | DFPlayer:"));
        Serial.print(hasAudio ? F("OK") : F("--"));
        Serial.print(F(" Base:"));
        Serial.print(hasBase ? F("OK") : F("--"));
        Serial.print(F(" | heap:"));
        Serial.println(ESP.getFreeHeap());
      }
    }
  }
#endif

  delay(1);
}
