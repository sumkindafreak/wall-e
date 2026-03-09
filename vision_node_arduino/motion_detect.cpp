/**
 * motion_detect.cpp - Frame differencing, clustering, centroid, object classification.
 */
#include "motion_detect.h"
#include <string.h>
#include <Arduino.h>

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

void motionDetectInit(MotionDetect* md) {
  memset(md, 0, sizeof(MotionDetect));
  md->motionThreshold = 25;
  md->minMotionPixels = 30;
  md->smoothFactor = 0.4f;
  md->occlusionTimeoutMs = 500;
}

void motionDetectSetFrameSize(MotionDetect* md, int w, int h) {
  md->width = w;
  md->height = h;
  md->centerX = w / 2;
  md->centerY = h / 2;
}

static void clusterMotionPixels(MotionDetect* md, const uint8_t* diff) {
  int w = md->width;
  int h = md->height;
  int minX = w, maxX = 0, minY = h, maxY = 0;
  int count = 0;

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if (diff[y * w + x] > 0) {
        count++;
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }

  md->clusterMinX = minX <= maxX ? minX : 0;
  md->clusterMaxX = maxX;
  md->clusterMinY = minY <= maxY ? minY : 0;
  md->clusterMaxY = maxY;
  md->motionPixelCount = count;
}

static uint8_t classifyObjectSize(uint32_t size) {
  if (size < 300) return OBJ_CLASS_SMALL;
  if (size <= 1200) return OBJ_CLASS_MEDIUM;
  return OBJ_CLASS_LARGE;
}

bool motionDetectProcess(MotionDetect* md, const uint8_t* current, const uint8_t* previous) {
  int w = md->width;
  int h = md->height;
  int sz = w * h;

  if (md->diffBuffer == nullptr || sz > (int)md->diffBufferSize) return false;
  uint8_t* diff = md->diffBuffer;

  int th = md->motionThreshold;
  for (int i = 0; i < sz; i++) {
    int d = abs((int)current[i] - (int)previous[i]);
    diff[i] = (d >= th) ? 255 : 0;
  }

  clusterMotionPixels(md, diff);

  bool motion = (md->motionPixelCount >= md->minMotionPixels);

  if (motion) {
    md->frameID++;
    int bw = md->clusterMaxX - md->clusterMinX + 1;
    int bh = md->clusterMaxY - md->clusterMinY + 1;
    bw = CLAMP(bw, 1, w);
    bh = CLAMP(bh, 1, h);

    int cx = md->clusterMinX + bw / 2;
    int cy = md->clusterMinY + bh / 2;

    float sm = md->smoothFactor;
    md->targetX = (int16_t)(md->targetX * (1.0f - sm) + cx * sm);
    md->targetY = (int16_t)(md->targetY * (1.0f - sm) + cy * sm);

    md->bboxWidth = (uint16_t)bw;
    md->bboxHeight = (uint16_t)bh;
    md->objectSize = (uint16_t)(bw * bh);
    md->objectClass = classifyObjectSize(md->objectSize);
    md->lastMotionMs = millis();
  } else {
    uint32_t now = millis();
    if (now - md->lastMotionMs > md->occlusionTimeoutMs) {
      md->objectClass = OBJ_CLASS_NONE;
      md->objectSize = 0;
    }
  }

  md->motionDetected = motion;
  return motion;
}
