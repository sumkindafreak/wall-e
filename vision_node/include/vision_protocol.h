/**
 * vision_protocol.h - WALL-E vision node → base brain (ESP-NOW)
 */
#ifndef VISION_PROTOCOL_H
#define VISION_PROTOCOL_H

#include <stdint.h>

#define VISION_MAGIC  0x5649534E

#define OBJ_CLASS_NONE   0
#define OBJ_CLASS_SMALL  1
#define OBJ_CLASS_MEDIUM 2
#define OBJ_CLASS_LARGE  3

/* Compact enums for wire protocol (matches recognition_engine.h) */
#define COLOUR_NONE   0
#define COLOUR_SKIN   1
#define COLOUR_RED    2
#define COLOUR_GREEN  3
#define COLOUR_BLUE   4
#define COLOUR_CUSTOM 5

#define DIST_FAR   0
#define DIST_MID   1
#define DIST_CLOSE 2

#define CLASS_UNKNOWN      0
#define CLASS_HUMAN_LIKE   1
#define CLASS_OBJECT       2
#define CLASS_LIGHT_SOURCE 3

#define VEVENT_NONE               0
#define VEVENT_TARGET_DETECTED    1
#define VEVENT_TARGET_APPROACHING 2
#define VEVENT_TARGET_CENTERED    3
#define VEVENT_TARGET_LOST        4

#define ZONE_LEFT    0
#define ZONE_CENTER  1
#define ZONE_RIGHT   2
#define ZONE_UNKNOWN 3

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;
  uint8_t  motionDetected;
  int16_t  targetX;
  int16_t  targetY;
  uint16_t objectSize;
  uint16_t bboxWidth;
  uint16_t bboxHeight;
  uint8_t  objectClass;
  uint32_t frameID;
  uint32_t visionNodeIp;  /* WiFi IP when connected to WALL-E AP, for web UI snapshot */
  /* Lightweight recognition layer */
  uint8_t  motionIntensity;     /* 0–100 */
  uint8_t  zone;               /* ZONE_* */
  uint8_t  colourId;           /* COLOUR_* */
  uint8_t  colourConfidence;   /* 0–100 */
  uint8_t  blobDetected;
  uint16_t blobSize;           /* blob pixel count (subsampled space) */
  int16_t  blobX;
  int16_t  blobY;
  uint8_t  distanceBand;       /* DIST_* */
  uint8_t  classification;     /* CLASS_* */
  uint8_t  visionEvent;        /* VEVENT_* */
  uint8_t  targetLocked;
  uint8_t  targetLockConfidence; /* 0–100 */
} VisionPacket_t;
#pragma pack(pop)

#define VISION_PACKET_SIZE  (sizeof(VisionPacket_t))

#endif
