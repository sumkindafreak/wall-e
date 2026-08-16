#include "audio_espnow.h"
#include "audio_protocol.h"
#include "radio_transport.h"

static bool sendPacket(uint8_t cmd, uint8_t param, uint8_t priority) {
  WalleAudioCommandPacket_t pkt = {
      .magic = WALLE_AUDIO_MAGIC,
      .cmd = cmd,
      .param = param,
      .reserved = 0,
      .priority = priority,
  };
  return radioTransportBroadcast(&pkt, sizeof(pkt));
}

void audioEspNowInit(void) {
  (void)radioTransportInit();
}

bool audioEspNowPlayTrack(uint8_t track, uint8_t priority) {
  if (track < 1) return false;
  return sendPacket(WALLE_AU_CMD_PLAY_TRACK, track, priority);
}

bool audioEspNowSetVolume(uint8_t vol_dfplayer_0_30) {
  const uint8_t v = vol_dfplayer_0_30 > 30 ? 30 : vol_dfplayer_0_30;
  return sendPacket(WALLE_AU_CMD_VOLUME, v, 0);
}

bool audioEspNowSendEvent(uint8_t event_id, uint8_t priority) {
  return sendPacket(WALLE_AU_CMD_PLAY_EVENT, event_id, priority);
}

bool audioEspNowStop(void) {
  return sendPacket(WALLE_AU_CMD_STOP, 0, 0);
}
