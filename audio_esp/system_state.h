/**
 * system_state.h — System mode and state management
 */
#ifndef AUDIO_ESP_SYSTEM_STATE_H
#define AUDIO_ESP_SYSTEM_STATE_H

#include "debug_log.h"

/* System operating modes */
enum SystemMode {
  MODE_BOOT,
  MODE_IDLE,
  MODE_LISTENING,
  MODE_PLAYING_AUDIO,
  MODE_DOCK_ASSIST,
  MODE_VOICE_COMMAND,
  MODE_FAULT
};

extern SystemMode g_systemMode;

inline const char* modeToString(SystemMode m) {
  switch (m) {
    case MODE_BOOT:         return "BOOT";
    case MODE_IDLE:         return "IDLE";
    case MODE_LISTENING:    return "LISTENING";
    case MODE_PLAYING_AUDIO: return "PLAYING_AUDIO";
    case MODE_DOCK_ASSIST:  return "DOCK_ASSIST";
    case MODE_VOICE_COMMAND: return "VOICE_COMMAND";
    case MODE_FAULT:        return "FAULT";
    default:                return "UNKNOWN";
  }
}

inline void setSystemMode(SystemMode m) {
  if (g_systemMode != m) {
    DEBUG_LOG("Mode: %s -> %s", modeToString(g_systemMode), modeToString(m));
    g_systemMode = m;
  }
}

inline SystemMode getSystemMode() { return g_systemMode; }

#endif
