// ============================================================
//  WALL-E CYD — SD directory browser implementation
// ============================================================

#include "sd_browser.h"
#include "sd_manager.h"
#include "sd_actions.h"

#include <SD.h>
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>
#include <strings.h>

static char       s_cwd[128];
static SdDirEntry s_entries[SD_BROWSER_MAX_ENTRIES];
static uint16_t   s_count = 0;
static uint32_t   s_dirTotal = 0;
static bool       s_truncated = false;
static uint16_t   s_scroll = 0;
static int16_t    s_selected = -1;

static const char kPrefsNs[] = "sdmem";
static const char kPrefsCwd[] = "cwd";

static bool isPathAllowed(const char* path) {
  if (!path || path[0] != '/') return false;
  if (strncmp(path, SD_ROOT, strlen(SD_ROOT)) != 0) return false;
  if (strstr(path, "..")) return false;
  return true;
}

static void persistLastCwd(void) {
  if (!sdIsAvailable() || !isPathAllowed(s_cwd)) return;
  Preferences prefs;
  if (!prefs.begin(kPrefsNs, false)) return;
  prefs.putString(kPrefsCwd, s_cwd);
  prefs.end();
}

static bool trySetCwdToExistingDir(const char* path) {
  if (!sdIsAvailable() || !path || !isPathAllowed(path)) return false;
  if (!SD.exists(path)) return false;
  File t = SD.open(path);
  if (!t) return false;
  if (!t.isDirectory()) {
    t.close();
    return false;
  }
  t.close();
  strncpy(s_cwd, path, sizeof(s_cwd) - 1);
  s_cwd[sizeof(s_cwd) - 1] = '\0';
  return true;
}

static void sortEntries(void) {
  for (uint16_t i = 0; i + 1 < s_count; i++) {
    for (uint16_t j = i + 1; j < s_count; j++) {
      SdDirEntry* a = &s_entries[i];
      SdDirEntry* b = &s_entries[j];
      bool swap = false;
      if (a->isDir != b->isDir) {
        if (!a->isDir && b->isDir) swap = true;
      } else {
        if (strcasecmp(a->name, b->name) > 0) swap = true;
      }
      if (swap) {
        SdDirEntry t = *a;
        *a = *b;
        *b = t;
      }
    }
  }
}

bool sdBrowserRefresh(void) {
  s_count = 0;
  s_dirTotal = 0;
  s_truncated = false;
  s_selected = -1;
  if (!sdIsAvailable()) {
    Serial.println("[SDBrowser] No SD card");
    return false;
  }
  if (!isPathAllowed(s_cwd)) {
    Serial.printf("[SDBrowser] Bad path: %s\n", s_cwd);
    return false;
  }

  File dir = SD.open(s_cwd);
  if (!dir) {
    Serial.printf("[SDBrowser] open failed: %s\n", s_cwd);
    return false;
  }
  if (!dir.isDirectory()) {
    dir.close();
    Serial.printf("[SDBrowser] Not a directory: %s\n", s_cwd);
    return false;
  }

  File child = dir.openNextFile();
  uint32_t idx = 0;
  while (child) {
    idx++;
    if ((idx & 7) == 0) {
      yield();
    }
    const char* nm = child.name();
    if (!nm || nm[0] == '\0') {
      child.close();
      child = dir.openNextFile();
      continue;
    }
    const char* base = strrchr(nm, '/');
    base = base ? base + 1 : nm;

    if (s_count < SD_BROWSER_MAX_ENTRIES) {
      SdDirEntry* e = &s_entries[s_count];
      strncpy(e->name, base, SD_BROWSER_NAME_MAX);
      e->name[SD_BROWSER_NAME_MAX] = '\0';
      if (child.isDirectory()) {
        e->isDir = true;
        e->size = 0;
      } else {
        e->isDir = false;
        e->size = (uint32_t)child.size();
      }
      s_count++;
    } else {
      s_truncated = true;
    }
    child.close();
    child = dir.openNextFile();
  }
  dir.close();

  s_dirTotal = idx;
  if (s_truncated) {
    Serial.printf("[SDBrowser] WARNING: folder has %lu entries; showing first %u only\n",
                  (unsigned long)s_dirTotal, (unsigned)SD_BROWSER_MAX_ENTRIES);
  }

  sortEntries();
  if (s_scroll >= s_count) s_scroll = 0;
  Serial.printf("[SDBrowser] Listed %u items in %s\n", (unsigned)s_count, s_cwd);
  return true;
}

void sdBrowserOnEnterPage(void) {
  s_scroll = 0;
  s_selected = -1;

  bool restored = false;
  if (sdIsAvailable()) {
    Preferences prefs;
    if (prefs.begin(kPrefsNs, true)) {
      String stored = prefs.getString(kPrefsCwd, "");
      prefs.end();
      if (stored.length() > 0 && trySetCwdToExistingDir(stored.c_str())) {
        restored = true;
      }
    }
  }
  if (!restored) {
    strncpy(s_cwd, SD_ROOT, sizeof(s_cwd) - 1);
    s_cwd[sizeof(s_cwd) - 1] = '\0';
  }

  sdBrowserRefresh();
  persistLastCwd();
  if (restored) {
    Serial.printf("[SDBrowser] Restored cwd: %s\n", s_cwd);
  }
}

const char* sdBrowserGetCwd(void) {
  return s_cwd;
}

bool sdBrowserIsListingTruncated(void) {
  return s_truncated;
}

uint32_t sdBrowserGetListingTotalCount(void) {
  return s_dirTotal;
}

uint16_t sdBrowserGetEntryCount(void) {
  return s_count;
}

uint16_t sdBrowserGetScroll(void) {
  return s_scroll;
}

void sdBrowserSetScroll(uint16_t s) {
  if (s_count == 0) {
    s_scroll = 0;
    return;
  }
  uint16_t maxScr = (s_count > SD_BROWSER_VISIBLE_ROWS) ? (s_count - SD_BROWSER_VISIBLE_ROWS) : 0;
  if (s > maxScr) s = maxScr;
  s_scroll = s;
}

int16_t sdBrowserGetSelected(void) {
  return s_selected;
}

void sdBrowserSetSelected(int16_t idx) {
  if (idx < 0 || (uint16_t)idx >= s_count) {
    s_selected = -1;
    return;
  }
  s_selected = idx;
}

const SdDirEntry* sdBrowserGetEntry(uint16_t i) {
  if (i >= s_count) return nullptr;
  return &s_entries[i];
}

bool sdBrowserBuildPath(const char* name, char* out, size_t outLen) {
  if (!name || !out || outLen < 8) return false;
  int n = snprintf(out, outLen, "%s/%s", s_cwd, name);
  return n > 0 && (size_t)n < outLen && isPathAllowed(out);
}

bool sdBrowserGoUp(void) {
  if (strcmp(s_cwd, SD_ROOT) == 0) {
    Serial.println("[SDBrowser] Already at jail root");
    return false;
  }
  char* slash = strrchr(s_cwd, '/');
  if (slash && slash != s_cwd) {
    *slash = '\0';
  } else {
    strncpy(s_cwd, SD_ROOT, sizeof(s_cwd) - 1);
    s_cwd[sizeof(s_cwd) - 1] = '\0';
  }
  if (!isPathAllowed(s_cwd)) {
    strncpy(s_cwd, SD_ROOT, sizeof(s_cwd) - 1);
    s_cwd[sizeof(s_cwd) - 1] = '\0';
  }
  s_scroll = 0;
  s_selected = -1;
  sdBrowserRefresh();
  persistLastCwd();
  Serial.printf("[SDBrowser] Up -> %s\n", s_cwd);
  return true;
}

bool sdBrowserEnterSelected(void) {
  if (s_selected < 0 || (uint16_t)s_selected >= s_count) return false;
  const SdDirEntry* e = &s_entries[s_selected];
  if (!e->isDir) return false;

  char next[128];
  if (!sdBrowserBuildPath(e->name, next, sizeof(next))) return false;
  if (!isPathAllowed(next)) return false;

  strncpy(s_cwd, next, sizeof(s_cwd) - 1);
  s_cwd[sizeof(s_cwd) - 1] = '\0';
  s_scroll = 0;
  s_selected = -1;
  sdBrowserRefresh();
  persistLastCwd();
  Serial.printf("[SDBrowser] Enter dir -> %s\n", s_cwd);
  return true;
}

bool sdBrowserDeleteSelected(void) {
  if (!sdIsAvailable()) return false;
  if (s_selected < 0 || (uint16_t)s_selected >= s_count) return false;
  const SdDirEntry* e = &s_entries[s_selected];
  if (e->isDir) {
    Serial.println("[SDBrowser] Delete folder not supported");
    return false;
  }
  char full[160];
  if (!sdBrowserBuildPath(e->name, full, sizeof(full))) return false;

  if (sdActionsDeleteFile(full)) {
    sdBrowserRefresh();
    return true;
  }
  return false;
}

bool sdBrowserRenameSelectedTo(const char* newBaseName) {
  if (!sdIsAvailable() || !newBaseName || !newBaseName[0]) return false;
  if (s_selected < 0 || (uint16_t)s_selected >= s_count) return false;
  const SdDirEntry* e = &s_entries[s_selected];
  if (e->isDir) {
    Serial.println("[SDBrowser] Rename folder not supported");
    return false;
  }
  char full[160];
  if (!sdBrowserBuildPath(e->name, full, sizeof(full))) return false;

  if (sdActionsRenameInPlace(full, newBaseName)) {
    sdBrowserRefresh();
    persistLastCwd();
    return true;
  }
  return false;
}

bool sdBrowserNavigateToAbsolute(const char* path) {
  if (!sdIsAvailable() || !path) return false;
  if (!isPathAllowed(path)) {
    Serial.printf("[SDBrowser] Navigate denied: %s\n", path);
    return false;
  }

  if (!SD.exists(path)) {
    if (!sdActionsMkdirOneLevel(path)) {
      Serial.printf("[SDBrowser] Cannot create: %s\n", path);
      return false;
    }
  }

  File t = SD.open(path);
  if (!t) {
    Serial.printf("[SDBrowser] open failed: %s\n", path);
    return false;
  }
  if (!t.isDirectory()) {
    t.close();
    Serial.printf("[SDBrowser] Not a directory: %s\n", path);
    return false;
  }
  t.close();

  strncpy(s_cwd, path, sizeof(s_cwd) - 1);
  s_cwd[sizeof(s_cwd) - 1] = '\0';
  s_scroll = 0;
  s_selected = -1;
  Serial.printf("[SDBrowser] Jump -> %s\n", path);
  bool ok = sdBrowserRefresh();
  if (ok) persistLastCwd();
  return ok;
}
