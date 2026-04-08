// ============================================================
//  WALL-E CYD — Memory Core + SD browser UI
// ============================================================

#include "ui_sd_memory_core.h"
#include "ui_draw.h"
#include "ui_state.h"
#include "ui_confirm_dialog.h"
#include "sd_browser.h"
#include "sd_manager.h"
#include "file_preview.h"
#include "sd_actions.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

static bool     s_previewOpen = false;
static bool     s_confirmOpen = false;
static bool     s_renameOpen = false;
static bool     s_previewIsInfo = false;
static char     s_previewBuf[FILE_PREVIEW_MAX_BYTES + 1];

static char     s_renBuf[SD_BROWSER_NAME_MAX + 1];
static uint8_t  s_renCur = 0;

static const char kRenCharset[] =
    " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-";

static int renCharsetIndexOf(char c) {
  const char* p = strchr(kRenCharset, c);
  if (p) return (int)(p - kRenCharset);
  return 0;
}

static void renApplyCharDelta(int delta) {
  size_t len = strlen(s_renBuf);
  int n = (int)(sizeof(kRenCharset) - 1);
  if (s_renCur < len) {
    int i = renCharsetIndexOf(s_renBuf[s_renCur]);
    i = (i + delta) % n;
    if (i < 0) i += n;
    s_renBuf[s_renCur] = kRenCharset[i];
  } else if (len < SD_BROWSER_NAME_MAX && delta != 0) {
    int i = renCharsetIndexOf('a');
    i = (i + delta) % n;
    if (i < 0) i += n;
    s_renBuf[len] = kRenCharset[i];
    s_renBuf[len + 1] = '\0';
  }
}

static void renBksp(void) {
  size_t len = strlen(s_renBuf);
  if (len == 0 || s_renCur == 0) return;
  memmove(s_renBuf + s_renCur - 1, s_renBuf + s_renCur, len - s_renCur + 1);
  s_renCur--;
}

void uiSdMemoryCoreRenameChrDec(void) {
  renApplyCharDelta(-1);
}
void uiSdMemoryCoreRenameChrInc(void) {
  renApplyCharDelta(1);
}
void uiSdMemoryCoreRenameBksp(void) {
  renBksp();
}
void uiSdMemoryCoreRenameCurLeft(void) {
  if (s_renCur > 0) s_renCur--;
}
void uiSdMemoryCoreRenameCurRight(void) {
  size_t len = strlen(s_renBuf);
  if (s_renCur < len) s_renCur++;
}
bool uiSdMemoryCoreRenameApplyOk(void) {
  if (!s_renameOpen || s_renBuf[0] == '\0') return false;
  if (sdBrowserRenameSelectedTo(s_renBuf)) {
    s_renameOpen = false;
    return true;
  }
  return false;
}

static const char* kShortcutLabel[] = { "Mem", "Logs", "Cfg", "Mis", "Diag", "Evt" };

bool uiSdMemoryCorePreviewIsOpen(void) {
  return s_previewOpen;
}

bool uiSdMemoryCoreConfirmIsOpen(void) {
  return s_confirmOpen;
}

bool uiSdMemoryCoreRenameIsOpen(void) {
  return s_renameOpen;
}

void uiSdMemoryCoreClosePreview(void) {
  s_previewOpen = false;
  s_previewIsInfo = false;
}

void uiSdMemoryCoreCloseConfirm(void) {
  s_confirmOpen = false;
}

void uiSdMemoryCoreCloseRename(void) {
  s_renameOpen = false;
}

void uiSdMemoryCoreRequestDeleteConfirm(void) {
  int16_t sel = sdBrowserGetSelected();
  if (sel < 0) {
    Serial.println("[MemCore] Delete: no selection");
    return;
  }
  const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sel);
  if (!e || e->isDir) {
    Serial.println("[MemCore] Delete: folders not supported");
    return;
  }
  char full[160];
  if (!sdBrowserBuildPath(e->name, full, sizeof(full))) return;
  s_confirmOpen = true;
  Serial.printf("[MemCore] Confirm delete: %s\n", e->name);
}

void uiSdMemoryCoreRequestRename(void) {
  s_previewOpen = false;
  s_confirmOpen = false;
  int16_t sel = sdBrowserGetSelected();
  if (sel < 0) {
    Serial.println("[MemCore] Rename: no selection");
    return;
  }
  const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sel);
  if (!e || e->isDir) {
    Serial.println("[MemCore] Rename: folders not supported");
    return;
  }
  char full[160];
  if (!sdBrowserBuildPath(e->name, full, sizeof(full))) return;
  if (sdActionsIsPathDeleteProtected(full)) {
    Serial.println("[MemCore] Rename: protected file");
    return;
  }
  strncpy(s_renBuf, e->name, sizeof(s_renBuf) - 1);
  s_renBuf[sizeof(s_renBuf) - 1] = '\0';
  s_renCur = (uint8_t)strlen(s_renBuf);
  if (s_renCur == 0) {
    s_renBuf[0] = 'a';
    s_renBuf[1] = '\0';
    s_renCur = 1;
  }
  s_renameOpen = true;
  Serial.printf("[MemCore] Rename modal: %s\n", e->name);
}

bool uiSdMemoryCoreTryOpenPreview(void) {
  int16_t sel = sdBrowserGetSelected();
  if (sel < 0) return false;
  const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sel);
  if (!e || e->isDir) return false;

  char full[160];
  if (!sdBrowserBuildPath(e->name, full, sizeof(full))) return false;

  s_previewIsInfo = false;
  size_t got = 0;

  if (filePreviewIsTextExtension(e->name)) {
    if (!filePreviewLoad(full, s_previewBuf, sizeof(s_previewBuf), &got)) {
      snprintf(s_previewBuf, sizeof(s_previewBuf), "Read error.");
      s_previewIsInfo = true;
    }
  } else {
    s_previewIsInfo = true;
    if (!filePreviewLoadInfo(full, s_previewBuf, sizeof(s_previewBuf))) {
      snprintf(s_previewBuf, sizeof(s_previewBuf), "No preview.");
    }
  }
  s_previewOpen = true;
  Serial.println("[MemCore] Preview opened");
  return true;
}

static void drawTruncPath(int y, const char* path) {
  if (!g_tft || !path) return;
  char line[48];
  if (strlen(path) > 40) {
    snprintf(line, sizeof(line), "...%s", path + strlen(path) - 36);
  } else {
    strncpy(line, path, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
  }
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);
  g_tft->setCursor(4, y);
  g_tft->print(line);
}

static void drawShortcuts(int y) {
  if (!g_tft) return;
  const int n = 6;
  const int gap = 2;
  const int totalW = SCREEN_W - 8;
  const int btnW = (totalW - (n - 1) * gap) / n;
  int x = 4;
  for (int i = 0; i < n; i++) {
    g_tft->fillRect(x, y, btnW, UI_MC_SHORTCUT_H - 1, C_BG_DARK);
    g_tft->drawRect(x, y, btnW, UI_MC_SHORTCUT_H - 1, C_BORDER);
    g_tft->setTextColor(C_YELLOW, C_BG_DARK);
    int tw = (int)strlen(kShortcutLabel[i]) * 6;
    g_tft->setCursor(x + (btnW - tw) / 2, y + 4);
    g_tft->print(kShortcutLabel[i]);
    x += btnW + gap;
  }
}

static void drawDetailLine(int y) {
  if (!g_tft) return;
  g_tft->fillRect(4, y, 312, 10, C_BG);
  int16_t sel = sdBrowserGetSelected();
  if (sel < 0 || (uint16_t)sel >= sdBrowserGetEntryCount()) {
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->setCursor(4, y + 2);
    g_tft->print("Select: tap row");
    return;
  }
  const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sel);
  if (!e) return;
  char line[56];
  if (e->isDir) {
    snprintf(line, sizeof(line), "Sel: [dir] %s", e->name);
  } else {
    snprintf(line, sizeof(line), "Sel: %s  (%lu B)", e->name, (unsigned long)e->size);
  }
  if (strlen(line) > 48) line[48] = '\0';
  g_tft->setTextColor(C_GREEN, C_BG);
  g_tft->setCursor(4, y + 2);
  g_tft->print(line);
}

static void drawList(int y0) {
  if (!g_tft) return;
  for (int r = 0; r < UI_MC_LIST_ROWS; r++) {
    uint16_t idx = sdBrowserGetScroll() + (uint16_t)r;
    int rowY = y0 + r * UI_MC_ROW_H;
    bool sel = (sdBrowserGetSelected() >= 0) &&
               (idx == (uint16_t)sdBrowserGetSelected());
    uint16_t bg = sel ? C_ACCENT_DIM : C_BG_DARK;
    g_tft->fillRect(4, rowY, 312, UI_MC_ROW_H - 1, bg);
    g_tft->drawRect(4, rowY, 312, UI_MC_ROW_H - 1, C_BORDER);

    if (idx < sdBrowserGetEntryCount()) {
      const SdDirEntry* e = sdBrowserGetEntry(idx);
      if (e) {
        g_tft->setTextColor(sel ? C_WHITE : C_TEXT_DIM, bg);
        char line[SD_BROWSER_NAME_MAX + 24];
        if (e->isDir) {
          snprintf(line, sizeof(line), "[D] %s", e->name);
        } else {
          snprintf(line, sizeof(line), "%luB %s", (unsigned long)e->size, e->name);
        }
        if (strlen(line) > 46) line[46] = '\0';
        g_tft->setCursor(8, rowY + 3);
        g_tft->print(line);
      }
    } else {
      g_tft->setTextColor(C_TEXT_DIM, bg);
      g_tft->setCursor(8, rowY + 3);
      if (sdBrowserGetEntryCount() == 0 && r == 0) {
        g_tft->print("(empty folder)");
      }
    }
  }
}

static void drawToolbar(int tbY) {
  if (!g_tft) return;
  const char* labels[] = { "Up", "Op", "Rf", "<", ">", "Dl", "Rn" };
  const int n = 7;
  const int gap = 2;
  const int totalW = SCREEN_W - 8;
  const int btnW = (totalW - (n - 1) * gap) / n;
  int x = 4;
  for (int i = 0; i < n; i++) {
    g_tft->fillRect(x, tbY, btnW, 22, C_BG_DARK);
    g_tft->drawRect(x, tbY, btnW, 22, C_BORDER);
    g_tft->setTextColor(C_ACCENT, C_BG_DARK);
    int tw = (int)strlen(labels[i]) * 6;
    g_tft->setCursor(x + (btnW - tw) / 2, tbY + 7);
    g_tft->print(labels[i]);
    x += btnW + gap;
  }
}

static void drawRenModalBtn(int bx, int by, int bw, int gh, const char* lab, uint16_t fill, uint16_t fg, uint16_t bg) {
  if (!g_tft) return;
  g_tft->fillRect(bx, by, bw, gh, fill);
  g_tft->drawRect(bx, by, bw, gh, C_BORDER);
  g_tft->setTextColor(fg, bg);
  int tw = (int)strlen(lab) * 6;
  g_tft->setCursor(bx + (bw - tw) / 2, by + 8);
  g_tft->print(lab);
}

static void drawRenameModal(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  g_tft->fillRect(20, 36, 280, 162, C_BG_DARK);
  g_tft->drawRect(20, 36, 280, 162, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Rename", 32, 42);
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  char show[SD_BROWSER_NAME_MAX + 4];
  strncpy(show, s_renBuf, sizeof(show) - 1);
  show[sizeof(show) - 1] = '\0';
  if (strlen(show) > 40) {
    show[37] = '.';
    show[38] = '.';
    show[39] = '.';
    show[40] = '\0';
  }
  g_tft->setCursor(32, 68);
  g_tft->print(show);
  g_tft->setTextColor(C_TEXT_DIM, C_BG_DARK);
  g_tft->setCursor(32, 80);
  g_tft->print("Chr-/Chr+  Bksp   < >   OK");

  const int r1y = 92;
  const int r2y = 122;
  const int r3y = 152;
  const int bw = 94;
  const int gh = 26;
  const int g = 4;
  const int x0 = 14;
  drawRenModalBtn(x0, r1y, bw, gh, "-", C_BG, C_ACCENT, C_BG);
  drawRenModalBtn(x0 + bw + g, r1y, bw, gh, "+", C_BG, C_ACCENT, C_BG);
  drawRenModalBtn(x0 + 2 * (bw + g), r1y, bw, gh, "Bksp", C_BG, C_ACCENT, C_BG);
  const int row2w = bw * 2 + g;
  const int x2 = (SCREEN_W - row2w) / 2;
  drawRenModalBtn(x2, r2y, bw, gh, "<", C_BG, C_ACCENT, C_BG);
  drawRenModalBtn(x2 + bw + g, r2y, bw, gh, ">", C_BG, C_ACCENT, C_BG);
  drawRenModalBtn(x2, r3y, bw, gh, "Cancel", C_BG, C_ACCENT, C_BG);
  g_tft->fillRect(x2 + bw + g, r3y, bw, gh, C_ACCENT);
  g_tft->drawRect(x2 + bw + g, r3y, bw, gh, C_BORDER);
  g_tft->setTextColor(C_BG, C_ACCENT);
  int twok = 2 * 6;
  g_tft->setCursor(x2 + bw + g + (bw - twok) / 2, r3y + 8);
  g_tft->print("OK");
}

static void drawPreviewScreen(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->setTextSize(2);
  g_tft->drawString(s_previewIsInfo ? "File info" : "Preview", 8, 8);
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  int y = 36;
  const char* p = s_previewBuf;
  while (*p && y < 170) {
    char line[40];
    int i = 0;
    while (*p && *p != '\n' && *p != '\r' && i < 38) line[i++] = *p++;
    line[i] = '\0';
    if (*p == '\n' || *p == '\r') p++;
    g_tft->setCursor(4, y);
    g_tft->print(line);
    y += 10;
  }
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

void uiDrawPageSdMemoryCore(void) {
  if (!g_tft) return;

  if (s_renameOpen) {
    drawRenameModal();
    return;
  }
  if (s_confirmOpen) {
    int16_t sel = sdBrowserGetSelected();
    const char* name = "";
    if (sel >= 0) {
      const SdDirEntry* e = sdBrowserGetEntry((uint16_t)sel);
      if (e) name = e->name;
    }
    uiConfirmDrawDeleteFile(g_tft, name);
    return;
  }
  if (s_previewOpen) {
    drawPreviewScreen();
    return;
  }

  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);

  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Memory Core", 8, 6);
  }

  const int c = uiContentTop();
  drawTruncPath(c + UI_MC_PATH_OFF, sdBrowserGetCwd());

  char stat[56];
  if (sdIsAvailable()) {
    uint32_t freeMb = sdGetFreeSpaceMB();
    uint32_t totMb = sdGetTotalSpaceMB();
    if (sdBrowserIsListingTruncated()) {
      snprintf(stat, sizeof(stat), "SD OK f~%lu t~%lu MB  %u of %lu+",
               (unsigned long)freeMb, (unsigned long)totMb,
               (unsigned)sdBrowserGetEntryCount(),
               (unsigned long)sdBrowserGetListingTotalCount());
    } else {
      snprintf(stat, sizeof(stat), "SD OK  free~%lu MB  total~%lu MB  Items:%u",
               (unsigned long)freeMb, (unsigned long)totMb,
               (unsigned)sdBrowserGetEntryCount());
    }
  } else {
    strncpy(stat, "NO SD CARD", sizeof(stat));
  }
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->setCursor(4, c + UI_MC_STATUS_OFF);
  g_tft->print(stat);

  if (!sdIsAvailable()) {
    g_tft->setTextColor(C_RED, C_BG);
    g_tft->setCursor(4, c + 40);
    g_tft->print("Insert card or check wiring.");
  } else {
    drawDetailLine(c + UI_MC_DETAIL_OFF);
    drawShortcuts(c + UI_MC_SHORTCUT_Y);
    drawList(c + UI_MC_LIST_OFF);
    const int tbY = c + UI_MC_LIST_OFF + UI_MC_LIST_ROWS * UI_MC_ROW_H + 4;
    drawToolbar(tbY);
  }

  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}
