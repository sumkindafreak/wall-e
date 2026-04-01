/**
 * colour_detection.h — Lightweight RGB565 colour heuristics (no ML).
 * Subsamples the frame; no heap allocations.
 */
#ifndef COLOUR_DETECTION_H
#define COLOUR_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

#define COLOUR_NONE   0
#define COLOUR_SKIN   1
#define COLOUR_RED    2
#define COLOUR_GREEN  3
#define COLOUR_BLUE   4
#define COLOUR_CUSTOM 5

typedef struct {
  uint8_t dominantId;       /* COLOUR_* */
  uint8_t confidence;       /* 0–100 */
  uint16_t skinCount;
  uint16_t redCount;
  uint16_t greenCount;
  uint16_t blueCount;
  uint16_t customCount;
  uint16_t sampleCount;
} ColourDetectResult;

/**
 * Analyse RGB565 frame (little-endian 16-bit per pixel).
 * step: pixel stride (>=2 recommended, e.g. 4).
 * custom RGB 0–255 with tolerance for dock marker.
 */
void colourDetectRGB565(
  const uint8_t* rgb565Buf,
  int width,
  int height,
  int step,
  uint8_t customR,
  uint8_t customG,
  uint8_t customB,
  uint8_t customTol,
  ColourDetectResult* out);

#endif
