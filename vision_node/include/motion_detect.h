/**
 * motion_detect.h
 * Frame differencing, clustering, centroid, object classification.
 */

#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <stdint.h>
#include <stdbool.h>
#include "vision_protocol.h"

typedef struct {
  int width, height;
  int centerX, centerY;
  int motionThreshold;      /* 20-30 typical */
  int minMotionPixels;      /* ~30 */
  float smoothFactor;       /* 0.4 for target smoothing */
  uint32_t occlusionTimeoutMs;  /* 500 */

  uint8_t* diffBuffer;      /* caller allocates: width*height bytes */
  size_t diffBufferSize;

  int clusterMinX, clusterMaxX, clusterMinY, clusterMaxY;
  int motionPixelCount;
  int16_t targetX, targetY;
  uint16_t bboxWidth, bboxHeight;
  uint16_t objectSize;
  uint8_t objectClass;
  uint32_t frameID;
  uint32_t lastMotionMs;
  bool motionDetected;
} MotionDetect;

void motionDetectInit(MotionDetect* md);
void motionDetectSetFrameSize(MotionDetect* md, int w, int h);
bool motionDetectProcess(MotionDetect* md, const uint8_t* current, const uint8_t* previous);

#endif /* MOTION_DETECT_H */
