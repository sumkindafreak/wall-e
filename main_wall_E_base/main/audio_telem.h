#pragma once
#include <stdint.h>

void audioTelemOnPacket(const uint8_t *data, int len);
bool audioTelemGet(uint16_t *ear_l, uint16_t *ear_r, uint8_t *voice_active, uint32_t *age_ms);
