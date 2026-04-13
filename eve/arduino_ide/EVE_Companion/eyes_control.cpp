#include "eyes_control.h"
#include "config.h"
#include "eve_expression_state.h"
#include "eve_face_display.h"

void eyesInit(void) {
#if EVE_ENABLE_EYES
  eveFaceDisplayInit();
#else
  Serial.println(F("[EVE][EYES] disabled — set EVE_ENABLE_EYES=1 and TFT pins in config.h"));
#endif
}

void eyesTick(void) {
#if EVE_ENABLE_EYES
  eveFaceDisplayTick(millis());
#endif
}

void eyesSetMode(uint8_t mode) {
#if EVE_ENABLE_EYES
  eveExpressionSetLegacyMode(mode);
#endif
  (void)mode;
}

void eyesNotifyWallEConnected(void) {
  eveExpressionNotifyWallEConnected();
}

void eyesNotifyWallEDisconnected(void) {
  eveExpressionNotifyWallEDisconnected();
}

void eyesNotifyDockingState(bool docked, bool charging) {
  eveExpressionSetDockedCharging(docked, charging);
}

void eyesNotifyRecordFailure(void) {
  eveExpressionNotifyRecordFailure();
}

void eyesNotifySharedVoicebox(bool active) {
  eveExpressionNotifySharedVoicebox(active);
}
