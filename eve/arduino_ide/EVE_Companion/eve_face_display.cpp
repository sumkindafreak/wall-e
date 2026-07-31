#include "config.h"
#include "eve_face_display.h"

#if EVE_ENABLE_EYES

#include "eve_eye_controller.h"

void eveFaceDisplayInit(void) {
  eveEyeControllerInit();
}

void eveFaceDisplayTick(uint32_t nowMs) {
  eveEyeControllerTick(nowMs);
}

#else /* !EVE_ENABLE_EYES */

void eveFaceDisplayInit(void) {}
void eveFaceDisplayTick(uint32_t) {}

#endif /* EVE_ENABLE_EYES */
