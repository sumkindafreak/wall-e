#include "eve_behavior_manager.h"
#include "eyes_control.h"
#include "audio_control.h"

void eveBehaviorManagerInit(void) {
  Serial.println(F("[BOOT][EVE] behavior manager"));
}

void eveBehaviorManagerTick(void) {}

void eveBehaviorOnRemoteSound(uint8_t track) {
  Serial.printf("[AUTO][EVE] remote sound track=%u\n", (unsigned)track);
  /* Modes are project-tunable; higher = more alert / confused */
  if (track == 5u || track == 6u)
    eyesSetMode(2);
  else if (track == 4u)
    eyesSetMode(1);
  else
    eyesSetMode(1);
  eyesNotifySharedVoicebox(track == 3u || track == 4u);
  audioPlayTrack(track);
}
