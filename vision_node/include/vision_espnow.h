#ifndef VISION_ESPNOW_H
#define VISION_ESPNOW_H

#include <stdbool.h>
#include "vision_protocol.h"

bool visionEspNowInit(void);
bool visionEspNowSend(const VisionPacket_t* pkt);

#endif
