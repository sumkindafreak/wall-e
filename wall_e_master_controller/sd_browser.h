// ============================================================
//  WALL-E CYD — SD directory browser (path jail, list, navigate)
// ============================================================

#ifndef SD_BROWSER_H
#define SD_BROWSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SD_BROWSER_MAX_ENTRIES  64
#define SD_BROWSER_NAME_MAX     48
#define SD_BROWSER_VISIBLE_ROWS   4

typedef struct {
  char     name[SD_BROWSER_NAME_MAX + 1];
  uint32_t size;
  bool     isDir;
} SdDirEntry;

/** Call when opening SD explorer — restores last cwd from Preferences (or /wall_e), refresh */
void sdBrowserOnEnterPage(void);

/** Reload current directory into s_entries */
bool sdBrowserRefresh(void);

const char* sdBrowserGetCwd(void);

/** True if last refresh hit the entry cap (only first SD_BROWSER_MAX_ENTRIES shown) */
bool sdBrowserIsListingTruncated(void);
/** Total entries in folder (>= entry count when truncated) */
uint32_t sdBrowserGetListingTotalCount(void);

uint16_t sdBrowserGetEntryCount(void);
uint16_t sdBrowserGetScroll(void);
void     sdBrowserSetScroll(uint16_t s);

/** Selected row index in full list, or -1 */
int16_t  sdBrowserGetSelected(void);
void     sdBrowserSetSelected(int16_t idx);

const SdDirEntry* sdBrowserGetEntry(uint16_t i);

/** Parent directory (stays inside jail) */
bool sdBrowserGoUp(void);

/** Enter selected folder — false if not a dir or error */
bool sdBrowserEnterSelected(void);

/** Build "/wall_e/..." full path for named child */
bool sdBrowserBuildPath(const char* name, char* out, size_t outLen);

/** Delete selected file (not dirs). Caller confirms. Uses sd_actions protection. */
bool sdBrowserDeleteSelected(void);

/** Rename selected file to new basename (same folder). Not for dirs. */
bool sdBrowserRenameSelectedTo(const char* newBaseName);

/** Jump cwd to absolute path under /wall_e (creates dir if missing). */
bool sdBrowserNavigateToAbsolute(const char* absolutePath);

#endif
