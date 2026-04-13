#include "dfplayer_character_controls.h"
#include "audio_player.h"
#include "voicebox_router.h"
#include "config.h"

void charControlsInit(void) {}

void charControlsPlayButton(void) {
  Serial.println(F("[BTN] PLAY pressed"));
  audioPlayTrack(TRACK_CHAR_PLAY, PRIO_MENU);
  if (voiceboxRouterIsSharedActive())
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_PLAY_ACK);
}

void charControlsStopButton(void) {
  Serial.println(F("[BTN] STOP pressed"));
  audioPlayTrack(TRACK_CHAR_STOP, PRIO_MENU);
  if (voiceboxRouterIsSharedActive())
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_STOP_SETTLE);
}

void charControlsRewindButton(void) {
  Serial.println(F("[BTN] REWIND pressed"));
  audioPlayTrack(TRACK_CHAR_REWIND, PRIO_MENU);
  if (voiceboxRouterIsSharedActive())
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_REWIND_CONFUSED);
}

void charControlsRecordButton(void) {
  Serial.println(F("[BTN] RECORD requested"));
  Serial.println(F("[AUDIO] Record function unavailable"));
  Serial.println(F("[AUTO] Triggering confused reaction (character)"));
  audioPlayTrack(TRACK_CHAR_RECORD_FAIL, PRIO_ERROR);
  if (voiceboxRouterIsSharedActive())
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_RECORD_REACT);
}
