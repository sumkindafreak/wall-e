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
} VisionPacket_t;
#pragma pack(pop)

#define VISION_PACKET_SIZE  (sizeof(VisionPacket_t))

#endif
