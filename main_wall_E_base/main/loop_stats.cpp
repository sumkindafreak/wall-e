#include "loop_stats.h"
#include <string.h>
#include <Arduino.h>

#define LOOP_STAT_RING 50

static uint16_t s_us[LOOP_STAT_RING];
static uint16_t s_nSamples;
static bool s_filled;
static uint32_t s_worst;

void loopStatsReset(void) {
  memset(s_us, 0, sizeof(s_us));
  s_nSamples = 0;
  s_filled = false;
  s_worst = 0;
}

void loopStatsOnLoopEnd(uint32_t loop_duration_us) {
  if (loop_duration_us > 6000000u) return;
  if (loop_duration_us > s_worst) s_worst = loop_duration_us;
  s_us[s_nSamples % LOOP_STAT_RING] = (uint16_t)min((uint32_t)65535u, loop_duration_us);
  s_nSamples++;
  if (s_nSamples >= LOOP_STAT_RING) s_filled = true;
}

static void insertionSortN(uint16_t* a, int n) {
  for (int i = 1; i < n; i++) {
    uint16_t k = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > k) {
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = k;
  }
}

uint16_t loopStatsGetP95Ms(void) {
  if (!s_filled && s_nSamples < 2) return 0;
  int n = s_filled ? LOOP_STAT_RING : (int)s_nSamples;
  if (n < 1) return 0;
  if (n > LOOP_STAT_RING) n = LOOP_STAT_RING;
  uint16_t copy[LOOP_STAT_RING];
  memcpy(copy, s_us, (size_t)n * sizeof(uint16_t));
  insertionSortN(copy, n);
  int k = (n * 95) / 100;
  if (k >= n) k = n - 1;
  if (k < 0) k = 0;
  uint32_t u = copy[k];
  if (u == 0) return 0;
  return (uint16_t)((u + 500u) / 1000u);
}

uint32_t loopStatsGetWorstUs(void) { return s_worst; }
