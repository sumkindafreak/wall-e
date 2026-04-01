/**
 * blob_detection.h — Binary mask blob: bbox, area, centroid.
 */
#ifndef BLOB_DETECTION_H
#define BLOB_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  bool detected;
  uint16_t pixelCount;
  int16_t minX, maxX, minY, maxY;
  int16_t cx, cy;
} BlobResult;

/**
 * mask: mw*mh bytes, 0 or 1 (or >127 = set).
 */
void blobDetectFromMask(const uint8_t* mask, int mw, int mh, BlobResult* out);

#endif
