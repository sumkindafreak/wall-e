// ============================================================
//  WALL-E CYD — Legacy API wrappers → ui_sd_memory_core
// ============================================================

#include "ui_sd_explorer.h"
#include "ui_sd_memory_core.h"

void uiDrawPageSdExplorer(void) {
  uiDrawPageSdMemoryCore();
}

bool uiSdExplorerPreviewIsOpen(void) {
  return uiSdMemoryCorePreviewIsOpen();
}

bool uiSdExplorerConfirmIsOpen(void) {
  return uiSdMemoryCoreConfirmIsOpen();
}

bool uiSdExplorerRenameIsOpen(void) {
  return uiSdMemoryCoreRenameIsOpen();
}

void uiSdExplorerClosePreview(void) {
  uiSdMemoryCoreClosePreview();
}

void uiSdExplorerCloseConfirm(void) {
  uiSdMemoryCoreCloseConfirm();
}

void uiSdExplorerCloseRename(void) {
  uiSdMemoryCoreCloseRename();
}

void uiSdExplorerRequestDeleteConfirm(void) {
  uiSdMemoryCoreRequestDeleteConfirm();
}

bool uiSdExplorerTryOpenPreview(void) {
  return uiSdMemoryCoreTryOpenPreview();
}

void uiSdExplorerRequestRename(void) {
  uiSdMemoryCoreRequestRename();
}

void uiSdExplorerRenameChrDec(void) {
  uiSdMemoryCoreRenameChrDec();
}
void uiSdExplorerRenameChrInc(void) {
  uiSdMemoryCoreRenameChrInc();
}
void uiSdExplorerRenameBksp(void) {
  uiSdMemoryCoreRenameBksp();
}
void uiSdExplorerRenameCurLeft(void) {
  uiSdMemoryCoreRenameCurLeft();
}
void uiSdExplorerRenameCurRight(void) {
  uiSdMemoryCoreRenameCurRight();
}
bool uiSdExplorerRenameApplyOk(void) {
  return uiSdMemoryCoreRenameApplyOk();
}
