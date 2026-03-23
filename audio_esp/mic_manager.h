/**
 * mic_manager.h — Left/right mic ADC, smoothing, direction detection
 * Direction: LEFT, RIGHT, CENTER, UNKNOWN
 */
#ifndef AUDIO_ESP_MIC_MANAGER_H
#define AUDIO_ESP_MIC_MANAGER_H

#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include <Arduino.h>

enum MicDirection {
  MIC_DIR_LEFT,
  MIC_DIR_RIGHT,
  MIC_DIR_CENTER,
  MIC_DIR_UNKNOWN
};

/* Initialize ADC for mics */
void micManagerInit();

/* Call periodically (e.g. every 20ms). Updates direction. */
void micManagerTick();

/* Get current direction */
MicDirection micGetDirection();

/* Get raw levels (0–4095) for diagnostics */
int micGetLeftLevel();
int micGetRightLevel();

/* Returns true if sudden noise surge detected */
bool micHasNoiseSurge();

/* Convert enum to string for protocol */
inline const char* micDirToString(MicDirection d) {
  switch (d) {
    case MIC_DIR_LEFT:   return "LEFT";
    case MIC_DIR_RIGHT:  return "RIGHT";
    case MIC_DIR_CENTER: return "CENTER";
    case MIC_DIR_UNKNOWN:
    default:             return "UNKNOWN";
  }
}

#endif
