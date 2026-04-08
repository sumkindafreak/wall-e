// ============================================================
//  WALL-E Master Controller — UI Draw
//  Layered static/dynamic rendering, regions, zero-flicker
// ============================================================

#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <TFT_eSPI.h>
#include "ui_state.h"
#include "protocol.h"

// ------------------------------------------------------------
//  UI Regions — SINGLE CENTERED JOYSTICK
// ------------------------------------------------------------
#define SCREEN_W        320
#define SCREEN_H        240
#define TOP_BAR_HEIGHT  30
/* Two text rows (nums + mode|emo) under battery bar — needs >22px */
#define TELEM_STRIP_H   30
/** Collapsed banner height (single-line status; tap banner to expand) */
#define BANNER_MINI_H   16
/** Max first Y when banner fully expanded (use uiContentTop() at runtime) */
#define CONTENT_TOP_MAX (TOP_BAR_HEIGHT + TELEM_STRIP_H)
#define BOTTOM_BAR_Y    200
#define BOTTOM_BAR_H    40
/** Bottom toast when Brain autonomy is in a “busy mind” state — keep in sync with uiDrawThinkingStrip */
#define UI_THINKING_STRIP_Y   (BOTTOM_BAR_Y - 24)
#define UI_THINKING_STRIP_H   24
/** Drive bottom bar — Dock/Cancel/E-STOP slightly narrower so four nav tiles fit (320px wide) */
#define DRIVE_DOCK_X        8
#define DRIVE_DOCK_W        56
#define DRIVE_CANCEL_X      (DRIVE_DOCK_X + DRIVE_DOCK_W + 4)
#define DRIVE_CANCEL_W      56
#define DRIVE_ESTOP_X       (DRIVE_CANCEL_X + DRIVE_CANCEL_W + 4)
#define DRIVE_ESTOP_W       78
#define DRIVE_NAV_GRID_X    (DRIVE_ESTOP_X + DRIVE_ESTOP_W + 4)
#define DRIVE_NAV_CELL_W    26  /* 4×26 + 3×2 gap = 110px; grid 210+110=320 */
#define DRIVE_NAV_GAP       2
#define DRIVE_NAV_BEH_X     (DRIVE_NAV_GRID_X + DRIVE_NAV_CELL_W + DRIVE_NAV_GAP)
#define DRIVE_NAV_PRF_X     (DRIVE_NAV_BEH_X + DRIVE_NAV_CELL_W + DRIVE_NAV_GAP)
#define DRIVE_NAV_AUT_X     (DRIVE_NAV_PRF_X + DRIVE_NAV_CELL_W + DRIVE_NAV_GAP)
#define DRIVE_BOTTOM_BTN_Y  (BOTTOM_BAR_Y + 4)
#define DRIVE_BOTTOM_BTN_H  32
#define CONTENT_H_MAX   (BOTTOM_BAR_Y - CONTENT_TOP_MAX)

// Single centered joystick
#define JOY_CX          160  // Center X
#define JOY_CY           95  // Center Y (above bottom bar; +15px vs old layout)
#define JOY_RADIUS      70   // Larger radius

#define GRID_SPACING    20
#define EYELET_X        296
#define EYELET_Y        8
#define EYELET_W        24
#define EYELET_H        16

// ------------------------------------------------------------
//  Colors — industrial graphite + amber
// ------------------------------------------------------------
#define C_BG        0x0000
#define C_BG_DARK   0x18C3
#define C_GRID      0x18C3
#define C_BORDER    0x3186
#define C_ACCENT    0xFD20
#define C_ACCENT_DIM 0xB360
#define C_RED       0xF800
#define C_GREEN     0x07E0
#define C_BLUE      0x001F
#define C_YELLOW    0xFFE0
#define C_WHITE     0xFFFF
#define C_TEXT_DIM  0xAD55

// ------------------------------------------------------------
//  Telemetry strip data (update when changed)
// ------------------------------------------------------------
typedef struct {
  float  batteryV;
  int    batteryPct;
  float  currentA;
  float  tempC;
  uint16_t packetRate;
  int8_t  rssi;        // Signal strength (or 0)
  bool   connected;
  const char* modeStr; // MANUAL / AUTO / SUPERVISED
  const char* emotionStr; /**< Emotion engine label, e.g. "CURIOUS" */
} TelemetryStripData;

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void uiDrawInit(TFT_eSPI* tft);

/** Runtime banner height: full title+telemetry or thin mini strip */
int uiBannerTotalHeight(void);
int uiContentTop(void);
int uiContentHeight(void);
void uiBannerInvalidateTelemetryCache(void);
void uiDrawBannerBackground(void);
void uiDrawCurrentPage(void);  // Central: static draw based on InputMode + Page
void uiDrawUpdateDynamic(const TelemetryStripData* telem, const DriveState* ds,
                         int joyDotX, int joyDotY);
void uiDrawTelemetryStrip(const TelemetryStripData* telem);
/** @param brainLinkOk true when recent TelemetryPacket from Base (ESP-NOW) */
void uiDrawControlAuthority(bool brainLinkOk);
void uiDrawQuickActionOverlay(void);  // Calibrate IMU, Reset Motors, Supervised, Reboot
void uiDrawAdvancedModeOverlay(void); // Raw motor %, IMU, CPU, latency
void uiDrawEStopRegion(bool highlighted);
void uiDrawPageBehaviour(void);
void uiDrawPageSystem(void);
void uiDrawPageAutonomy(void);
/** Shown above bottom bar when Base reports autonomy “thinking”-like states */
void uiDrawThinkingStrip(const TelemetryPacket* tm, bool linkOk);
void uiDrawPageHelp(void);
void uiDrawPageSdExplorer(void);
void uiDrawStaticDrive(void);  // Touchscreen Drive page static
void uiDrawStaticBehaviour(void);
void uiDrawStaticSystem(void);
void uiDrawStaticProfile(void);  // NEW: Profile selection page
void uiDrawStaticServoEditor(void);  // NEW: Servo tuning page
void uiDrawStaticServoTest(void);    // NEW: Individual servo test page
#if USE_PHYSICAL_JOYSTICKS
void uiDrawPhysicalJoystickLayout(void);  // Battery graph left, behaviour right
#endif

// Physical joystick visual feedback
void uiDrawPhysicalJoystickIndicators(float joy1X, float joy1Y, float joy2X, float joy2Y);

extern TFT_eSPI* g_tft;

#endif // UI_DRAW_H
