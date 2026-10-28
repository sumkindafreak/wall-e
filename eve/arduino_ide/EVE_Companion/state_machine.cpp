/**
 * EVE state machine — Arduino glue.
 *
 * The decisions (link health, command validation, modes, stop latch) live in plain-C++ modules
 * that are unit-tested on a PC:
 *   eve_control_core  — handshake, heartbeats, link supervision, commands, mode arbitration
 *   eve_safety        — latched stop + fault register
 *   eve_routines      — timed, cancellable expressions
 * This file connects them to the real outputs (eyes, glow, audio, servos) and to the
 * desktop-companion / spatial-awareness modules that already existed.
 */
#include "state_machine.h"

#include <string.h>

#include "audio_control.h"
#include "battery_monitor.h"
#include "config.h"
#include "eve_attachment_manager.h"
#include "eve_behavior_manager.h"
#include "eve_control_core.h"
#include "eve_desktop_companion.h"
#include "eve_expression_state.h"
#include "eve_output_ids.h"
#include "eve_pin_check.h"
#include "eve_routines.h"
#include "eve_spatial_awareness.h"
#include "eyes_control.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "uart_link.h"
#if EVE_ENABLE_EYES
#include "eve_emotion_engine.h"
#endif

static bool s_initDone = false;

// =============================================================================
// Shared helpers
// =============================================================================
static void logLine(const char* line) { Serial.println(line); }

/** Base glow for a mode (operator / routine / stop layers sit on top, see neopixel_control.h). */
static uint8_t glowForMode(EveMode m) {
  switch (m) {
    case EVE_MODE_WAIT_ATTACH: return EVE_GLOW_WAITING;
    case EVE_MODE_IDLE:
    case EVE_MODE_ATTACHED: return EVE_GLOW_LINKED;
    case EVE_MODE_ESCORT: return EVE_GLOW_ESCORT;
    case EVE_MODE_INTERACT: return EVE_GLOW_INTERACT;
    case EVE_MODE_LOW_POWER: return EVE_GLOW_LOW_POWER;
    case EVE_MODE_ERROR: return EVE_GLOW_FAULT;
    case EVE_MODE_SLEEP: return EVE_GLOW_STANDBY_BEACON;
    case EVE_MODE_DOCKED:
      if (eveControlDockMimicCharging() || eveBatteryCharging() == 1) return eveBatteryFull() ? EVE_GLOW_FULL : EVE_GLOW_CHARGING;
      return EVE_GLOW_IDLE;
    default: return EVE_GLOW_WAITING;
  }
}

/** Charging as far as EVE can actually tell (dock mimic fakes it on the bench). */
static bool dockCharging(void) { return eveControlDockMimicCharging() || eveBatteryCharging() == 1; }

// =============================================================================
// Safety hooks: what a STOP (or a blocking fault) does to the hardware
// =============================================================================
static void haltOutputs(void) {
  eveRoutineCancel();   /* active + pending gesture steps */
  servoHaltAll(true);   /* freeze, then HOLD or RELEASE per EVE_SERVO_STOP_POLICY */
  audioStop();          /* explicit audio stop */
}

// =============================================================================
// Routine hooks
// =============================================================================
static void routineExpression(uint8_t exprId, uint32_t holdMs) { eveExpressionRequest((EveExpressionId)exprId, holdMs); }

static void routineGlow(uint8_t pattern, uint32_t ms) { neopixelSetOverlay(pattern, ms); }

static void routineSound(uint8_t track) { audioPlayTrack(track); }

static bool routineServo(uint8_t ch, int16_t deg, uint16_t dps) { return servoMoveTo(ch, deg, dps); }

// =============================================================================
// Control-core hooks
// =============================================================================
static bool hookSend(uint8_t type, const char* json) { return uartLinkSendJson((EveMsgType)type, json); }

static uint32_t hookHeap(void) { return ESP.getFreeHeap(); }

static uint8_t hookOutputs(void) {
  uint8_t o = 0;
#if EVE_ENABLE_EYES
  if (EVE_FACE_GFX_READY) o |= EVE_OUT_EYES;
#endif
  if (servoAvailable(EVE_SERVO_HEAD_PAN) || servoAvailable(EVE_SERVO_RIGHT_ARM)) o |= EVE_OUT_SERVOS;
  if (neopixelStripActive()) o |= EVE_OUT_GLOW_STRIP;
  if (neopixelOnboardActive()) o |= EVE_OUT_ONBOARD_RGB;
  if (audioIsReady()) o |= EVE_OUT_AUDIO;
#if EVE_ENABLE_TOF
  if (EVE_I2C_SDA >= 0 && EVE_I2C_SCL >= 0) o |= EVE_OUT_TOF;
#endif
  if (eveAttachmentPresence() >= 0) o |= EVE_OUT_PRESENCE;
  if (eveBatteryHardwareEnabled()) o |= EVE_OUT_BATTERY;
  return o;
}

static int8_t hookPresent(void) { return eveAttachmentPresence(); }

static uint32_t hookRxErrors(void) { return uartLinkRxErrors(); }

static bool hookRemoteSound(uint8_t track) { return eveBehaviorOnRemoteSound(track); }

static void hookEyesLegacy(uint8_t mode) { eyesSetMode(mode); }

static void hookGlowOperator(int8_t pattern) { neopixelSetOperator(pattern); }

static bool hookBusCommand(const char* json) { return eveDesktopCompanionApplyConfigJson(json); }

static void hookModeChanged(EveMode from, EveMode to) {
  neopixelSetOperator(-1); /* operator glow lasts until the next mode change */
  neopixelSetBase(glowForMode(to));
  if (to == EVE_MODE_DOCKED) {
    eyesNotifyDockingState(true, dockCharging());
  } else if (from == EVE_MODE_DOCKED) {
    eyesNotifyDockingState(false, false);
  }
  if (to == EVE_MODE_ATTACHED) eyesSetMode(1); /* calm / social, as before */
  if (to == EVE_MODE_ERROR) {
    Serial.printf("[EVE][SM] ERROR: blocking fault 0x%04x - motion disabled until the cause is fixed\n",
                  (unsigned)eveSafetyFaults());
  }
}

static void hookLink(bool up, bool timedOut) {
  if (up) {
    eyesNotifyWallEConnected();
  } else {
    eyesNotifyWallEDisconnected();
    if (timedOut) Serial.println(F("[EVE][LINK] no valid frames from WALL-E - waiting again"));
  }
}

// =============================================================================
// Boot-time configuration check
// =============================================================================
static void checkPinConfig(void) {
  EvePinUse uses[32];
  size_t n = 0;
  auto add = [&](const char* name, int pin, uint8_t group) {
    if (pin >= 0 && n < sizeof(uses) / sizeof(uses[0])) uses[n++] = {name, pin, group};
  };
  add("UART TX to WALL-E", EVE_UART_TX_PIN, EVE_PINGRP_EXCLUSIVE);
  add("UART RX from WALL-E", EVE_UART_RX_PIN, EVE_PINGRP_EXCLUSIVE);
  add("presence pin", EVE_PRESENT_PIN, EVE_PINGRP_EXCLUSIVE);
#if EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
  add("INA219 SDA", EVE_INA219_SDA, EVE_PINGRP_I2C_SDA);
  add("INA219 SCL", EVE_INA219_SCL, EVE_PINGRP_I2C_SCL);
#elif EVE_ENABLE_BATTERY_MONITOR
  add("battery ADC", EVE_BAT_ADC_PIN, EVE_PINGRP_EXCLUSIVE);
  add("current ADC", EVE_CUR_ADC_PIN, EVE_PINGRP_EXCLUSIVE);
#endif
#if EVE_ONBOARD_RGB_HEARTBEAT
  add("onboard RGB", EVE_ONBOARD_RGB_PIN, EVE_PINGRP_EXCLUSIVE);
#endif
#if EVE_ENABLE_NEOPIXEL
  add("NeoPixel strip", EVE_NEOPIXEL_PIN, EVE_PINGRP_EXCLUSIVE);
#endif
#if EVE_ENABLE_SERVOS
  add("head servo", EVE_SERVO_L_PIN, EVE_PINGRP_EXCLUSIVE);
  add("arm servo", EVE_SERVO_R_PIN, EVE_PINGRP_EXCLUSIVE);
#endif
#if EVE_ENABLE_TOF
  add("ToF SDA", EVE_I2C_SDA, EVE_PINGRP_I2C_SDA);
  add("ToF SCL", EVE_I2C_SCL, EVE_PINGRP_I2C_SCL);
  add("ToF XSHUT", EVE_TOF_XSHUT_SECOND, EVE_PINGRP_EXCLUSIVE);
#endif
#if EVE_ENABLE_EYES
  add("eye MOSI", EVE_TFT_SPI_MOSI, EVE_PINGRP_SPI_MOSI);
  add("eye SCK", EVE_TFT_SPI_SCK, EVE_PINGRP_SPI_SCK);
  add("eye MISO", EVE_TFT_SPI_MISO, EVE_PINGRP_SPI_MISO);
  add("left eye CS", EVE_LEFT_EYE_CS, EVE_PINGRP_EXCLUSIVE);
  add("right eye CS", EVE_RIGHT_EYE_CS, EVE_PINGRP_EXCLUSIVE);
  add("eye DC", EVE_TFT_DC, EVE_PINGRP_EXCLUSIVE);
  add("eye RST", EVE_TFT_RST, EVE_PINGRP_EXCLUSIVE);
#endif
  /* The SD card is mounted whenever its pins are set (asset manager), audio or not. */
  add("SD MOSI", EVE_SD_SPI_MOSI, EVE_PINGRP_SPI_MOSI);
  add("SD SCK", EVE_SD_SPI_SCK, EVE_PINGRP_SPI_SCK);
  add("SD MISO", EVE_SD_SPI_MISO, EVE_PINGRP_SPI_MISO);
  add("SD CS", EVE_SD_SPI_CS, EVE_PINGRP_EXCLUSIVE);
#if EVE_ENABLE_AUDIO
  add("I2S BCLK", EVE_I2S_BCLK_PIN, EVE_PINGRP_EXCLUSIVE);
  add("I2S LRCK", EVE_I2S_LRCK_PIN, EVE_PINGRP_EXCLUSIVE);
  add("I2S DOUT", EVE_I2S_DOUT_PIN, EVE_PINGRP_EXCLUSIVE);
#endif

  char msg[128];
  int problems = evePinCheck(uses, n, msg, sizeof(msg));
#if EVE_ENABLE_TOF && EVE_ENABLE_BATTERY_MONITOR && EVE_BATTERY_INA219
  /* Both use the one Wire bus: different pins would re-route it and lose the INA219. */
  if (EVE_I2C_SDA != EVE_INA219_SDA || EVE_I2C_SCL != EVE_INA219_SCL) {
    if (problems == 0) snprintf(msg, sizeof(msg), "ToF and INA219 share Wire: set EVE_I2C_SDA/SCL = EVE_INA219_SDA/SCL");
    problems++;
  }
#endif
  if (problems > 0) {
    Serial.printf("[EVE][CONFIG] %d pin problem(s): %s\n", problems, msg);
    Serial.println(F("[EVE][CONFIG] blocking fault raised - motion stays disabled until config.h is fixed"));
    eveSafetySetFault(EVE_FAULT_PIN_CONFLICT, true);
  } else {
    Serial.printf("[EVE][CONFIG] %u configured pins checked - no conflicts\n", (unsigned)n);
  }

#if EVE_ENABLE_EYES
  if (!EVE_FACE_GFX_READY) {
    Serial.println(F("[EVE][CONFIG] eyes enabled but TFT pins are not set - eye panels stay dark"));
    eveSafetySetFault(EVE_FAULT_EYES, true);
  }
#endif
}

// =============================================================================
// Public API
// =============================================================================
void stateMachineEarlyInit(void) {
  static const EveSafetyHooks safetyHooks = {haltOutputs, logLine};
  eveSafetyInit(&safetyHooks);
  static const EveRoutineHooks routineHooks = {routineExpression, routineGlow, routineSound,
                                               routineServo,      eveSafetyRoutinesAllowed, logLine};
  eveRoutinesInit(&routineHooks);
  checkPinConfig();
}

void stateMachineInit(void) {
  if (s_initDone) {
    /* Old callers used stateMachineInit() as "reset": keep that meaning, never clear the stop. */
    stateMachineLocalReset();
    return;
  }
  s_initDone = true;
  EveControlHooks h;
  memset(&h, 0, sizeof(h));
  h.send = hookSend;
  h.battery = eveBatteryFillReport;
  h.freeHeap = hookHeap;
  h.outputs = hookOutputs;
  h.present = hookPresent;
  h.rxErrors = hookRxErrors;
  h.servoAvailable = servoAvailable;
  h.servoLimits = servoGetLimits;
  h.servoMove = servoMoveTo;
  h.haltMotion = servoHaltAll;
  h.remoteSound = hookRemoteSound;
  h.stopSound = audioStop;
  h.eyesExpression = routineExpression;
  h.eyesLegacyMode = hookEyesLegacy;
  h.glowOperator = hookGlowOperator;
  h.busCommand = hookBusCommand;
  h.onModeChanged = hookModeChanged;
  h.onLink = hookLink;
  h.log = logLine;
  eveDesktopCompanionInit();
  eveControlInit(&h, millis());
  neopixelSetBase(glowForMode(eveControlMode()));
#if EVE_ENABLE_DOCK_MIMIC_TEST && EVE_DOCK_MIMIC_ON_BOOT
  stateMachineSetDockMimic(true, EVE_DOCK_MIMIC_FAKE_CHARGING != 0);
#endif
}

void stateMachineTick(void) {
  const uint32_t now = millis();

  // ---- Battery: hand trustworthy changes to the core --------------------------------------------
  static EveBatLevel s_lvl = EVE_BATLVL_UNKNOWN;
  static int8_t s_chg = -1;
  static bool s_full = false;
  static bool s_batSent = false;
  const EveBatLevel lvl = eveBatteryLevel();
  const int8_t chg = eveBatteryCharging();
  const bool full = eveBatteryFull();
  if (!s_batSent || lvl != s_lvl || chg != s_chg || full != s_full) {
    s_batSent = true;
    s_lvl = lvl;
    s_chg = chg;
    s_full = full;
    eveControlSetBattery(lvl, chg, full, now);
  }

  eveControlTick(now);

  const EveMode mode = eveControlMode();
  neopixelSetBase(glowForMode(mode)); /* docked charging / full can change without a mode change */
  eveExpressionSetSearchingWallE(mode == EVE_MODE_WAIT_ATTACH);

  // ---- Docked eye state follows measured charging -----------------------------------------------
  static bool s_prevDockChg = false;
  const bool docked = (mode == EVE_MODE_DOCKED);
  const bool chgNow = docked && dockCharging();
  if (docked && chgNow != s_prevDockChg) eyesNotifyDockingState(true, chgNow);
  s_prevDockChg = chgNow;

  // ---- Behaviour flags for spatial awareness / emotion engine (unchanged meaning) ----------------
  {
    uint8_t sf = 0;
    const bool sleepNow = (mode == EVE_MODE_SLEEP);
#if EVE_ENABLE_EYES
    static bool s_prevSleepEmotion = false;
    if (sleepNow != s_prevSleepEmotion) {
      eveEmotionNotifySleep(sleepNow);
      s_prevSleepEmotion = sleepNow;
    }
#endif
    if (sleepNow) sf |= EVE_SPATIAL_FLAG_SLEEP;
    if (mode == EVE_MODE_LOW_POWER) sf |= EVE_SPATIAL_FLAG_LOW_BATTERY;
    if (mode == EVE_MODE_ESCORT || mode == EVE_MODE_INTERACT) sf |= EVE_SPATIAL_FLAG_ALERT;
    if (eveAttachmentIsAttached()) sf |= EVE_SPATIAL_FLAG_ATTACHED;
#if EVE_ENABLE_EYES
    if (eveExpressionGetCurrent() == EVE_EXPR_CONFUSED) sf |= EVE_SPATIAL_FLAG_CONFUSED;
#endif
    eveSpatialSetBehaviorFlags(sf);
  }

  // ---- Desktop companion only while docked -------------------------------------------------------
  eveDesktopCompanionSetActive(docked, chgNow);
  if (docked) eveDesktopCompanionTick(now);
}

void stateMachineOnUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
#if EVE_SERIAL_VERBOSE_LOGS
  Serial.printf("[EVE][LINK] RX %s seq=%u len=%u\n", eveLinkMsgName(type), (unsigned)seq, (unsigned)len);
#endif
  eveControlOnFrame(type, payload, len, seq, millis());
}

uint32_t stateMachineGetSessionId(void) { return eveControlSession(); }

bool stateMachineIsDocked(void) { return eveControlMode() == EVE_MODE_DOCKED; }

const char* stateMachineGetStateName(void) { return eveControlModeName(eveControlMode()); }

void stateMachineSetDockMimic(bool enabled, bool fakeCharging) {
#if EVE_ENABLE_DOCK_MIMIC_TEST
  eveControlSetDockMimic(enabled, fakeCharging, millis());
#else
  (void)enabled;
  (void)fakeCharging;
  Serial.println(F("[EVE][SM] dock mimic disabled in config"));
#endif
}

bool stateMachineIsDockMimic(void) { return eveControlDockMimic(); }

bool stateMachineAllowsCompanionUart(void) { return eveControlAllowsCompanionUart(); }

const char* stateMachineGetPeerLabel(void) { return eveControlPeer(); }

void stateMachineLocalStop(EveStopSource src) { eveControlLocalStop(src, millis()); }

bool stateMachineLocalClearStop(EveStopSource src) { return eveControlLocalClearStop(src, millis()); }

void stateMachineLocalReset(void) { eveControlLocalReset(millis()); }

bool stateMachineLocalRoutine(const char* name, const char** whyNot) {
  const EveRoutineId id = eveRoutineFromName(name);
  if (id == EVE_ROUTINE_NONE) {
    if (whyNot) *whyNot = "unknown routine";
    return false;
  }
  return eveControlLocalRoutine(id, millis(), whyNot);
}

bool stateMachineLocalStandby(bool on, const char** whyNot) { return eveControlLocalStandby(on, millis(), whyNot); }

size_t stateMachineStatusJson(char* out, size_t cap) { return eveControlStatusJson(out, cap, millis()); }

void stateMachinePrintDiagnostics(void) {
  const uint32_t now = millis();
  const EveControlStats* st = eveControlStats();
  const EveFrameDecoder::Stats& ds = uartLinkStats();
  Serial.printf("[EVE][DIAG] mode=%s req=%s linked=%s peer=%s session=%lu link_age=%lums supervision=%s\n",
                eveControlModeName(eveControlMode()), eveControlModeName(eveControlRequestedMode()),
                eveControlLinked() ? "yes" : "no", eveControlPeer(), (unsigned long)eveControlSession(),
                (unsigned long)eveControlLinkAgeMs(now), eveControlPeerHeartbeats() ? "heartbeat" : "legacy");
  Serial.printf("[EVE][DIAG] stop=%s (src=%s, latched %lu times) faults=0x%04x power_inhibit=%s routine=%s\n",
                eveSafetyStopLatched() ? "LATCHED" : "clear", eveSafetyStopSourceName(eveSafetyStopSource()),
                (unsigned long)eveSafetyStopCount(), (unsigned)eveSafetyFaults(), eveSafetyPowerInhibit() ? "yes" : "no",
                eveRoutineName(eveRoutineActive()));
  Serial.printf("[EVE][DIAG] frames ok=%lu crc_err=%lu ver_err=%lu len_err=%lu echo=%lu | cmds ok=%lu rejected=%lu (last: %s)\n",
                (unsigned long)ds.ok, (unsigned long)ds.crcErr, (unsigned long)ds.versionErr, (unsigned long)ds.lengthErr,
                (unsigned long)st->echoIgnored, (unsigned long)st->accepted, (unsigned long)st->rejected, st->lastReject);
  Serial.printf("[EVE][DIAG] battery %s %.2fV %+.2fA %d%% charging=%d full=%s (%s)\n",
                eveBatteryHardwareEnabled() ? "sensor" : "no-sensor", (double)eveBatteryVoltage(),
                (double)eveBatteryCurrentA(), eveBatteryPercent(), (int)eveBatteryCharging(),
                eveBatteryFull() ? "yes" : "no", eveBatteryInvalidReason());
  Serial.printf("[EVE][DIAG] servos: head cmd=%d tgt=%d %s | arm cmd=%d tgt=%d %s (cmd -1 = never commanded)\n",
                servoGetCommandedDeg(EVE_SERVO_HEAD_PAN), servoGetTargetDeg(EVE_SERVO_HEAD_PAN),
                servoAvailable(EVE_SERVO_HEAD_PAN) ? (servoOutputActive(EVE_SERVO_HEAD_PAN) ? "pulsing" : "silent") : "off",
                servoGetCommandedDeg(EVE_SERVO_RIGHT_ARM), servoGetTargetDeg(EVE_SERVO_RIGHT_ARM),
                servoAvailable(EVE_SERVO_RIGHT_ARM) ? (servoOutputActive(EVE_SERVO_RIGHT_ARM) ? "pulsing" : "silent") : "off");
  Serial.printf("[EVE][DIAG] glow=%s audio=%s presence=%d outputs=0x%02x\n", eveGlowName(neopixelCurrentPattern()),
                audioIsReady() ? "ready" : "off", (int)eveAttachmentPresence(), (unsigned)hookOutputs());
}
