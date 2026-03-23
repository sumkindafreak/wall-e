/**
 * voice_commands.h — Parse serial voice input, debounce, emit VOICE_CMD events
 */
#ifndef AUDIO_ESP_VOICE_COMMANDS_H
#define AUDIO_ESP_VOICE_COMMANDS_H

#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include <Arduino.h>

/* Voice command IDs (emitted to event router) */
enum VoiceCmdId {
  VOICE_CMD_NONE = 0,
  VOICE_CMD_STOP,
  VOICE_CMD_COME_HERE,
  VOICE_CMD_GO_HOME,
  VOICE_CMD_SLEEP,
  VOICE_CMD_WAKE
};

/* Initialize voice input */
void voiceCommandsInit();

/* Parse a line (e.g. "VC:STOP" or "STOP"). Returns VOICE_CMD_NONE if none/cooldown. */
VoiceCmdId voiceCommandsParse(const char* line);

/* Call every loop when using dedicated voice UART. Returns VOICE_CMD_NONE if not used. */
VoiceCmdId voiceCommandsTick();

/* Convert to protocol string */
inline const char* voiceCmdToString(VoiceCmdId id) {
  switch (id) {
    case VOICE_CMD_STOP:     return "STOP";
    case VOICE_CMD_COME_HERE: return "COME_HERE";
    case VOICE_CMD_GO_HOME:  return "GO_HOME";
    case VOICE_CMD_SLEEP:    return "SLEEP";
    case VOICE_CMD_WAKE:     return "WAKE";
    default:                 return "";
  }
}

#endif
