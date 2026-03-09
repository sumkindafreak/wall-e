// ============================================================
//  WALL-E CYD Master Controller — SD Card Manager Implementation
// ============================================================

#include "sd_manager.h"
#include <FS.h>

// ============================================================
//  Internal State
// ============================================================

static bool s_sdAvailable = false;
static uint32_t s_lastUpdateMs = 0;

// Log buffer (non-blocking writes)
#define LOG_BUFFER_SIZE 512
static char s_logBuffer[LOG_BUFFER_SIZE];
static uint16_t s_logBufferPos = 0;
static bool s_logPending = false;

// ============================================================
//  Helper: Calculate checksum
// ============================================================

static uint32_t calculateChecksum(const void* data, size_t len) {
  uint32_t sum = 0;
  const uint8_t* bytes = (const uint8_t*)data;
  for (size_t i = 0; i < len; i++) {
    sum += bytes[i];
  }
  return sum;
}

// ============================================================
//  Helper: Create directory if not exists
// ============================================================

static bool ensureDirectory(const char* path) {
  if (!s_sdAvailable) return false;
  
  if (!SD.exists(path)) {
    if (SD.mkdir(path)) {
      Serial.printf("[SD] Created: %s\n", path);
      return true;
    } else {
      Serial.printf("[SD] ⚠️  Failed to create: %s\n", path);
      return false;
    }
  }
  return true;
}

// ============================================================
//  Initialization
// ============================================================

bool sdInit() {
  Serial.println("[SD] Initializing...");
  
  // Initialize SPI for SD card
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  // Mount SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] ⚠️  Mount failed - continuing without SD");
    s_sdAvailable = false;
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] ⚠️  No SD card detected");
    s_sdAvailable = false;
    return false;
  }
  
  s_sdAvailable = true;
  
  // Print card info
  Serial.print("[SD] Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("[SD] Size: %lluMB\n", cardSize);
  Serial.printf("[SD] Free: %uMB\n", sdGetFreeSpaceMB());
  
  // Create directory structure
  ensureDirectory(SD_ROOT);
  ensureDirectory(SD_MACROS);
  ensureDirectory(SD_ANIMATIONS);
  ensureDirectory(SD_STORY);
  ensureDirectory(SD_LOGS);
  ensureDirectory(SD_PROFILES);
  
  Serial.println("[SD] ✓ Ready");
  return true;
}

bool sdIsAvailable() {
  return s_sdAvailable;
}

uint32_t sdGetFreeSpaceMB() {
  if (!s_sdAvailable) return 0;
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  return (uint32_t)((total - used) / (1024 * 1024));
}

// ============================================================
//  Macro Management
// ============================================================

bool sdSaveMacro(uint8_t slot, const MacroFrame* frames, uint16_t frameCount) {
  if (!s_sdAvailable || !frames || frameCount == 0) return false;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/macro_%d.bin", SD_MACROS, slot);
  
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("[SD] ⚠️  Failed to create: %s\n", path);
    return false;
  }
  
  // Write frame count
  file.write((uint8_t*)&frameCount, sizeof(frameCount));
  
  // Write frames
  size_t written = file.write((uint8_t*)frames, sizeof(MacroFrame) * frameCount);
  file.close();
  
  if (written == sizeof(MacroFrame) * frameCount) {
    Serial.printf("[SD] Saved macro %d (%d frames)\n", slot, frameCount);
    return true;
  } else {
    Serial.printf("[SD] ⚠️  Write error on macro %d\n", slot);
    return false;
  }
}

uint16_t sdLoadMacro(uint8_t slot, MacroFrame* frames, uint16_t maxFrames) {
  if (!s_sdAvailable || !frames) return 0;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/macro_%d.bin", SD_MACROS, slot);
  
  if (!SD.exists(path)) return 0;
  
  File file = SD.open(path, FILE_READ);
  if (!file) return 0;
  
  // Read frame count
  uint16_t frameCount = 0;
  file.read((uint8_t*)&frameCount, sizeof(frameCount));
  
  if (frameCount > maxFrames) frameCount = maxFrames;
  
  // Read frames
  size_t bytesRead = file.read((uint8_t*)frames, sizeof(MacroFrame) * frameCount);
  file.close();
  
  if (bytesRead == sizeof(MacroFrame) * frameCount) {
    Serial.printf("[SD] Loaded macro %d (%d frames)\n", slot, frameCount);
    return frameCount;
  }
  
  return 0;
}

bool sdDeleteMacro(uint8_t slot) {
  if (!s_sdAvailable) return false;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/macro_%d.bin", SD_MACROS, slot);
  
  if (SD.remove(path)) {
    Serial.printf("[SD] Deleted macro %d\n", slot);
    return true;
  }
  return false;
}

bool sdMacroExists(uint8_t slot) {
  if (!s_sdAvailable) return false;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/macro_%d.bin", SD_MACROS, slot);
  return SD.exists(path);
}

uint16_t sdGetMacroFrameCount(uint8_t slot) {
  if (!s_sdAvailable) return 0;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/macro_%d.bin", SD_MACROS, slot);
  
  File file = SD.open(path, FILE_READ);
  if (!file) return 0;
  
  uint16_t frameCount = 0;
  file.read((uint8_t*)&frameCount, sizeof(frameCount));
  file.close();
  
  return frameCount;
}

// ============================================================
//  Animation Management
// ============================================================

bool sdSaveAnimation(const char* name, const AnimKeyframe* keyframes, uint16_t count) {
  if (!s_sdAvailable || !name || !keyframes || count == 0) return false;
  
  char path[96];
  snprintf(path, sizeof(path), "%s/%s.anim", SD_ANIMATIONS, name);
  
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  
  file.write((uint8_t*)&count, sizeof(count));
  size_t written = file.write((uint8_t*)keyframes, sizeof(AnimKeyframe) * count);
  file.close();
  
  return (written == sizeof(AnimKeyframe) * count);
}

uint16_t sdLoadAnimation(const char* name, AnimKeyframe* keyframes, uint16_t maxCount) {
  if (!s_sdAvailable || !name || !keyframes) return 0;
  
  char path[96];
  snprintf(path, sizeof(path), "%s/%s.anim", SD_ANIMATIONS, name);
  
  if (!SD.exists(path)) return 0;
  
  File file = SD.open(path, FILE_READ);
  if (!file) return 0;
  
  uint16_t count = 0;
  file.read((uint8_t*)&count, sizeof(count));
  
  if (count > maxCount) count = maxCount;
  
  file.read((uint8_t*)keyframes, sizeof(AnimKeyframe) * count);
  file.close();
  
  return count;
}

bool sdAnimationExists(const char* name) {
  if (!s_sdAvailable || !name) return false;
  
  char path[96];
  snprintf(path, sizeof(path), "%s/%s.anim", SD_ANIMATIONS, name);
  return SD.exists(path);
}

// ============================================================
//  Story Memory
// ============================================================

bool sdLoadStoryMemory(StoryMemory* memory) {
  if (!s_sdAvailable || !memory) return false;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/memory.dat", SD_STORY);
  
  if (!SD.exists(path)) {
    // Initialize with defaults
    memset(memory, 0, sizeof(StoryMemory));
    return false;
  }
  
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  
  size_t bytesRead = file.read((uint8_t*)memory, sizeof(StoryMemory));
  file.close();
  
  if (bytesRead != sizeof(StoryMemory)) return false;
  
  // Verify checksum
  uint32_t expectedChecksum = memory->checksum;
  memory->checksum = 0;
  uint32_t actualChecksum = calculateChecksum(memory, sizeof(StoryMemory) - sizeof(uint32_t));
  memory->checksum = expectedChecksum;
  
  if (expectedChecksum != actualChecksum) {
    Serial.println("[SD] ⚠️  Story memory checksum failed");
    memset(memory, 0, sizeof(StoryMemory));
    return false;
  }
  
  Serial.println("[SD] Loaded story memory");
  return true;
}

bool sdSaveStoryMemory(const StoryMemory* memory) {
  if (!s_sdAvailable || !memory) return false;
  
  // Create copy with checksum
  StoryMemory temp = *memory;
  temp.checksum = 0;
  temp.checksum = calculateChecksum(&temp, sizeof(StoryMemory) - sizeof(uint32_t));
  
  char path[64];
  snprintf(path, sizeof(path), "%s/memory.dat", SD_STORY);
  
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  
  size_t written = file.write((uint8_t*)&temp, sizeof(StoryMemory));
  file.close();
  
  if (written == sizeof(StoryMemory)) {
    Serial.println("[SD] Saved story memory");
    return true;
  }
  return false;
}

// ============================================================
//  Profile Export/Import
// ============================================================

bool sdExportProfile(uint8_t slot, const char* data) {
  if (!s_sdAvailable || !data) return false;
  
  char path[64];
  snprintf(path, sizeof(path), "%s/profile_%d.json", SD_PROFILES, slot);
  
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  
  file.print(data);
  file.close();
  
  Serial.printf("[SD] Exported profile %d\n", slot);
  return true;
}

String sdImportProfile(uint8_t slot) {
  if (!s_sdAvailable) return "";
  
  char path[64];
  snprintf(path, sizeof(path), "%s/profile_%d.json", SD_PROFILES, slot);
  
  if (!SD.exists(path)) return "";
  
  File file = SD.open(path, FILE_READ);
  if (!file) return "";
  
  String data = file.readString();
  file.close();
  
  Serial.printf("[SD] Imported profile %d\n", slot);
  return data;
}

// ============================================================
//  Logging System
// ============================================================

void sdLogInit() {
  s_logBufferPos = 0;
  s_logPending = false;
}

void sdLog(const char* message) {
  if (!s_sdAvailable || !message) return;
  
  // Get timestamp
  uint32_t now = millis();
  char timestamped[256];
  snprintf(timestamped, sizeof(timestamped), "[%lu] %s\n", now, message);
  
  // Add to buffer
  size_t len = strlen(timestamped);
  if (s_logBufferPos + len < LOG_BUFFER_SIZE) {
    strcpy(s_logBuffer + s_logBufferPos, timestamped);
    s_logBufferPos += len;
    s_logPending = true;
  } else {
    // Buffer full, flush immediately
    sdLogFlush();
    strcpy(s_logBuffer, timestamped);
    s_logBufferPos = len;
    s_logPending = true;
  }
}

void sdLogFlush() {
  if (!s_sdAvailable || !s_logPending || s_logBufferPos == 0) return;
  
  // Get current date for log filename (YYYYMMDD)
  char path[64];
  snprintf(path, sizeof(path), "%s/log_%lu.txt", SD_LOGS, millis() / 86400000);
  
  File file = SD.open(path, FILE_APPEND);
  if (file) {
    file.write((uint8_t*)s_logBuffer, s_logBufferPos);
    file.close();
  }
  
  s_logBufferPos = 0;
  s_logPending = false;
}

// ============================================================
//  File Listing
// ============================================================

uint8_t sdListMacros(uint8_t* slots, uint8_t maxSlots) {
  if (!s_sdAvailable || !slots) return 0;
  
  uint8_t count = 0;
  for (uint8_t i = 0; i < 255 && count < maxSlots; i++) {
    if (sdMacroExists(i)) {
      slots[count++] = i;
    }
  }
  return count;
}

uint8_t sdListAnimations(char names[][32], uint8_t maxCount) {
  if (!s_sdAvailable || !names) return 0;
  
  File root = SD.open(SD_ANIMATIONS);
  if (!root) return 0;
  
  uint8_t count = 0;
  File file = root.openNextFile();
  while (file && count < maxCount) {
    if (!file.isDirectory()) {
      String name = file.name();
      if (name.endsWith(".anim")) {
        name.remove(name.length() - 5);  // Remove ".anim"
        strncpy(names[count], name.c_str(), 31);
        names[count][31] = '\0';
        count++;
      }
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();
  
  return count;
}

// ============================================================
//  Non-Blocking Update
// ============================================================

void sdUpdate() {
  uint32_t now = millis();
  
  // Flush log buffer every 5 seconds
  if (s_logPending && (now - s_lastUpdateMs > 5000)) {
    sdLogFlush();
    s_lastUpdateMs = now;
  }
}
