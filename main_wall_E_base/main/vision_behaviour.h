#pragma once
#include <stdint.h>
#include "vision_protocol.h"
#include <Arduino.h>

typedef enum {
  VBEHAVE_IDLE, VBEHAVE_CURIOUS, VBEHAVE_TRACKING, VBEHAVE_FOLLOWING,
  VBEHAVE_SCANNING, VBEHAVE_STARTLED, VBEHAVE_DOCKING, VBEHAVE_SLEEP
} VisionBehaviourState;
void visionBehaviourInit(void);
void visionBehaviourUpdate(uint32_t now);
void visionBehaviourOnPacket(const VisionPacket_t* pkt, size_t len);
VisionBehaviourState visionBehaviourGetState(void);
const char* visionBehaviourGetStateName(void);
String visionGetStatusJSON(void);
void visionSetEnabled(bool en);
bool visionIsEnabled(void);