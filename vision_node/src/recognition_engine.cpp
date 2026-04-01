/**
 * recognition_engine.cpp — Fuse colour, blob, distance, classification, events, lock.
 */

#include "recognition_engine.h"
#include "colour_detection.h"
#include "blob_detection.h"
#include <Arduino.h>
#include <string.h>

#define SUBSTEP 4
#define MASK_W ((160 + SUBSTEP - 1) / SUBSTEP)
#define MASK_H ((120 + SUBSTEP - 1) / SUBSTEP)

static uint8_t s_customR = 220;
static uint8_t s_customG = 180;
static uint8_t s_customB = 40;
static uint8_t s_customTol = 55;

static float s_smoothX = 80.0f;
static float s_smoothY = 60.0f;

static bool s_prevMotion = false;
static uint32_t s_lastMotionSeenMs = 0;
static uint16_t s_prevObjectSize = 0;
static bool s_lostEventLatched = false;
static struct {
  uint8_t colourId;
  uint8_t colourConfidence;
  uint8_t blobDetected;
  uint16_t blobSize;
  int16_t blobX;
  int16_t blobY;
  uint8_t distanceBand;
  uint8_t classification;
  uint8_t targetLocked;
  uint8_t targetLockConfidence;
} s_lastRec;

static inline uint8_t distanceFromArea(uint16_t area) {
  if (area > 2500) return DIST_CLOSE;
  if (area > 650) return DIST_MID;
  return DIST_FAR;
}

static inline uint8_t zoneFromX(int16_t x, int w) {
  if (w <= 0) return ZONE_UNKNOWN;
  int t = w / 3;
  if (x < t) return ZONE_LEFT;
  if (x < 2 * t) return ZONE_CENTER;
  return ZONE_RIGHT;
}

void recognitionEngineInit(void) {
  memset(&s_lastRec, 0, sizeof(s_lastRec));
  s_smoothX = 80.0f;
  s_smoothY = 60.0f;
  s_prevMotion = false;
  s_lastMotionSeenMs = 0;
  s_prevObjectSize = 0;
  s_lostEventLatched = false;
}

void recognitionSetCustomTargetColour(uint8_t r, uint8_t g, uint8_t b, uint8_t tol) {
  s_customR = r;
  s_customG = g;
  s_customB = b;
  s_customTol = tol;
}

static void copyLastRecToPacket(VisionPacket_t* pkt) {
  pkt->colourId = s_lastRec.colourId;
  pkt->colourConfidence = s_lastRec.colourConfidence;
  pkt->blobDetected = s_lastRec.blobDetected;
  pkt->blobSize = s_lastRec.blobSize;
  pkt->blobX = s_lastRec.blobX;
  pkt->blobY = s_lastRec.blobY;
  pkt->distanceBand = s_lastRec.distanceBand;
  pkt->classification = s_lastRec.classification;
  pkt->targetLocked = s_lastRec.targetLocked;
  pkt->targetLockConfidence = s_lastRec.targetLockConfidence;
}

void recognitionApplyMotionBasics(VisionPacket_t* pkt, const MotionDetect* md, int frameW, int frameH) {
  pkt->motionIntensity = md->motionIntensity;
  pkt->zone = ZONE_UNKNOWN;
  if (md->motionDetected) {
    pkt->zone = zoneFromX(md->targetX, frameW);
  }
  copyLastRecToPacket(pkt);

  uint32_t now = millis();
  if (md->motionDetected) {
    s_lastMotionSeenMs = now;
    s_lostEventLatched = false;
  }

  uint8_t ev = VEVENT_NONE;
  if (md->motionDetected && !s_prevMotion) {
    ev = VEVENT_TARGET_DETECTED;
  } else if (!md->motionDetected && s_lastMotionSeenMs != 0 && (now - s_lastMotionSeenMs) > 300 && !s_lostEventLatched) {
    ev = VEVENT_TARGET_LOST;
    s_lostEventLatched = true;
  } else if (md->motionDetected && s_prevObjectSize > 0 && md->objectSize > s_prevObjectSize + 200) {
    ev = VEVENT_TARGET_APPROACHING;
  } else if (md->motionDetected) {
    int cx = frameW / 2;
    if (abs((int)md->targetX - cx) < 14) ev = VEVENT_TARGET_CENTERED;
  }

  pkt->visionEvent = ev;

  s_prevMotion = md->motionDetected;
  s_prevObjectSize = md->objectSize;
}

void recognitionProcessRgbFrame(VisionPacket_t* pkt, const uint8_t* rgb565Buf, int w, int h) {
  if (!rgb565Buf || w < 16 || h < 16) return;

  ColourDetectResult cr;
  colourDetectRGB565(rgb565Buf, w, h, SUBSTEP, s_customR, s_customG, s_customB, s_customTol, &cr);

  static uint8_t s_mask[MASK_W * MASK_H];
  memset(s_mask, 0, sizeof(s_mask));

  for (int y = 0; y < h; y += SUBSTEP) {
    int my = y / SUBSTEP;
    if (my >= MASK_H) break;
    for (int x = 0; x < w; x += SUBSTEP) {
      int mx = x / SUBSTEP;
      if (mx >= MASK_W) break;
      const uint8_t* px = rgb565Buf + ((size_t)y * (size_t)w + (size_t)x) * 2;
      uint16_t p = (uint16_t)px[0] | ((uint16_t)px[1] << 8);
      uint8_t r, g, b;
      r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
      g = (uint8_t)(((p >> 5) & 0x3F) * 255 / 63);
      b = (uint8_t)((p & 0x1F) * 255 / 31);

      bool hit = false;
      if (cr.dominantId == COLOUR_SKIN) {
        int drg = (int)r - (int)g;
        int drb = (int)r - (int)b;
        hit = (r > 95 && g > 40 && b > 20 && r > g && r > b && drg > 15 && drb > 15);
      } else if (cr.dominantId == COLOUR_RED)
        hit = (r > 140 && r > g + 40 && r > b + 40);
      else if (cr.dominantId == COLOUR_GREEN)
        hit = (g > 100 && g > r + 30 && g > b + 30);
      else if (cr.dominantId == COLOUR_BLUE)
        hit = (b > 90 && b > r + 25 && b > g + 25);
      else if (cr.dominantId == COLOUR_CUSTOM) {
        int dr = abs((int)r - (int)s_customR);
        int dg = abs((int)g - (int)s_customG);
        int db = abs((int)b - (int)s_customB);
        hit = (dr <= (int)s_customTol && dg <= (int)s_customTol && db <= (int)s_customTol);
      } else {
        /* No strong colour — use mid luminance “object” blob */
        hit = ((uint32_t)r + g + b > 280) && ((uint32_t)r + g + b < 720);
      }

      s_mask[my * MASK_W + mx] = hit ? 255 : 0;
    }
  }

  BlobResult blob;
  blobDetectFromMask(s_mask, MASK_W, MASK_H, &blob);

  int16_t bx = pkt->targetX;
  int16_t by = pkt->targetY;
  uint16_t bsize = 0;
  uint8_t bdet = 0;
  if (blob.detected && blob.pixelCount > 8) {
    bdet = 1;
    bsize = blob.pixelCount;
    bx = (int16_t)(blob.cx * SUBSTEP + SUBSTEP / 2);
    by = (int16_t)(blob.cy * SUBSTEP + SUBSTEP / 2);
    if (bx > (int16_t)(w - 1)) bx = (int16_t)(w - 1);
    if (by > (int16_t)(h - 1)) by = (int16_t)(h - 1);
  }

  /* Light-source scan: fraction of very bright pixels */
  uint32_t bright = 0, total = 0;
  for (int y = 0; y < h; y += SUBSTEP * 2) {
    for (int x = 0; x < w; x += SUBSTEP * 2) {
      const uint8_t* px = rgb565Buf + ((size_t)y * (size_t)w + (size_t)x) * 2;
      uint16_t p = (uint16_t)px[0] | ((uint16_t)px[1] << 8);
      uint8_t r, g, b;
      r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
      g = (uint8_t)(((p >> 5) & 0x3F) * 255 / 63);
      b = (uint8_t)((p & 0x1F) * 255 / 31);
      total++;
      if ((uint32_t)r + g + b > 720) bright++;
    }
  }

  uint8_t cls = CLASS_UNKNOWN;
  if (total > 0 && (bright * 100u) / total > 35) {
    cls = CLASS_LIGHT_SOURCE;
  } else if (cr.dominantId == COLOUR_SKIN && cr.confidence >= 18) {
    cls = CLASS_HUMAN_LIKE;
  } else if (pkt->motionDetected && bdet) {
    cls = CLASS_OBJECT;
  }

  uint8_t dist = distanceFromArea(pkt->objectSize > 0 ? pkt->objectSize : bsize * SUBSTEP * SUBSTEP);

  uint8_t lockConf = 0;
  if (pkt->motionDetected && (cr.confidence >= 25 || bdet)) {
    {
      int lc = (int)cr.confidence + (int)pkt->motionIntensity / 4;
      if (lc > 100) lc = 100;
      lockConf = (uint8_t)lc;
    }
  }
  uint8_t locked = (lockConf >= 38) ? 1u : 0u;

  s_lastRec.colourId = cr.dominantId;
  s_lastRec.colourConfidence = cr.confidence;
  s_lastRec.blobDetected = bdet;
  s_lastRec.blobSize = bsize;
  s_lastRec.blobX = bx;
  s_lastRec.blobY = by;
  s_lastRec.distanceBand = dist;
  s_lastRec.classification = cls;
  s_lastRec.targetLocked = locked;
  s_lastRec.targetLockConfidence = lockConf;

  pkt->colourId = cr.dominantId;
  pkt->colourConfidence = cr.confidence;
  pkt->blobDetected = bdet;
  pkt->blobSize = bsize;
  pkt->blobX = bx;
  pkt->blobY = by;
  pkt->distanceBand = dist;
  pkt->classification = cls;
  pkt->targetLocked = locked;
  pkt->targetLockConfidence = lockConf;

  Serial.printf(
    "[Recogn] col=%u conf=%u blob=%u sz=%u cls=%u dist=%u lock=%u\n",
    (unsigned)cr.dominantId, (unsigned)cr.confidence, (unsigned)bdet,
    (unsigned)bsize, (unsigned)cls, (unsigned)dist, (unsigned)locked);
}

void recognitionApplyLockSmoothing(VisionPacket_t* pkt, int frameW, int frameH) {
  if (!pkt->targetLocked) {
    s_smoothX = (float)pkt->targetX;
    s_smoothY = (float)pkt->targetY;
    return;
  }
  float alpha = 0.18f;
  s_smoothX = s_smoothX * (1.0f - alpha) + (float)pkt->targetX * alpha;
  s_smoothY = s_smoothY * (1.0f - alpha) + (float)pkt->targetY * alpha;
  pkt->targetX = (int16_t)(s_smoothX + 0.5f);
  pkt->targetY = (int16_t)(s_smoothY + 0.5f);
  if (pkt->targetX < 0) pkt->targetX = 0;
  if (pkt->targetY < 0) pkt->targetY = 0;
  if (frameW > 0 && pkt->targetX >= frameW) pkt->targetX = (int16_t)(frameW - 1);
  if (frameH > 0 && pkt->targetY >= frameH) pkt->targetY = (int16_t)(frameH - 1);
}
