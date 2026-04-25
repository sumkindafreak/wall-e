#pragma once
#include <Arduino.h>
#include "eve_protocol.h"
typedef void (*DockEveFrameCallback)(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);
void dockEveLinkInit(void);
void dockEveLinkPoll(uint32_t maxBytes);
void dockEveLinkSetFrameCallback(DockEveFrameCallback cb);
bool dockEveLinkSendJson(EveMsgType type, const char* jsonUtf8);
