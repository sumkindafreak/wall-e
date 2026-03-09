/**
 * motion_detect.h - Frame differencing, clustering, centroid, object classification.
 */
#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vision_protocol.h"

typedef struct {
  int width, height;
  int centerX, centerY;
  int motionThreshold;
  int minMotionPixels;
  float smoothFactor;
  uint32_t occlusionTimeoutMs;

  uint8_t* diffBuffer;
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

#endif
