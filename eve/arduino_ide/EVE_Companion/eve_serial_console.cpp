#include "eve_serial_console.h"
#include "audio_control.h"
#include "config.h"
#include "eve_expression_state.h"
#include "eve_output_ids.h"
#include "eve_routines.h"
#include "eve_desktop_companion.h"
#include "eve_status_manager.h"
#include "mic_input.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "state_machine.h"

#include <stdlib.h>
#include <string.h>

static String s_line;

static bool isOn_(const char* s) {
  return s && (strcmp(s, "on") == 0 || strcmp(s, "1") == 0 || strcmp(s, "true") == 0);
}

static void printHelp_() {
  Serial.println(F("[EVE][CLI] commands:"));
  Serial.println(F("  help | ?"));
  Serial.println(F("  status                 state + status JSON"));
  Serial.println(F("  diag                   link / stop / faults / battery / servo summary"));
  Serial.println(F("  stop                   latch STOP (cancels gestures, halts servos, stops audio)"));
  Serial.println(F("  clearstop              explicit clear of a latched stop (nothing resumes by itself)"));
  Serial.println(F("  standby on|off"));
  Serial.println(F("  routine wake|greet|scan|play|protect   (scan = animation only)"));
  Serial.println(F("  eyes <name> [ms]       e.g. eyes happy 2000 (names: neutral soft happy curious alert ...)"));
  Serial.println(F("  dock on|off|status     bench dock simulation"));
  Serial.println(F("  pose <head> <arm>      degrees, clamped to the calibrated limits in config.h"));
  Serial.println(F("  head <deg> | arm <deg>"));
  Serial.println(F("  glow <name|number|auto>"));
  Serial.println(F("  audio <track> | audio stop"));
  Serial.println(F("  personality <curiosity> <responsiveness> <activity> [comfort] [excitement] [sleepiness]"));
  Serial.println(F("  mic on|off|status"));
  Serial.println(F("  mic set <spike> <clap> <quiet> [cooldown_ms]"));
  Serial.println(F("  mic test"));
  Serial.println(F("  reset                  restart the WALL-E handshake (a latched stop stays latched)"));
}

static void printStatus_() {
  Serial.print(F("[EVE][CLI] state="));
  Serial.print(stateMachineGetStateName());
  Serial.print(F(" docked="));
  Serial.print(stateMachineIsDocked() ? F("yes") : F("no"));
  Serial.print(F(" mimic="));
  Serial.print(stateMachineIsDockMimic() ? F("yes") : F("no"));
  Serial.print(F(" peer="));
  Serial.print(stateMachineGetPeerLabel());
  Serial.print(F(" session="));
  Serial.print((unsigned long)stateMachineGetSessionId());
  Serial.print(F(" stop="));
  Serial.println(eveSafetyStopLatched() ? F("LATCHED") : F("clear"));
  Serial.print(F("[EVE][CLI] status json="));
  Serial.println(eveStatusManagerGetJSON());
  Serial.print(F("[EVE][CLI] mic json="));
  Serial.println(getMicStatusJson());
}

static void reportMove_(const char* what, bool ok) {
  Serial.printf("[EVE][CLI] %s %s\n", what, ok ? "ok" : "refused (servo off, stopped, fault or low power - see diag)");
}

static int readInt_(const char* s, int fallback) {
  return s ? atoi(s) : fallback;
}

static void handleDock_(char* arg) {
  if (!arg || strcmp(arg, "status") == 0) {
    Serial.print(F("[EVE][CLI] dock mimic="));
    Serial.println(stateMachineIsDockMimic() ? F("on") : F("off"));
    return;
  }
  if (isOn_(arg)) {
    stateMachineSetDockMimic(true, EVE_DOCK_MIMIC_FAKE_CHARGING != 0);
  } else if (strcmp(arg, "off") == 0 || strcmp(arg, "0") == 0 || strcmp(arg, "false") == 0) {
    stateMachineSetDockMimic(false, false);
  } else {
    Serial.println(F("[EVE][CLI] usage: dock on|off|status"));
  }
}

static void handlePersonality_(char* c, char* r, char* a, char* comfort, char* excitement, char* sleepiness) {
  char json[192];
  snprintf(json, sizeof(json),
           "{\"cmd\":\"personality\",\"curiosity\":%d,\"responsiveness\":%d,\"activity\":%d,"
           "\"comfort\":%d,\"excitement\":%d,\"sleepiness\":%d}",
           constrain(readInt_(c, 70), 0, 100),
           constrain(readInt_(r, 65), 0, 100),
           constrain(readInt_(a, 50), 0, 100),
           constrain(readInt_(comfort, 55), 0, 100),
           constrain(readInt_(excitement, 35), 0, 100),
           constrain(readInt_(sleepiness, 20), 0, 100));
  Serial.print(F("[EVE][CLI] personality "));
  Serial.println(eveDesktopCompanionApplyConfigJson(json) ? F("ok") : F("failed"));
}

static void handleMic_(char* arg, char* a, char* b, char* c, char* d) {
  if (!arg || strcmp(arg, "status") == 0) {
    Serial.println(getMicStatusJson());
    return;
  }
  if (strcmp(arg, "on") == 0 || strcmp(arg, "off") == 0) {
    micSetReactionsEnabled(strcmp(arg, "on") == 0);
    Serial.print(F("[EVE][CLI] mic reactions="));
    Serial.println(micReactionsEnabled() ? F("on") : F("off"));
    return;
  }
  if (strcmp(arg, "test") == 0) {
    Serial.print(F("[EVE][CLI] mic test "));
    Serial.println(eveDesktopCompanionApplyConfigJson("{\"cmd\":\"mic_test\"}") ? F("ok") : F("failed"));
    return;
  }
  if (strcmp(arg, "set") == 0) {
    EveMicSettings s = micGetSettings();
    s.spikeThreshold = a ? atof(a) : s.spikeThreshold;
    s.clapThreshold = b ? atof(b) : s.clapThreshold;
    s.quietThreshold = c ? atof(c) : s.quietThreshold;
    s.reactionCooldownMs = d ? (uint32_t)atol(d) : s.reactionCooldownMs;
    micSetSettings(s);
    Serial.println(getMicStatusJson());
    return;
  }
  Serial.println(F("[EVE][CLI] usage: mic on|off|status|test|set <spike> <clap> <quiet> [cooldown]"));
}

static void executeLine_(char* line) {
  char* cmd = strtok(line, " ");
  if (!cmd) return;
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
    printHelp_();
  } else if (strcmp(cmd, "status") == 0) {
    printStatus_();
  } else if (strcmp(cmd, "dock") == 0 || strcmp(cmd, "docktest") == 0) {
    handleDock_(strtok(nullptr, " "));
  } else if (strcmp(cmd, "stop") == 0) {
    stateMachineLocalStop(EVE_STOP_SRC_SERIAL);
    Serial.println(F("[EVE][CLI] STOP latched - type clearstop to release"));
  } else if (strcmp(cmd, "clearstop") == 0) {
    Serial.println(stateMachineLocalClearStop(EVE_STOP_SRC_SERIAL) ? F("[EVE][CLI] stop cleared")
                                                                  : F("[EVE][CLI] no stop was latched"));
  } else if (strcmp(cmd, "diag") == 0) {
    stateMachinePrintDiagnostics();
  } else if (strcmp(cmd, "standby") == 0) {
    const char* arg = strtok(nullptr, " ");
    const char* why = nullptr;
    const bool on = !(arg && (strcmp(arg, "off") == 0 || strcmp(arg, "0") == 0));
    const bool ok = stateMachineLocalStandby(on, &why);
    Serial.printf("[EVE][CLI] standby %s: %s\n", on ? "on" : "off", ok ? "ok" : (why ? why : "refused"));
  } else if (strcmp(cmd, "routine") == 0) {
    const char* name = strtok(nullptr, " ");
    const char* why = nullptr;
    const bool ok = name && stateMachineLocalRoutine(name, &why);
    Serial.printf("[EVE][CLI] routine %s: %s%s\n", name ? name : "?", ok ? "started" : (why ? why : "usage: routine <name>"),
                  (ok && name && strcmp(name, "scan") == 0) ? " (animation only - no detection)" : "");
  } else if (strcmp(cmd, "eyes") == 0) {
    const char* name = strtok(nullptr, " ");
    const int id = eveExprFromName(name);
    const int ms = readInt_(strtok(nullptr, " "), 2000);
    if (id < 0) {
      Serial.println(F("[EVE][CLI] unknown expression; e.g. neutral soft happy curious alert sleepy affection"));
    } else if (eveSafetyStopLatched()) {
      Serial.println(F("[EVE][CLI] refused: stop latched"));
    } else {
      eveExpressionRequest((EveExpressionId)id, (uint32_t)constrain(ms, 100, 30000));
      Serial.println(F("[EVE][CLI] eyes ok"));
    }
  } else if (strcmp(cmd, "pose") == 0) {
    const int head = readInt_(strtok(nullptr, " "), EVE_SERVO_HEAD_REST_DEG);
    const int arm = readInt_(strtok(nullptr, " "), EVE_SERVO_ARM_REST_DEG);
    const bool okH = servoMoveTo(EVE_SERVO_HEAD_PAN, (int16_t)constrain(head, -360, 360), EVE_SERVO_DEFAULT_DPS);
    const bool okA = servoMoveTo(EVE_SERVO_RIGHT_ARM, (int16_t)constrain(arm, -360, 360), EVE_SERVO_DEFAULT_DPS);
    reportMove_("pose", okH || okA);
  } else if (strcmp(cmd, "head") == 0) {
    const int d = readInt_(strtok(nullptr, " "), EVE_SERVO_HEAD_REST_DEG);
    reportMove_("head", servoMoveTo(EVE_SERVO_HEAD_PAN, (int16_t)constrain(d, -360, 360), EVE_SERVO_DEFAULT_DPS));
  } else if (strcmp(cmd, "arm") == 0) {
    const int d = readInt_(strtok(nullptr, " "), EVE_SERVO_ARM_REST_DEG);
    reportMove_("arm", servoMoveTo(EVE_SERVO_RIGHT_ARM, (int16_t)constrain(d, -360, 360), EVE_SERVO_DEFAULT_DPS));
  } else if (strcmp(cmd, "glow") == 0) {
    const char* arg = strtok(nullptr, " ");
    int p = -1;
    if (arg && strcmp(arg, "auto") != 0) p = (arg[0] >= '0' && arg[0] <= '9') ? atoi(arg) : eveGlowFromName(arg);
    if (arg && strcmp(arg, "auto") != 0 && (p < 0 || p >= EVE_GLOW_COUNT)) {
      Serial.println(F("[EVE][CLI] unknown glow; names: idle linked waiting critical standby interact escort low_power charging full fault stopped scan play protect off auto"));
    } else {
      neopixelSetOperator((int8_t)p);
      Serial.printf("[EVE][CLI] glow %s\n", p < 0 ? "auto" : eveGlowName((uint8_t)p));
    }
  } else if (strcmp(cmd, "audio") == 0) {
    const char* arg = strtok(nullptr, " ");
    if (arg && strcmp(arg, "stop") == 0) {
      audioStop();
      Serial.println(F("[EVE][CLI] audio stopped"));
    } else {
      const bool ok = audioPlayTrack((uint8_t)constrain(readInt_(arg, 1), 0, 255));
      Serial.println(ok ? F("[EVE][CLI] audio playing") : F("[EVE][CLI] audio not played (disabled, missing file or stopped)"));
    }
  } else if (strcmp(cmd, "personality") == 0) {
    handlePersonality_(strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "),
                       strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "));
  } else if (strcmp(cmd, "mic") == 0) {
    handleMic_(strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "),
               strtok(nullptr, " "), strtok(nullptr, " "));
  } else if (strcmp(cmd, "reset") == 0) {
    stateMachineLocalReset();
    Serial.println(eveSafetyStopLatched() ? F("[EVE][CLI] handshake restarted (stop still latched)")
                                          : F("[EVE][CLI] handshake restarted"));
  } else {
    Serial.println(F("[EVE][CLI] unknown command; type help"));
  }
}

void eveSerialConsoleInit(void) {
#if EVE_ENABLE_SERIAL_CONSOLE
  s_line.reserve(80);
  Serial.println(F("[EVE][CLI] ready; type help"));
#endif
}

void eveSerialConsoleTick(void) {
#if EVE_ENABLE_SERIAL_CONSOLE
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (s_line.length() < 120) s_line += c;
      continue;
    }
    s_line.trim();
    s_line.toLowerCase();
    if (s_line.length() > 0) {
      char buf[128];
      s_line.toCharArray(buf, sizeof(buf));
      executeLine_(buf);
    }
    s_line = "";
  }
#endif
}

