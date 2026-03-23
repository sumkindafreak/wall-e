/**
 * voice_commands.cpp — Parse voice command strings
 * Voice commands come from Base (forwarded as "VC:STOP") or from
 * a dedicated voice UART if PIN_VOICE_RX is set.
 */
#include "voice_commands.h"
#include <string.h>

static unsigned long s_lastCmdTime = 0;

void voiceCommandsInit() {
  s_lastCmdTime = 0;
  DEBUG_LOG("Voice commands init");
}

static VoiceCmdId parseCommand(const char* s) {
  if (!s || !*s) return VOICE_CMD_NONE;
  char buf[32];
  size_t i = 0;
  while (*s && i < sizeof(buf) - 1) {
    char c = *s++;
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c != ' ' && c != '\r' && c != '\n') buf[i++] = c;
  }
  buf[i] = '\0';

  if (strstr(buf, "STOP"))       return VOICE_CMD_STOP;
  if (strstr(buf, "COMEHERE") || strstr(buf, "COME_HERE")) return VOICE_CMD_COME_HERE;
  if (strstr(buf, "GOHOME")   || strstr(buf, "GO_HOME"))  return VOICE_CMD_GO_HOME;
  if (strstr(buf, "SLEEP"))   return VOICE_CMD_SLEEP;
  if (strstr(buf, "WAKE"))    return VOICE_CMD_WAKE;
  return VOICE_CMD_NONE;
}

/* Parse a line; if "VC:xxx" or raw command, returns ID. Applies cooldown. */
VoiceCmdId voiceCommandsParse(const char* line) {
  if (!line || !*line) return VOICE_CMD_NONE;
  const char* toParse = line;
  if (strncmp(line, "VC:", 3) == 0) toParse = line + 3;
  VoiceCmdId id = parseCommand(toParse);
  if (id == VOICE_CMD_NONE) return VOICE_CMD_NONE;
  unsigned long now = millis();
  if (now - s_lastCmdTime >= VOICE_COOLDOWN_MS) {
    s_lastCmdTime = now;
    return id;
  }
  return VOICE_CMD_NONE;
}

VoiceCmdId voiceCommandsTick() {
  /* When PIN_VOICE_RX >= 0, read from voice UART here. For now, voice comes via comms. */
  return VOICE_CMD_NONE;
}
