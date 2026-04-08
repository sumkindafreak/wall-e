// ============================================================
//  WALL-E CYD — Safe SD file actions (delete protection)
// ============================================================

#ifndef SD_ACTIONS_H
#define SD_ACTIONS_H

#include <stdbool.h>
#include <stddef.h>

/** True if path must not be deleted from the Memory Core UI */
bool sdActionsIsPathDeleteProtected(const char* fullPath);

/** Remove file; returns false if protected or I/O error */
bool sdActionsDeleteFile(const char* fullPath);

/** Optional: rename file within same parent (safe names only) */
bool sdActionsRenameInPlace(const char* fullPathOld, const char* newBaseName);

/** Create a single directory (one level under existing parent) */
bool sdActionsMkdirOneLevel(const char* absolutePath);

#endif
