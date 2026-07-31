#include "eve_behavior_manager.h"
#include "eyes_control.h"
#include "audio_control.h"
#include "eve_emotion_engine.h"
#include "eve_expression_state.h"
#include "config.h"

void eveBehaviorManagerInit(void) {
  Serial.println(F("[BOOT][EVE] behavior manager"));
}

void eveBehaviorManagerTick(void) {}

void eveBehaviorOnRemoteSound(uint8_t track) {
  Serial.printf("[AUTO][EVE] remote sound track=%u\n", (unsigned)track);
  /* Eyes-only voice reaction: widen / soften lids / blink — no mouth animation. */
  if (track == 5u || track == 6u)
    eyesSetMode(2);
  else if (track == 4u)
    eyesSetMode(1);
  else
    eyesSetMode(1);
  eyesNotifySharedVoicebox(track == 3u || track == 4u);
#if EVE_ENABLE_EYES
  eveEmotionNotifyVoice(true, track);
  if (track == 5u || track == 6u) {
    eveExpressionRequest(EVE_EXPR_ALERT, 1600);
  } else {
    eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 2200);
  }
#endif
  audioPlayTrack(track);
}
