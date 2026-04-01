/**
 * blob_detection.cpp — Single connected region from binary mask (bounding box + centroid).
 */

#include "blob_detection.h"
#include <string.h>

void blobDetectFromMask(const uint8_t* mask, int mw, int mh, BlobResult* out) {
  memset(out, 0, sizeof(*out));
  if (!mask || mw <= 0 || mh <= 0) return;

  int minX = mw, maxX = -1, minY = mh, maxY = -1;
  uint32_t count = 0;

  for (int y = 0; y < mh; y++) {
    for (int x = 0; x < mw; x++) {
      if (mask[y * mw + x] > 127) {
        count++;
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }

  if (count == 0 || maxX < minX || maxY < minY) {
    out->detected = false;
    return;
  }

  out->detected = true;
  out->pixelCount = (uint16_t)(count > 0xFFFF ? 0xFFFF : count);
  out->minX = (int16_t)minX;
  out->maxX = (int16_t)maxX;
  out->minY = (int16_t)minY;
  out->maxY = (int16_t)maxY;
  out->cx = (int16_t)((minX + maxX) / 2);
  out->cy = (int16_t)((minY + maxY) / 2);
}
