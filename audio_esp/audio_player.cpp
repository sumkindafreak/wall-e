/**
 * audio_player.cpp — DFPlayer Mini implementation
 */
#include "audio_player.h"

HardwareSerial DFPlayerSerial(1);
DFRobotDFPlayerMini dfplayer;
uint8_t g_audioPriority = 0;
static bool s_playing = false;
static unsigned long s_playStartMs = 0;
static bool s_ok = false;  /* true = DFPlayer present and ready */
#define PLAY_TIMEOUT_MS 60000  /* Max track length */

bool audioPlayerInit() {
  s_ok = false;
  DFPlayerSerial.begin(DFPLAYER_BAUD, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);
  delay(300);
  if (!dfplayer.begin(DFPlayerSerial, true, true)) {
    return false;  /* No DFPlayer — boot continues without audio */
  }
  dfplayer.volume(20);
  dfplayer.EQ(DFPLAYER_EQ_NORMAL);
  g_audioPriority = 0;
  s_ok = true;
  return true;
}

void audioPlayTrack(uint8_t trackId, uint8_t priority) {
  if (!s_ok) return;
  if (priority < g_audioPriority && audioIsBusy()) {
    DEBUG_AUDIO_LOG("Skip track %u (priority %u < %u)", trackId, priority, g_audioPriority);
    return;
  }
  g_audioPriority = priority;
  dfplayer.play(trackId);
  s_playing = true;
  s_playStartMs = millis();
  DEBUG_AUDIO_LOG("Play track %u", trackId);
}

void audioPlayStartup()  { audioPlayTrack(TRACK_STARTUP, PRIO_NORMAL); }
void audioPlayHello()   { audioPlayTrack(TRACK_HELLO,   PRIO_NORMAL); }
void audioPlayAck()     { audioPlayTrack(TRACK_ACK,     PRIO_ACK); }
void audioPlayError()   { audioPlayTrack(TRACK_ERROR,   PRIO_ERROR); }
void audioPlayCurious() { audioPlayTrack(TRACK_CURIOUS, PRIO_NORMAL); }
void audioPlaySleep()   { audioPlayTrack(TRACK_SLEEP,   PRIO_NORMAL); }
void audioPlayWake()    { audioPlayTrack(TRACK_WAKE,    PRIO_NORMAL); }
void audioPlayDockGuide() { audioPlayTrack(TRACK_DOCK_GUIDE, PRIO_NORMAL); }
void audioPlayStop()    { audioPlayTrack(TRACK_STOP,    PRIO_VOICE_CMD); }

void audioStop() {
  if (!s_ok) return;
  dfplayer.stop();
  g_audioPriority = 0;
  s_playing = false;
  DEBUG_AUDIO_LOG("Stop");
}

void audioSetVolume(uint8_t vol) {
  if (!s_ok) return;
  if (vol > 30) vol = 30;
  dfplayer.volume(vol);
  DEBUG_AUDIO_LOG("Volume %u", vol);
}

bool audioIsReady() { return s_ok; }

bool audioIsBusy() {
  if (!s_ok || !s_playing) return false;
  /* readState: 1 or 513 = playing; 2 or 512 = stopped (library varies) */
  int st = dfplayer.readState();
  if (st == 2 || st == 512 || st == 0 || st == -1) {
    s_playing = false;
    g_audioPriority = 0;
    return false;
  }
  /* Timeout fallback */
  if (millis() - s_playStartMs > PLAY_TIMEOUT_MS) {
    s_playing = false;
    g_audioPriority = 0;
    return false;
  }
  return true;
}

void audioPlayerTick() {
  /* Keep s_playing in sync when readState indicates done */
  (void)audioIsBusy();
}
