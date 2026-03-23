/**
 * event_router.cpp — Route events to ESP-NOW (no UART)
 */
#include "event_router.h"
#include "audio_protocol.h"
#include <string.h>

/* Map our enums to protocol enums */
static uint8_t micDirToProto(MicDirection d) {
  switch (d) {
    case MIC_DIR_LEFT:   return WALLE_AU_MIC_DIR_LEFT;
    case MIC_DIR_RIGHT:  return WALLE_AU_MIC_DIR_RIGHT;
    case MIC_DIR_CENTER: return WALLE_AU_MIC_DIR_CENTER;
    default:             return WALLE_AU_MIC_DIR_UNKNOWN;
  }
}
static uint8_t dockIrToProto(DockIrState s) {
  switch (s) {
    case DOCK_IR_LEFT:    return WALLE_AU_DOCK_IR_LEFT;
    case DOCK_IR_RIGHT:   return WALLE_AU_DOCK_IR_RIGHT;
    case DOCK_IR_BOTH:    return WALLE_AU_DOCK_IR_BOTH;
    case DOCK_IR_UNSTABLE: return WALLE_AU_DOCK_IR_UNSTABLE;
    default:              return WALLE_AU_DOCK_IR_NONE;
  }
}
static uint8_t voiceCmdToProto(VoiceCmdId id) {
  switch (id) {
    case VOICE_CMD_STOP:     return WALLE_AU_VOICE_STOP;
    case VOICE_CMD_COME_HERE: return WALLE_AU_VOICE_COME_HERE;
    case VOICE_CMD_GO_HOME:  return WALLE_AU_VOICE_GO_HOME;
    case VOICE_CMD_SLEEP:    return WALLE_AU_VOICE_SLEEP;
    case VOICE_CMD_WAKE:     return WALLE_AU_VOICE_WAKE;
    default:                 return WALLE_AU_VOICE_NONE;
  }
}
static uint8_t modeToProto(SystemMode m) {
  switch (m) {
    case MODE_BOOT:         return WALLE_AU_MODE_BOOT;
    case MODE_IDLE:         return WALLE_AU_MODE_IDLE;
    case MODE_LISTENING:    return WALLE_AU_MODE_LISTENING;
    case MODE_PLAYING_AUDIO: return WALLE_AU_MODE_PLAYING;
    case MODE_DOCK_ASSIST:  return WALLE_AU_MODE_DOCK_ASSIST;
    case MODE_VOICE_COMMAND: return WALLE_AU_MODE_VOICE_CMD;
    case MODE_FAULT:        return WALLE_AU_MODE_FAULT;
    default:                return WALLE_AU_MODE_IDLE;
  }
}
static uint8_t faultToProto(const char* fault) {
  if (!fault || !*fault) return WALLE_AU_FAULT_NONE;
  if (strstr(fault, "DFPLAYER")) return WALLE_AU_FAULT_DFPLAYER;
  return WALLE_AU_FAULT_NONE;
}

/* Last emitted values for change-driven status */
static uint8_t s_lastVoiceCmd = WALLE_AU_VOICE_NONE;
static uint8_t s_fault = WALLE_AU_FAULT_NONE;

void eventRouterInit() {
  s_lastVoiceCmd = WALLE_AU_VOICE_NONE;
  DEBUG_LOG("Event router init (ESP-NOW)");
}

void eventRouterProcessIncoming() {
  /* Incoming handled by espnow_manager recv callback */
}

void eventRouterTick() {}

void eventEmitVoiceCmd(const char* cmd) {
  VoiceCmdId id = VOICE_CMD_NONE;
  if (cmd) {
    if (strstr(cmd, "STOP")) id = VOICE_CMD_STOP;
    else if (strstr(cmd, "COME_HERE")) id = VOICE_CMD_COME_HERE;
    else if (strstr(cmd, "GO_HOME")) id = VOICE_CMD_GO_HOME;
    else if (strstr(cmd, "SLEEP")) id = VOICE_CMD_SLEEP;
    else if (strstr(cmd, "WAKE")) id = VOICE_CMD_WAKE;
  }
  s_lastVoiceCmd = voiceCmdToProto(id);
}

void eventEmitMicDir(const char* dir) {
  (void)dir;
  /* Status sent from main loop with full state */
}

void eventEmitDockIr(const char* state) {
  (void)state;
}

void eventEmitAudioBusy(bool busy) {
  (void)busy;
}

void eventEmitAudioDone() {}

void eventEmitFault(const char* fault) {
  s_fault = faultToProto(fault);
}

void eventEmitMode(const char* mode) {
  (void)mode;
}

/* Called from main loop to send combined status */
void eventRouterSendStatus(void) {
  uint8_t vc = s_lastVoiceCmd;
  s_lastVoiceCmd = WALLE_AU_VOICE_NONE;  /* One-shot */
  espnowManagerSendStatus(
    micDirToProto(micGetDirection()),
    dockIrToProto(irDockGetState()),
    vc,
    modeToProto(getSystemMode()),
    s_fault,
    audioIsBusy() ? 1 : 0
  );
}
