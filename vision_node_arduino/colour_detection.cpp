/**
 * colour_detection.cpp — RGB565 subsampled heuristics (no heap).
 */

#include "colour_detection.h"
#include <string.h>

static inline void rgb565to888(uint16_t p, uint8_t* r, uint8_t* g, uint8_t* b) {
  uint8_t R5 = (uint8_t)((p >> 11) & 0x1F);
  uint8_t G6 = (uint8_t)((p >> 5) & 0x3F);
  uint8_t B5 = (uint8_t)(p & 0x1F);
  *r = (uint8_t)((R5 << 3) | (R5 >> 2));
  *g = (uint8_t)((G6 << 2) | (G6 >> 4));
  *b = (uint8_t)((B5 << 3) | (B5 >> 2));
}

static inline bool isSkinTone(uint8_t r, uint8_t g, uint8_t b) {
  /* Classic RGB skin heuristic (fast) */
  int drg = (int)r - (int)g;
  int drb = (int)r - (int)b;
  return (r > 95 && g > 40 && b > 20 && r > g && r > b && drg > 15 && drb > 15 && r > 60);
}

static inline bool isRed(uint8_t r, uint8_t g, uint8_t b) {
  return (r > 140 && r > g + 40 && r > b + 40);
}
static inline bool isGreen(uint8_t r, uint8_t g, uint8_t b) {
  return (g > 100 && g > r + 30 && g > b + 30);
}
static inline bool isBlue(uint8_t r, uint8_t g, uint8_t b) {
  return (b > 90 && b > r + 25 && b > g + 25);
}

static inline bool isCustom(uint8_t r, uint8_t g, uint8_t b, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t tol) {
  int dr = abs((int)r - (int)cr);
  int dg = abs((int)g - (int)cg);
  int db = abs((int)b - (int)cb);
  return (dr <= (int)tol && dg <= (int)tol && db <= (int)tol);
}

void colourDetectRGB565(
  const uint8_t* rgb565Buf,
  int width,
  int height,
  int step,
  uint8_t customR,
  uint8_t customG,
  uint8_t customB,
  uint8_t customTol,
  ColourDetectResult* out) {
  memset(out, 0, sizeof(*out));
  if (!rgb565Buf || width <= 0 || height <= 0 || step < 1) return;

  uint32_t skin = 0, red = 0, green = 0, blue = 0, custom = 0;
  uint32_t n = 0;

  for (int y = 0; y < height; y += step) {
    const uint8_t* row = rgb565Buf + (size_t)y * (size_t)width * 2;
    for (int x = 0; x < width; x += step) {
      const uint8_t* px = row + (size_t)x * 2;
      uint16_t p = (uint16_t)px[0] | ((uint16_t)px[1] << 8);
      uint8_t r, g, b;
      rgb565to888(p, &r, &g, &b);
      n++;
      if (isSkinTone(r, g, b)) skin++;
      else if (isRed(r, g, b)) red++;
      else if (isGreen(r, g, b)) green++;
      else if (isBlue(r, g, b)) blue++;
      if (isCustom(r, g, b, customR, customG, customB, customTol)) custom++;
    }
  }

  out->sampleCount = (uint16_t)(n > 0xFFFF ? 0xFFFF : n);
  out->skinCount = (uint16_t)(skin > 0xFFFF ? 0xFFFF : skin);
  out->redCount = (uint16_t)(red > 0xFFFF ? 0xFFFF : red);
  out->greenCount = (uint16_t)(green > 0xFFFF ? 0xFFFF : green);
  out->blueCount = (uint16_t)(blue > 0xFFFF ? 0xFFFF : blue);
  out->customCount = (uint16_t)(custom > 0xFFFF ? 0xFFFF : custom);

  if (n == 0) return;

  uint32_t maxCat = skin;
  uint8_t best = COLOUR_SKIN;
  if (red > maxCat) {
    maxCat = red;
    best = COLOUR_RED;
  }
  if (green > maxCat) {
    maxCat = green;
    best = COLOUR_GREEN;
  }
  if (blue > maxCat) {
    maxCat = blue;
    best = COLOUR_BLUE;
  }
  if (custom > maxCat) {
    maxCat = custom;
    best = COLOUR_CUSTOM;
  }

  if (maxCat == 0) {
    out->dominantId = COLOUR_NONE;
    out->confidence = 0;
    return;
  }

  out->dominantId = best;
  uint32_t conf = (maxCat * 100u) / n;
  if (conf > 100u) conf = 100u;
  out->confidence = (uint8_t)conf;
}
