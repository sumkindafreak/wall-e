// ============================================================
//  WALL-E CYD — small text file preview (bounded buffer)
// ============================================================

#ifndef FILE_PREVIEW_H
#define FILE_PREVIEW_H

#include <stddef.h>
#include <stdbool.h>

#define FILE_PREVIEW_MAX_BYTES  900

/** True if extension is suitable for text preview */
bool filePreviewIsTextExtension(const char* filename);

/** Read up to FILE_PREVIEW_MAX_BYTES-1 into buf (NUL-terminated). */
bool filePreviewLoad(const char* fullPath, char* buf, size_t bufCap, size_t* outLen);

/** Fill buf with human-readable info for non-text files (size, note). */
bool filePreviewLoadInfo(const char* fullPath, char* buf, size_t bufCap);

#endif
