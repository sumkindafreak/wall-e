#pragma once
#include <stdint.h>

void loopStatsReset(void);
void loopStatsOnLoopEnd(uint32_t loop_duration_us);
/** Approximate 95th percentile of main-loop duration over last 50 samples, in ms (1 = coarse). */
uint16_t loopStatsGetP95Ms(void);
uint32_t loopStatsGetWorstUs(void);
