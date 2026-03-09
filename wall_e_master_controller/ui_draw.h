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
#define TELEM_STRIP_H   22
#define BOTTOM_BAR_Y    200
#define BOTTOM_BAR_H    40
#define CONTENT_TOP     52
#define CONTENT_H       148

// Single centered joystick
#define JOY_CX          160  // Center X
#define JOY_CY          126  // Center Y (middle of content area)
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
} TelemetryStripData;

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void uiDrawInit(TFT_eSPI* tft);
void uiDrawCurrentPage(void);  // Central: static draw based on InputMode + Page
void uiDrawUpdateDynamic(const TelemetryStripData* telem, const DriveState* ds,
                         int joyDotX, int joyDotY);
void uiDrawTelemetryStrip(const TelemetryStripData* telem);
void uiDrawControlAuthority(void);
void uiDrawQuickActionOverlay(void);  // Calibrate IMU, Reset Motors, Supervised, Reboot
void uiDrawAdvancedModeOverlay(void); // Raw motor %, IMU, CPU, latency
void uiDrawEStopRegion(bool highlighted);
void uiDrawPageBehaviour(void);
void uiDrawPageSystem(void);
void uiDrawPageAutonomy(void);
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
