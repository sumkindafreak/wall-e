// ============================================================
//  WALL-E CYD Performance Console — Developer Console Implementation
// ============================================================

#include "dev_console.h"
#include "sd_manager.h"
#include <ESP.h>

// ============================================================
//  Unlock Pattern (Top-right corner hold for 3 seconds)
// ============================================================

#define UNLOCK_X_MIN  400
#define UNLOCK_Y_MIN  0
#define UNLOCK_Y_MAX  40
#define UNLOCK_HOLD_MS  3000

// ============================================================
//  Internal State
// ============================================================

static bool s_unlocked = false;
static DevConsolePage s_currentPage = DEV_OVERVIEW;
static uint32_t s_unlockHoldStart = 0;
static bool s_unlockHolding = false;

// Data buffers for live monitoring
#define SERVO_HISTORY_SIZE  40  // Reduced from 80 to save RAM
static float s_servoHistory[9][SERVO_HISTORY_SIZE];
static uint8_t s_servoHistoryIdx = 0;

static uint32_t s_lastPacketInterval = 0;
static uint32_t s_lastLoopTime = 0;

static float s_lastSonar = 0.0f;
static float s_lastCompass = 0.0f;
static bool s_lastGPSValid = false;

// ============================================================
//  Colors
// ============================================================

#define COL_BG       0x0000
#define COL_TEXT     0xFFFF
#define COL_ACCENT   0x07E0  // Green
#define COL_WARNING  0xFBE0  // Yellow
#define COL_ERROR    0xF800  // Red
#define COL_GRID     0x4208  // Dark gray

// ============================================================
//  Initialization
// ============================================================

void devConsoleInit() {
  s_unlocked = false;
  s_currentPage = DEV_OVERVIEW;
  memset(s_servoHistory, 0, sizeof(s_servoHistory));
  Serial.println("[DevConsole] Ready - unlock with 3s hold (top-right)");
}

// ============================================================
//  Unlock Logic
// ============================================================

bool devConsoleIsUnlocked() {
  return s_unlocked;
}

void devConsoleCheckUnlock(uint16_t x, uint16_t y, uint32_t holdTimeMs) {
  // Check if touch is in unlock zone
  if (x >= UNLOCK_X_MIN && y >= UNLOCK_Y_MIN && y <= UNLOCK_Y_MAX) {
    if (!s_unlockHolding) {
      s_unlockHolding = true;
      s_unlockHoldStart = millis();
    } else if (millis() - s_unlockHoldStart >= UNLOCK_HOLD_MS) {
      s_unlocked = !s_unlocked;
      s_unlockHolding = false;
      Serial.printf("[DevConsole] %s\n", s_unlocked ? "UNLOCKED" : "LOCKED");
    }
  } else {
    s_unlockHolding = false;
  }
}

void devConsoleLock() {
  s_unlocked = false;
  Serial.println("[DevConsole] Locked");
}

// ============================================================
//  Data Feeding
// ============================================================

void devConsoleFeedServoData(const float servos[9]) {
  for (int i = 0; i < 9; i++) {
    s_servoHistory[i][s_servoHistoryIdx] = servos[i];
  }
  s_servoHistoryIdx = (s_servoHistoryIdx + 1) % SERVO_HISTORY_SIZE;
}

void devConsoleFeedPacketTiming(uint32_t sendIntervalUs, uint32_t loopTimeUs) {
  s_lastPacketInterval = sendIntervalUs;
  s_lastLoopTime = loopTimeUs;
}

void devConsoleFeedSensorData(float sonar, float compass, bool gpsValid) {
  s_lastSonar = sonar;
  s_lastCompass = compass;
  s_lastGPSValid = gpsValid;
}

// ============================================================
//  Navigation
// ============================================================

void devConsoleNextPage() {
  s_currentPage = (DevConsolePage)((s_currentPage + 1) % DEV_PAGE_COUNT);
}

void devConsolePrevPage() {
  if (s_currentPage == 0) {
    s_currentPage = (DevConsolePage)(DEV_PAGE_COUNT - 1);
  } else {
    s_currentPage = (DevConsolePage)(s_currentPage - 1);
  }
}

DevConsolePage devConsoleGetPage() {
  return s_currentPage;
}

// ============================================================
//  Drawing Helpers
// ============================================================

static void drawHeader(TFT_eSPI* tft, const char* title) {
  tft->fillRect(0, 0, 480, 30, COL_ACCENT);
  tft->setTextColor(COL_BG, COL_ACCENT);
  tft->setTextSize(2);
  tft->setCursor(10, 8);
  tft->print("DEV: ");
  tft->print(title);
}

static void drawText(TFT_eSPI* tft, int16_t x, int16_t y, const char* label, const char* value, uint16_t color = COL_TEXT) {
  tft->setTextColor(COL_TEXT, COL_BG);
  tft->setTextSize(1);
  tft->setCursor(x, y);
  tft->print(label);
  tft->setTextColor(color, COL_BG);
  tft->print(value);
}

static void drawServoGraph(TFT_eSPI* tft, int16_t x, int16_t y, int16_t w, int16_t h) {
  // Draw grid
  tft->drawRect(x, y, w, h, COL_GRID);
  tft->drawLine(x, y + h/2, x + w, y + h/2, COL_GRID);  // Center line
  
  // Draw servo data (last 80 samples)
  uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF, 0xFFFF, 0xFBE0, 0xFD20};
  
  for (int servo = 0; servo < 9; servo++) {
    for (int i = 1; i < SERVO_HISTORY_SIZE; i++) {
      int idx1 = (s_servoHistoryIdx + i - 1) % SERVO_HISTORY_SIZE;
      int idx2 = (s_servoHistoryIdx + i) % SERVO_HISTORY_SIZE;
      
      float val1 = s_servoHistory[servo][idx1];
      float val2 = s_servoHistory[servo][idx2];
      
      // Map 0-100 to graph height
      int16_t y1 = y + h - (int16_t)((val1 / 100.0f) * h);
      int16_t y2 = y + h - (int16_t)((val2 / 100.0f) * h);
      int16_t x1 = x + (i - 1) * w / SERVO_HISTORY_SIZE;
      int16_t x2 = x + i * w / SERVO_HISTORY_SIZE;
      
      tft->drawLine(x1, y1, x2, y2, colors[servo % 9]);
    }
  }
}

// ============================================================
//  Page Drawing
// ============================================================

static void drawOverviewPage(TFT_eSPI* tft) {
  drawHeader(tft, "System Overview");
  
  // Memory info
  char buf[64];
  uint32_t freeMem = ESP.getFreeHeap();
  uint32_t totalMem = ESP.getHeapSize();
  snprintf(buf, sizeof(buf), "%u KB / %u KB", freeMem / 1024, totalMem / 1024);
  drawText(tft, 10, 40, "Heap: ", buf, freeMem < 20000 ? COL_WARNING : COL_ACCENT);
  
  // Packet timing
  snprintf(buf, sizeof(buf), "%u us", s_lastPacketInterval);
  drawText(tft, 10, 60, "Packet Interval: ", buf);
  
  // Loop time
  snprintf(buf, sizeof(buf), "%u us", s_lastLoopTime);
  drawText(tft, 10, 80, "Loop Time: ", buf, s_lastLoopTime > 20000 ? COL_WARNING : COL_ACCENT);
  
  // SD status
  if (sdIsAvailable()) {
    snprintf(buf, sizeof(buf), "%u MB free", sdGetFreeSpaceMB());
    drawText(tft, 10, 100, "SD Card: ", buf, COL_ACCENT);
  } else {
    drawText(tft, 10, 100, "SD Card: ", "NOT AVAILABLE", COL_ERROR);
  }
  
  // Uptime
  uint32_t uptime = millis() / 1000;
  snprintf(buf, sizeof(buf), "%u:%02u:%02u", uptime / 3600, (uptime % 3600) / 60, uptime % 60);
  drawText(tft, 10, 120, "Uptime: ", buf);
}

static void drawServoGraphPage(TFT_eSPI* tft) {
  drawHeader(tft, "Servo Live Graph");
  
  // Draw legend
  tft->setTextSize(1);
  const char* servoNames[] = {"Neck", "Head", "EyeL", "EyeR", "BrowL", "BrowR", "ArmL", "ArmR", "Extra"};
  uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF, 0xFFFF, 0xFBE0, 0xFD20};
  
  for (int i = 0; i < 9; i++) {
    tft->setTextColor(colors[i], COL_BG);
    tft->setCursor(10 + (i % 3) * 150, 40 + (i / 3) * 12);
    tft->print(servoNames[i]);
  }
  
  // Draw graph
  drawServoGraph(tft, 10, 80, 460, 150);
}

static void drawPacketTimingPage(TFT_eSPI* tft) {
  drawHeader(tft, "Packet Timing");
  
  char buf[64];
  
  // Target vs actual
  snprintf(buf, sizeof(buf), "20000 us (50Hz)");
  drawText(tft, 10, 40, "Target: ", buf);
  
  snprintf(buf, sizeof(buf), "%u us", s_lastPacketInterval);
  uint16_t color = COL_ACCENT;
  if (s_lastPacketInterval > 21000 || s_lastPacketInterval < 19000) color = COL_WARNING;
  drawText(tft, 10, 60, "Actual: ", buf, color);
  
  // Frequency
  if (s_lastPacketInterval > 0) {
    float freq = 1000000.0f / s_lastPacketInterval;
    snprintf(buf, sizeof(buf), "%.1f Hz", freq);
    drawText(tft, 10, 80, "Frequency: ", buf);
  }
  
  // Jitter
  int32_t jitter = (int32_t)s_lastPacketInterval - 20000;
  snprintf(buf, sizeof(buf), "%+d us", jitter);
  drawText(tft, 10, 100, "Jitter: ", buf, abs(jitter) > 1000 ? COL_WARNING : COL_ACCENT);
}

static void drawMemoryPage(TFT_eSPI* tft) {
  drawHeader(tft, "Memory Status");
  
  char buf[64];
  
  // Heap
  uint32_t freeMem = ESP.getFreeHeap();
  uint32_t totalMem = ESP.getHeapSize();
  uint32_t minMem = ESP.getMinFreeHeap();
  
  snprintf(buf, sizeof(buf), "%u KB", totalMem / 1024);
  drawText(tft, 10, 40, "Total Heap: ", buf);
  
  snprintf(buf, sizeof(buf), "%u KB", freeMem / 1024);
  drawText(tft, 10, 60, "Free Heap: ", buf, freeMem < 20000 ? COL_WARNING : COL_ACCENT);
  
  snprintf(buf, sizeof(buf), "%u KB", minMem / 1024);
  drawText(tft, 10, 80, "Min Free: ", buf);
  
  snprintf(buf, sizeof(buf), "%u%%", (freeMem * 100) / totalMem);
  drawText(tft, 10, 100, "Usage: ", buf);
  
  // PSRAM (if available)
  if (ESP.getPsramSize() > 0) {
    snprintf(buf, sizeof(buf), "%u KB", ESP.getFreePsram() / 1024);
    drawText(tft, 10, 120, "Free PSRAM: ", buf);
  }
}

static void drawSensorsPage(TFT_eSPI* tft) {
  drawHeader(tft, "Sensor Status");
  
  char buf[64];
  
  // Sonar
  snprintf(buf, sizeof(buf), "%.1f cm", s_lastSonar);
  drawText(tft, 10, 40, "Sonar: ", buf);
  
  // Compass
  snprintf(buf, sizeof(buf), "%.1f deg", s_lastCompass);
  drawText(tft, 10, 60, "Compass: ", buf);
  
  // GPS
  drawText(tft, 10, 80, "GPS: ", s_lastGPSValid ? "VALID" : "NO FIX", 
           s_lastGPSValid ? COL_ACCENT : COL_WARNING);
}

static void drawSDBrowserPage(TFT_eSPI* tft) {
  drawHeader(tft, "SD File Browser");
  
  if (!sdIsAvailable()) {
    drawText(tft, 10, 40, "", "SD CARD NOT AVAILABLE", COL_ERROR);
    return;
  }
  
  // List macros
  uint8_t macroSlots[10];
  uint8_t macroCount = sdListMacros(macroSlots, 10);
  
  tft->setTextSize(1);
  tft->setTextColor(COL_TEXT, COL_BG);
  tft->setCursor(10, 40);
  tft->print("Macros:");
  
  if (macroCount == 0) {
    tft->setCursor(20, 55);
    tft->print("(none)");
  } else {
    for (uint8_t i = 0; i < macroCount && i < 8; i++) {
      char buf[64];
      uint16_t frames = sdGetMacroFrameCount(macroSlots[i]);
      snprintf(buf, sizeof(buf), "  Slot %d: %d frames", macroSlots[i], frames);
      tft->setCursor(20, 55 + i * 12);
      tft->print(buf);
    }
  }
  
  // Free space
  char buf[64];
  snprintf(buf, sizeof(buf), "%u MB free", sdGetFreeSpaceMB());
  drawText(tft, 10, 200, "Storage: ", buf);
}

// ============================================================
//  Main Draw Function
// ============================================================

void devConsoleDraw(TFT_eSPI* tft) {
  if (!s_unlocked) return;
  
  tft->fillScreen(COL_BG);
  
  switch (s_currentPage) {
    case DEV_OVERVIEW:      drawOverviewPage(tft);      break;
    case DEV_SERVO_GRAPH:   drawServoGraphPage(tft);    break;
    case DEV_PACKET_TIMING: drawPacketTimingPage(tft);  break;
    case DEV_MEMORY:        drawMemoryPage(tft);        break;
    case DEV_SENSORS:       drawSensorsPage(tft);       break;
    case DEV_SD_BROWSER:    drawSDBrowserPage(tft);     break;
  }
  
  // Footer with page indicator
  tft->setTextSize(1);
  tft->setTextColor(COL_GRID, COL_BG);
  tft->setCursor(200, 310);
  char buf[32];
  snprintf(buf, sizeof(buf), "Page %d/%d", s_currentPage + 1, DEV_PAGE_COUNT);
  tft->print(buf);
}

// ============================================================
//  Update (for live data)
// ============================================================

void devConsoleUpdate(uint32_t now) {
  if (!s_unlocked) return;
  
  // Live update for servo graph page
  static uint32_t lastUpdate = 0;
  if (s_currentPage == DEV_SERVO_GRAPH && now - lastUpdate > 50) {
    // Trigger redraw if needed
    lastUpdate = now;
  }
}
