// ============================================================
//  WALL-E CYD — SD actions implementation
// ============================================================

#include "sd_actions.h"
#include "sd_manager.h"

#include <SD.h>
#include <Arduino.h>
#include <string.h>
#include <strings.h>

bool sdActionsIsPathDeleteProtected(const char* fullPath) {
  if (!fullPath || fullPath[0] != '/') return true;
  if (strncmp(fullPath, SD_ROOT, strlen(SD_ROOT)) != 0) return true;

  const char* base = strrchr(fullPath, '/');
  base = base ? base + 1 : fullPath;

  if (strcasecmp(base, "layout_version.txt") == 0) return true;

  return false;
}

bool sdActionsDeleteFile(const char* fullPath) {
  if (!sdIsAvailable() || !fullPath) return false;
  if (sdActionsIsPathDeleteProtected(fullPath)) {
    Serial.printf("[SDActions] Delete blocked (protected): %s\n", fullPath);
    return false;
  }
  if (SD.remove(fullPath)) {
    Serial.printf("[SDActions] Deleted file: %s\n", fullPath);
    return true;
  }
  Serial.printf("[SDActions] Delete failed: %s\n", fullPath);
  return false;
}

bool sdActionsRenameInPlace(const char* fullPathOld, const char* newBaseName) {
  if (!sdIsAvailable() || !fullPathOld || !newBaseName || !newBaseName[0]) return false;
  if (sdActionsIsPathDeleteProtected(fullPathOld)) {
    Serial.printf("[SDActions] Rename blocked (protected): %s\n", fullPathOld);
    return false;
  }
  if (strstr(newBaseName, "..") || strchr(newBaseName, '/')) return false;

  char parent[128];
  strncpy(parent, fullPathOld, sizeof(parent) - 1);
  parent[sizeof(parent) - 1] = '\0';
  char* slash = strrchr(parent, '/');
  if (!slash) return false;
  *slash = '\0';

  char dest[160];
  snprintf(dest, sizeof(dest), "%s/%s", parent, newBaseName);

  if (SD.rename(fullPathOld, dest)) {
    Serial.printf("[SDActions] Renamed -> %s\n", dest);
    return true;
  }
  Serial.println("[SDActions] Rename failed");
  return false;
}

bool sdActionsMkdirOneLevel(const char* absolutePath) {
  if (!sdIsAvailable() || !absolutePath) return false;
  if (strncmp(absolutePath, SD_ROOT, strlen(SD_ROOT)) != 0) return false;
  if (SD.exists(absolutePath)) return true;
  if (SD.mkdir(absolutePath)) {
    Serial.printf("[SDActions] mkdir: %s\n", absolutePath);
    return true;
  }
  return false;
}
