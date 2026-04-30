#include "eve_serial_console.h"
#include "audio_control.h"
#include "config.h"
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
  Serial.println(F("  status"));
  Serial.println(F("  dock on|off|status"));
  Serial.println(F("  pose <head 45-135> <arm 0-180>"));
  Serial.println(F("  head <deg> | arm <deg>"));
  Serial.println(F("  glow <pattern>"));
  Serial.println(F("  audio <track>"));
  Serial.println(F("  personality <curiosity> <responsiveness> <activity> [comfort] [excitement] [sleepiness]"));
  Serial.println(F("  mic on|off|status"));
  Serial.println(F("  mic set <spike> <clap> <quiet> [cooldown_ms]"));
  Serial.println(F("  mic test"));
  Serial.println(F("  reset"));
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
  Serial.println((unsigned long)stateMachineGetSessionId());
  Serial.print(F("[EVE][CLI] status json="));
  Serial.println(eveStatusManagerGetJSON());
  Serial.print(F("[EVE][CLI] mic json="));
  Serial.println(getMicStatusJson());
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
  } else if (strcmp(cmd, "pose") == 0) {
    int head = constrain(readInt_(strtok(nullptr, " "), 90), 45, 135);
    int arm = constrain(readInt_(strtok(nullptr, " "), 90), 0, 180);
    servoSetHeadPanTarget(head);
    servoSetRightArmTarget(arm);
    Serial.println(F("[EVE][CLI] pose ok"));
  } else if (strcmp(cmd, "head") == 0) {
    servoSetHeadPanTarget(constrain(readInt_(strtok(nullptr, " "), 90), 45, 135));
    Serial.println(F("[EVE][CLI] head ok"));
  } else if (strcmp(cmd, "arm") == 0) {
    servoSetRightArmTarget(constrain(readInt_(strtok(nullptr, " "), 90), 0, 180));
    Serial.println(F("[EVE][CLI] arm ok"));
  } else if (strcmp(cmd, "glow") == 0) {
    neopixelSetPattern((uint8_t)constrain(readInt_(strtok(nullptr, " "), 1), 0, 255));
    Serial.println(F("[EVE][CLI] glow ok"));
  } else if (strcmp(cmd, "audio") == 0) {
    audioPlayTrack((uint8_t)constrain(readInt_(strtok(nullptr, " "), 1), 0, 255));
    Serial.println(F("[EVE][CLI] audio ok"));
  } else if (strcmp(cmd, "personality") == 0) {
    handlePersonality_(strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "),
                       strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "));
  } else if (strcmp(cmd, "mic") == 0) {
    handleMic_(strtok(nullptr, " "), strtok(nullptr, " "), strtok(nullptr, " "),
               strtok(nullptr, " "), strtok(nullptr, " "));
  } else if (strcmp(cmd, "reset") == 0) {
    stateMachineInit();
    Serial.println(F("[EVE][CLI] state reset"));
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

