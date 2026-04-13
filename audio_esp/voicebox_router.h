#ifndef AUDIO_ESP_VOICEBOX_ROUTER_H
#define AUDIO_ESP_VOICEBOX_ROUTER_H

#include <Arduino.h>
#include <stdint.h>
#include "voicebox_protocol.h"
#include "menu_protocol.h"

void voiceboxRouterInit(void);
void voiceboxRouterApplyCmd(const uint8_t* data, int len);

walle_voicebox_mode_t voiceboxRouterGetMode(void);
uint8_t voiceboxRouterGetEveAudioOk(void);
uint8_t voiceboxRouterGetBondStrength(void);

void voiceboxRouterSetPairRequest(walle_ui_pair_request_t req);
walle_ui_pair_request_t voiceboxRouterPeekPairRequest(void);
void voiceboxRouterClearPairRequest(void);

bool voiceboxRouterIsSharedActive(void);

#endif
