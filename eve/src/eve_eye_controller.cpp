#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_controller.h"
#include "eve_eye_blink.h"
#include "eve_eye_display_driver.h"
#include "eve_single_eye_renderer.h"
#include "eve_eye_renderer.h"
#include "eve_eye_animations.h"
#include "eve_expression_state.h"
#include "eve_emotion_engine.h"
#include "eve_gaze_engine.h"
#include "eve_idle_engine.h"

#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static EveEye s_leftEye;
static EveEye s_rightEye;
static EveSingleEyeUi s_leftUi;
static EveSingleEyeUi s_rightUi;
static EveEyeUi s_legacyUi;
static bool s_dualPhysical = false;
static bool s_ready = false;
static uint32_t s_lastMs = 0;
static int s_lastExpr = -1;

static lv_obj_t* screenForPanel(EveEyePanelId panel) {
  lv_display_t* disp = eveEyeDisplayGet(panel);
  if (!disp) {
    return nullptr;
  }
  lv_display_set_default(disp);
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_screen_load(scr);
  return scr;
}

static void renderDualEye(EveEye* eye, EveSingleEyeUi* ui) {
  eveSingleEyeRendererApply(ui, &eye->smooth);
}

static void buildSharedTarget(EveEyeTarget* shared) {
  eveExpressionGetTarget(shared);

  float mgx = 0.f;
  float mgy = 0.f;
  eveEyeBlinkGetMicroGaze(&mgx, &mgy);
  shared->gazeX += (shared->gazeX + mgx * 0.55f - shared->gazeX) * 0.12f;
  shared->gazeY += (shared->gazeY + mgy * 0.55f - shared->gazeY) * 0.12f;

  float sq = eveEyeBlinkSquintOverlay();
  float wd = eveEyeBlinkWidenOverlay();
  shared->squint = fminf(1.f, shared->squint + sq * 0.35f);
  shared->eyeScaleY = fminf(1.25f, shared->eyeScaleY * (1.f + wd * 0.08f));
  shared->eyeScaleX = fminf(1.2f, shared->eyeScaleX * (1.f + wd * 0.04f));
}

void eveEyeControllerInit(void) {
  lv_init();
  eveEyeDisplayInit();
  s_dualPhysical = eveEyeDisplayIsDualPhysical();

  eveExpressionInit();
  eveEmotionInit();
  eveGazeInit();
  eveIdleInit();
  eveEyeBlinkInit();

  lv_display_t* leftDisp = eveEyeDisplayGet(EVE_EYE_PANEL_LEFT);
  if (!leftDisp) {
    Serial.println(F("[EVE_EYE] Eye Controller: LVGL display init failed"));
    return;
  }

  if (s_dualPhysical) {
    lv_obj_t* scrL = screenForPanel(EVE_EYE_PANEL_LEFT);
    eveSingleEyeRendererInit(scrL, &s_leftUi, EVE_EYE_SIDE_LEFT);
    lv_obj_t* scrR = screenForPanel(EVE_EYE_PANEL_RIGHT);
    eveSingleEyeRendererInit(scrR, &s_rightUi, EVE_EYE_SIDE_RIGHT);
    eveEyeInit(&s_leftEye, EVE_EYE_SIDE_LEFT);
    eveEyeInit(&s_rightEye, EVE_EYE_SIDE_RIGHT);
    Serial.println(F("[EVE_EYE] Eye Controller: dual physical left + right eye panels"));
  } else {
    lv_display_set_default(leftDisp);
    lv_obj_t* scr = lv_screen_active();
    if (!scr) {
      scr = lv_obj_create(nullptr);
      lv_screen_load(scr);
    }
    eveEyeRendererInit(scr, &s_legacyUi);
    eveEyeAnimationsInit(&s_legacyUi);
    Serial.println(F("[EVE_EYE] Eye Controller: legacy combined visor on left panel"));
  }

  s_lastMs = millis();
  s_ready = true;
}

EveEye* eveEyeControllerLeft(void) {
  return &s_leftEye;
}

EveEye* eveEyeControllerRight(void) {
  return &s_rightEye;
}

void eveEyeControllerApplyRequest(const EveEyeBehaviourRequest* req) {
  if (!req) {
    return;
  }
  if (req->expression != EVE_EXPR_NEUTRAL_IDLE) {
    eveExpressionRequest(req->expression, 1800);
  }
  if (req->look != EVE_GAZE_CENTER) {
    eveGazeLook(req->look, 900);
  }
  if (req->blinkRequest) {
    eveEyeBlinkTriggerRandom();
  }
  if (req->slowBlink) {
    eveEyeBlinkTriggerSlow(EVE_EYE_SIDE_LEFT);
    eveEyeBlinkTriggerSlow(EVE_EYE_SIDE_RIGHT);
  }
  if (req->doubleBlink) {
    eveEyeBlinkTriggerDouble(EVE_EYE_SIDE_LEFT);
    eveEyeBlinkTriggerDouble(EVE_EYE_SIDE_RIGHT);
  }
  if (req->leftWink) {
    eveEyeBlinkTriggerWink(EVE_EYE_SIDE_LEFT);
  }
  if (req->rightWink) {
    eveEyeBlinkTriggerWink(EVE_EYE_SIDE_RIGHT);
  }
}

void eveEyeControllerTick(uint32_t nowMs) {
  if (!s_ready) {
    return;
  }

  uint32_t dt = nowMs - s_lastMs;
  if (s_lastMs == 0 || dt > 64) {
    dt = 64;
  }
  s_lastMs = nowMs;
  float dts = dt * 0.001f;

#if EVE_FACE_DEBUG_BENCH
  eveExpressionDebugSerialPoll();
#endif

  eveEmotionTick(nowMs);
  eveGazeTick(nowMs, dts);
  eveExpressionTick(nowMs);
  eveIdleTick(nowMs);
  eveEyeBlinkTick(nowMs, dts);

  EveEyeTarget shared;
  buildSharedTarget(&shared);

  if (s_dualPhysical) {
    float lidL = eveEyeBlinkLid(EVE_EYE_SIDE_LEFT);
    float lidR = eveEyeBlinkLid(EVE_EYE_SIDE_RIGHT);

    s_leftEye.microGazeX = s_rightEye.microGazeX = 0.f;
    s_leftEye.microGazeY = s_rightEye.microGazeY = 0.f;
    float mgx = 0.f;
    float mgy = 0.f;
    eveEyeBlinkGetMicroGaze(&mgx, &mgy);
    s_leftEye.microGazeX = mgx * 0.5f;
    s_rightEye.microGazeX = mgx * 0.5f;
    s_leftEye.microGazeY = s_rightEye.microGazeY = mgy * 0.5f;

    eveEyeApplySharedTarget(&s_leftEye, &shared, lidL, shared.lid);
    eveEyeApplySharedTarget(&s_rightEye, &shared, lidR, shared.lid);
    eveEyeTick(&s_leftEye, 0.18f);
    eveEyeTick(&s_rightEye, 0.18f);

    lv_display_set_default(eveEyeDisplayGet(EVE_EYE_PANEL_LEFT));
    renderDualEye(&s_leftEye, &s_leftUi);
    lv_display_set_default(eveEyeDisplayGet(EVE_EYE_PANEL_RIGHT));
    renderDualEye(&s_rightEye, &s_rightUi);
  } else {
    static EveEyeTarget s_smooth;
    static bool s_smoothInit = false;
    if (!s_smoothInit) {
      s_smooth = shared;
      s_smooth.lidLeft = s_smooth.lidRight = 0.f;
      s_smoothInit = true;
    }
#define L(F) s_smooth.F += (shared.F - s_smooth.F) * 0.16f
    L(eyeSep);
    L(eyeScaleX);
    L(eyeScaleY);
    L(gazeX);
    L(gazeY);
    L(glowOpa);
    L(scanOpa);
    L(tiltDeg);
    L(squint);
#undef L
    s_smooth.lidLeft = eveEyeBlinkLid(EVE_EYE_SIDE_LEFT);
    s_smooth.lidRight = eveEyeBlinkLid(EVE_EYE_SIDE_RIGHT);
    float lidGoal = fminf(1.f, fmaxf(shared.lid, fmaxf(s_smooth.lidLeft, s_smooth.lidRight)));
    s_smooth.lid += (lidGoal - s_smooth.lid) * 0.42f;

    float pulse = 5.f * sinf((float)nowMs * 0.0028f);
    s_smooth.glowOpa = fminf(255.f, fmaxf(0.f, s_smooth.glowOpa + pulse * 0.04f));

    eveEyeRendererApply(&s_legacyUi, &s_smooth);
  }

  int cur = (int)eveExpressionGetCurrent();
  if (cur != s_lastExpr) {
    s_lastExpr = cur;
    Serial.print(F("[EVE_EYE] Expression -> "));
#if EVE_FACE_DEBUG_BENCH
    Serial.println(eveExpressionName((EveExpressionId)cur));
#else
    Serial.println(cur);
#endif
  }

  eveEyeDisplayTickLvgl(dt);
}

#endif /* EVE_ENABLE_EYES */
