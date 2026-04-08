// ============================================================
//  WALL-E CYD — SD card explorer page (draw + UI state)
// ============================================================

#ifndef UI_SD_EXPLORER_H
#define UI_SD_EXPLORER_H

#include <stdbool.h>

/** Layout: see ui_sd_memory_core.h (Memory Core page) */

void uiDrawPageSdExplorer(void);

/** True while text preview is shown (Back returns to list) */
bool uiSdExplorerPreviewIsOpen(void);
/** True while delete confirmation is shown */
bool uiSdExplorerConfirmIsOpen(void);
/** True while rename modal is shown */
bool uiSdExplorerRenameIsOpen(void);

void uiSdExplorerClosePreview(void);
void uiSdExplorerCloseConfirm(void);
void uiSdExplorerCloseRename(void);

/** User tapped Delete — show confirm (if file selected) */
void uiSdExplorerRequestDeleteConfirm(void);
void uiSdExplorerRequestRename(void);
void uiSdExplorerRenameChrDec(void);
void uiSdExplorerRenameChrInc(void);
void uiSdExplorerRenameBksp(void);
void uiSdExplorerRenameCurLeft(void);
void uiSdExplorerRenameCurRight(void);
bool uiSdExplorerRenameApplyOk(void);

/** Load preview for currently selected file (text types only) */
bool uiSdExplorerTryOpenPreview(void);

#endif
