// ============================================================
//  WALL-E CYD Master Controller — SD Card Manager
//  Professional storage system for macros, animations, logs
// ============================================================

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// ============================================================
//  SD Card Configuration
// ============================================================

#define SD_CS_PIN      5
#define SD_MOSI_PIN    23
#define SD_MISO_PIN    19
#define SD_SCK_PIN     18

// Directory structure
#define SD_ROOT         "/wall_e"
#define SD_MACROS       "/wall_e/macros"
#define SD_ANIMATIONS   "/wall_e/animations"
#define SD_STORY        "/wall_e/story"
#define SD_LOGS         "/wall_e/logs"
#define SD_PROFILES     "/wall_e/profiles"

// ============================================================
//  Macro Frame Structure (Binary)
// ============================================================

struct MacroFrame {
  uint32_t timeOffset;      // Milliseconds from start
  float trackLeft;          // -100 to +100
  float trackRight;         // -100 to +100
  float servo[9];           // 0-100 positions
};

// ============================================================
//  Animation Keyframe Structure (Binary)
// ============================================================

struct AnimKeyframe {
  uint16_t timeMs;          // Time in ms
  float offset[9];          // Servo offsets
};

// ============================================================
//  Story Memory Structure (Persistent)
// ============================================================

struct StoryMemory {
  uint32_t totalInteractions;
  uint32_t lastInteractionTime;
  float familiarityScore[16];    // Directional familiarity
  float emotionalBaseline;
  uint32_t checksum;             // Data integrity
};

// ============================================================
//  Public API
// ============================================================

// Initialize SD card and create directory structure
bool sdInit();

// Check if SD is available
bool sdIsAvailable();

// Get free space (MB)
uint32_t sdGetFreeSpaceMB();

// Macro management
bool sdSaveMacro(uint8_t slot, const MacroFrame* frames, uint16_t frameCount);
uint16_t sdLoadMacro(uint8_t slot, MacroFrame* frames, uint16_t maxFrames);
bool sdDeleteMacro(uint8_t slot);
bool sdMacroExists(uint8_t slot);
uint16_t sdGetMacroFrameCount(uint8_t slot);

// Animation management
bool sdSaveAnimation(const char* name, const AnimKeyframe* keyframes, uint16_t count);
uint16_t sdLoadAnimation(const char* name, AnimKeyframe* keyframes, uint16_t maxCount);
bool sdAnimationExists(const char* name);

// Story memory
bool sdLoadStoryMemory(StoryMemory* memory);
bool sdSaveStoryMemory(const StoryMemory* memory);

// Profile export/import
bool sdExportProfile(uint8_t slot, const char* data);
String sdImportProfile(uint8_t slot);

// Logging (non-blocking, buffered)
void sdLogInit();
void sdLog(const char* message);
void sdLogFlush();  // Call periodically to write buffer

// File listing
uint8_t sdListMacros(uint8_t* slots, uint8_t maxSlots);
uint8_t sdListAnimations(char names[][32], uint8_t maxCount);

// Maintenance
void sdUpdate();  // Call in main loop for non-blocking operations

#endif // SD_MANAGER_H
