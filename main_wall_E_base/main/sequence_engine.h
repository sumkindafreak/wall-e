#pragma once

#include <Arduino.h>

void sequenceEngineInit(void);
void sequenceEngineTick(unsigned long now_ms);

bool sequenceRun(const char* id, char* errBuf, size_t errLen);
void sequenceStop(void);
bool sequenceIsRunning(void);

String sequenceGetStatusJSON(void);
String sequenceListJSON(void);
bool sequenceGetOneJSON(const char* id, String& out);
bool sequenceSaveJSON(const char* body, size_t len, char* errBuf, size_t errLen);
bool sequenceDelete(const char* id);
