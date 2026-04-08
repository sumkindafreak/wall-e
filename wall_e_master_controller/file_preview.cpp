// ============================================================
//  WALL-E CYD — file preview implementation
// ============================================================

#include "file_preview.h"
#include "sd_manager.h"

#include <SD.h>
#include <Arduino.h>
#include <string.h>
#include <strings.h>

bool filePreviewIsTextExtension(const char* filename) {
  if (!filename) return false;
  const char* dot = strrchr(filename, '.');
  if (!dot) return false;
  dot++;
  if (strcasecmp(dot, "txt") == 0) return true;
  if (strcasecmp(dot, "log") == 0) return true;
  if (strcasecmp(dot, "json") == 0) return true;
  if (strcasecmp(dot, "csv") == 0) return true;
  if (strcasecmp(dot, "md") == 0) return true;
  if (strcasecmp(dot, "cfg") == 0) return true;
  if (strcasecmp(dot, "ini") == 0) return true;
  return false;
}

bool filePreviewLoad(const char* fullPath, char* buf, size_t bufCap, size_t* outLen) {
  if (!sdIsAvailable() || !fullPath || !buf || bufCap < 8) return false;
  File f = SD.open(fullPath, FILE_READ);
  if (!f) {
    Serial.printf("[Preview] open failed: %s\n", fullPath);
    return false;
  }
  size_t maxRead = bufCap - 1;
  if (maxRead > FILE_PREVIEW_MAX_BYTES) maxRead = FILE_PREVIEW_MAX_BYTES;
  size_t n = f.read((uint8_t*)buf, maxRead);
  f.close();
  for (size_t i = 0; i < n; i++) {
    char c = buf[i];
    if ((unsigned char)c < 9 && c != '\n' && c != '\r' && c != '\t') buf[i] = '.';
  }
  buf[n] = '\0';
  if (outLen) *outLen = n;
  Serial.printf("[Preview] Loaded %u bytes from %s\n", (unsigned)n, fullPath);
  return true;
}

bool filePreviewLoadInfo(const char* fullPath, char* buf, size_t bufCap) {
  if (!sdIsAvailable() || !fullPath || !buf || bufCap < 32) return false;
  File f = SD.open(fullPath, FILE_READ);
  if (!f) {
    snprintf(buf, bufCap, "Cannot open file.");
    return false;
  }
  uint32_t sz = (uint32_t)f.size();
  f.close();
  snprintf(buf, bufCap,
           "Binary / no text preview\n%lu bytes\n",
           (unsigned long)sz);
  return true;
}
