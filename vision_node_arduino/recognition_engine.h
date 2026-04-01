/**
 * recognition_engine.h — Fuse motion + colour + blob → classification, distance, events, lock.
 */
#ifndef RECOGNITION_ENGINE_H
#define RECOGNITION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "vision_protocol.h"
#include "motion_detect.h"

#define RECOGNITION_INTERVAL_MS 150u

void recognitionEngineInit(void);
void recognitionSetCustomTargetColour(uint8_t r, uint8_t g, uint8_t b, uint8_t tol);

/** Fill motion-derived fields: zone, motionIntensity, optional target smooth prep */
void recognitionApplyMotionBasics(VisionPacket_t* pkt, const MotionDetect* md, int frameW, int frameH);

/**
 * Process one RGB565 frame (same resolution as grayscale, typically QQVGA).
 * Updates pkt recognition fields; does not clear motion fields.
 */
void recognitionProcessRgbFrame(VisionPacket_t* pkt, const uint8_t* rgb565Buf, int w, int h);

/** Apply target smoothing / lock confidence to targetX/Y in pkt */
void recognitionApplyLockSmoothing(VisionPacket_t* pkt, int frameW, int frameH);

#endif
