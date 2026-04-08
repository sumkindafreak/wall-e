// ============================================================
//  WALL-E CYD — SD Explorer + Memory Core page
//  Layout must match touch_input.cpp (PAGE_SD_EXPLORER == 9)
// ============================================================

#ifndef UI_SD_MEMORY_CORE_H
#define UI_SD_MEMORY_CORE_H

#include <stdbool.h>

#define UI_MC_PATH_OFF      2
#define UI_MC_STATUS_OFF    14
#define UI_MC_DETAIL_OFF    24
#define UI_MC_SHORTCUT_Y    36
#define UI_MC_SHORTCUT_H    14
#define UI_MC_LIST_OFF      52
#define UI_MC_LIST_ROWS     4
#define UI_MC_ROW_H         13

void uiDrawPageSdMemoryCore(void);

bool uiSdMemoryCorePreviewIsOpen(void);
bool uiSdMemoryCoreConfirmIsOpen(void);
bool uiSdMemoryCoreRenameIsOpen(void);
void uiSdMemoryCoreClosePreview(void);
void uiSdMemoryCoreCloseConfirm(void);
void uiSdMemoryCoreCloseRename(void);
void uiSdMemoryCoreRequestDeleteConfirm(void);
void uiSdMemoryCoreRequestRename(void);
bool uiSdMemoryCoreTryOpenPreview(void);

void uiSdMemoryCoreRenameChrDec(void);
void uiSdMemoryCoreRenameChrInc(void);
void uiSdMemoryCoreRenameBksp(void);
void uiSdMemoryCoreRenameCurLeft(void);
void uiSdMemoryCoreRenameCurRight(void);
/** Apply new name; closes modal on success */
bool uiSdMemoryCoreRenameApplyOk(void);

#endif
