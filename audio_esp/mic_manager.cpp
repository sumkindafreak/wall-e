/**
 * mic_manager.cpp — Left/right mic ADC and direction
 */
#include "mic_manager.h"

static int s_leftLevel = 0;
static int s_rightLevel = 0;
static MicDirection s_direction = MIC_DIR_UNKNOWN;
static bool s_noiseSurge = false;
static unsigned long s_lastRead = 0;
static int s_leftSmooth[MIC_SMOOTH_SAMPLES];
static int s_rightSmooth[MIC_SMOOTH_SAMPLES];
static uint8_t s_smoothIdx = 0;
static bool s_smoothFilled = false;

void micManagerInit() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(PIN_MIC_LEFT, INPUT);
  pinMode(PIN_MIC_RIGHT, INPUT);
  memset(s_leftSmooth, 0, sizeof(s_leftSmooth));
  memset(s_rightSmooth, 0, sizeof(s_rightSmooth));
  s_smoothIdx = 0;
  s_smoothFilled = false;
  s_direction = MIC_DIR_UNKNOWN;
  s_noiseSurge = false;
  DEBUG_LOG("Mic manager init");
}

static int average(const int* arr, int n) {
  long sum = 0;
  for (int i = 0; i < n; i++) sum += arr[i];
  return (int)(sum / n);
}

void micManagerTick() {
  unsigned long now = millis();
  if (now - s_lastRead < MIC_READ_INTERVAL_MS) return;
  s_lastRead = now;

  int leftRaw = analogRead(PIN_MIC_LEFT);
  int rightRaw = analogRead(PIN_MIC_RIGHT);

  s_leftSmooth[s_smoothIdx] = leftRaw;
  s_rightSmooth[s_smoothIdx] = rightRaw;
  s_smoothIdx = (s_smoothIdx + 1) % MIC_SMOOTH_SAMPLES;
  if (s_smoothIdx == 0) s_smoothFilled = true;

  int n = s_smoothFilled ? MIC_SMOOTH_SAMPLES : (int)s_smoothIdx;
  if (n == 0) n = 1;
  s_leftLevel = average(s_leftSmooth, n);
  s_rightLevel = average(s_rightSmooth, n);

  int maxLevel = (s_leftLevel > s_rightLevel) ? s_leftLevel : s_rightLevel;
  if (maxLevel < MIC_NOISE_FLOOR) {
    s_direction = MIC_DIR_UNKNOWN;
    s_noiseSurge = false;
    return;
  }

  /* Sudden surge check */
  s_noiseSurge = (maxLevel > MIC_SURGE_THRESHOLD);

  float ratio;
  if (s_rightLevel > 0)
    ratio = (float)s_leftLevel / (float)s_rightLevel;
  else
    ratio = (s_leftLevel > 0) ? 999.0f : 1.0f;

  if (ratio > MIC_DIR_THRESHOLD)
    s_direction = MIC_DIR_LEFT;
  else if (ratio < (1.0f / MIC_DIR_THRESHOLD))
    s_direction = MIC_DIR_RIGHT;
  else
    s_direction = MIC_DIR_CENTER;
}

MicDirection micGetDirection() { return s_direction; }
int micGetLeftLevel()  { return s_leftLevel; }
int micGetRightLevel() { return s_rightLevel; }
bool micHasNoiseSurge() { return s_noiseSurge; }
