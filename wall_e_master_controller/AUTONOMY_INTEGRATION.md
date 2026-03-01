// ============================================================
//  WALL-E AUTONOMOUS MODE - INTEGRATION GUIDE
// ============================================================

## INTEGRATION STEPS

### 1. Add to wall_e_master_controller.ino

At top with other includes:
```cpp
#include "autonomy_engine.h"
#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
```

In setup():
```cpp
// After profileInit()
autonomyInit();
```

In loop(), AFTER joystick read but BEFORE packetUpdate:
```cpp
// Check if joystick is active (for autonomy override)
bool joystickActive = joy.active[JOY2_X] || joy.active[JOY2_Y];
autonomySetJoystickOverride(joystickActive);

// Update sensors
sonarUpdate(now);
compassUpdate(now);
gpsUpdate(now);

// Update autonomy engine
autonomyUpdate(now);

// Get autonomy drive output if enabled
if (autonomyIsEnabled() && !joystickActive) {
  const AutoContext* ctx = autonomyGetContext();
  // Apply autonomous drive commands
  // (autonomy engine modifies DriveState internally)
}
```

### 2. Add PAGE_AUTONOMY to ui_draw.h

In Page enum:
```cpp
enum Page {
  PAGE_DRIVE,
  PAGE_BEHAVIOUR,
  PAGE_SYSTEM,
  PAGE_PROFILE,
  PAGE_SERVO_EDITOR,
  PAGE_SERVO_TEST,
  PAGE_AUTONOMY        // NEW
};
```

### 3. Add Autonomy UI Page (ui_draw.cpp)

```cpp
void uiDrawPageAutonomy(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  
  // Grid
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  
  // Top bar
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Autonomy", 10, 6);
  
  // Enable/Disable toggle
  bool enabled = autonomyIsEnabled();
  g_tft->fillRect(200, 50, 100, 32, enabled ? C_GREEN : C_RED);
  g_tft->setTextColor(C_WHITE, enabled ? C_GREEN : C_RED);
  g_tft->setTextSize(1);
  g_tft->drawString(enabled ? "ENABLED" : "DISABLED", 215, 60);
  
  // Current state
  const char* stateName = autonomyGetStateName();
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("State:", 16, 56);
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(stateName, 60, 56);
  
  // Sonar distance
  const AutoContext* ctx = autonomyGetContext();
  char buf[32];
  snprintf(buf, sizeof(buf), "Sonar: %.1fcm", ctx->detectedDistance);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString(buf, 16, 80);
  
  // Heading
  const LocationState* loc = autonomyGetLocation();
  snprintf(buf, sizeof(buf), "Heading: %.0f°", loc->heading);
  g_tft->drawString(buf, 16, 104);
  
  // GPS status
  if (loc->gpsValid) {
    snprintf(buf, sizeof(buf), "GPS: %.6f,%.6f", loc->latitude, loc->longitude);
  } else {
    snprintf(buf, sizeof(buf), "GPS: No fix");
  }
  g_tft->drawString(buf, 16, 128);
  
  // Personality
  const Personality* pers = autonomyGetPersonality();
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  snprintf(buf, sizeof(buf), "Curiosity:%.1f Bravery:%.1f", pers->curiosityLevel, pers->braveryLevel);
  g_tft->drawString(buf, 16, 160);
  snprintf(buf, sizeof(buf), "Energy:%.1f Random:%.1f", pers->energyLevel, pers->randomness);
  g_tft->drawString(buf, 16, 176);
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}
```

In uiDrawCurrentPage():
```cpp
case PAGE_AUTONOMY:
  uiDrawPageAutonomy();
  break;
```

### 4. Add Touch Zones (touch_input.h)

```cpp
enum TouchZone {
  // ... existing zones ...
  TOUCH_ZONE_NAV_AUTONOMY,
  TOUCH_ZONE_AUTONOMY_TOGGLE
};
```

### 5. Add Touch Handlers (touch_input.cpp)

In touchGetZone():
```cpp
// Autonomy page
if (page == 6) {  // PAGE_AUTONOMY
  // Toggle button (200, 50, 100x32)
  if (screenX >= 200 && screenX <= 300 && screenY >= 50 && screenY <= 82) {
    return TOUCH_ZONE_AUTONOMY_TOGGLE;
  }
}

// System page - add Autonomy button
if (page == 2) {
  // Autonomy button at (16, 140)
  if (screenY >= 140 && screenY <= 172 && screenX >= 16 && screenX <= 116) {
    return TOUCH_ZONE_NAV_AUTONOMY;
  }
}
```

In wall_e_master_controller.ino loop():
```cpp
if (zone == TOUCH_ZONE_NAV_AUTONOMY) {
  g_currentPage = PAGE_AUTONOMY;
  g_needStaticRedraw = true;
  playUISound(SOUND_CLICK);
}

if (zone == TOUCH_ZONE_AUTONOMY_TOGGLE) {
  autonomySetEnabled(!autonomyIsEnabled());
  g_needStaticRedraw = true;
  playUISound(SOUND_MODE_CHANGE);
}
```

### 6. Apply Personality from Profile

In profiles.cpp profileApply():
```cpp
// Apply autonomous personality
Personality pers;
pers.curiosityLevel = p->autonomyCuriosity;
pers.braveryLevel = p->autonomyBravery;
pers.energyLevel = p->autonomyEnergy;
pers.randomness = p->autonomyRandomness;
autonomySetPersonality(&pers);
autonomySetEnabled(p->autonomyEnabled);
```

### 7. Hardware Connections

**Sonar (HC-SR04 or similar):**
- Trigger Pin: GPIO 26
- Echo Pin: GPIO 27
- VCC: 5V
- GND: GND

**Compass (HMC5883L or QMC5883L):**
- SDA: GPIO 21 (shared I2C)
- SCL: GPIO 22 (shared I2C)
- VCC: 3.3V
- GND: GND

**GPS Module (NEO-6M/7M/8M):**
- RX: GPIO 16 (UART2 TX)
- TX: GPIO 17 (UART2 RX)
- VCC: 3.3V or 5V (check module)
- GND: GND

### 8. Required Libraries

Add to platformio.ini or Arduino IDE:
```
TinyGPSPlus @ ^1.0.3
```

### 9. Safety Features

- Sonar timeout (2s) stops autonomy
- Compass failure disables heading features
- Joystick ALWAYS overrides autonomy
- E-STOP immediately disables autonomy
- Deadman button NOT required for autonomy (only for manual drive)

### 10. Behavior Summary

**AUTO_IDLE:** Waiting, idle behaviours only
**AUTO_SCAN:** Neck sweeps left-right, capturing sonar readings
**AUTO_EVALUATE:** Processes scan results, decides action
**AUTO_APPROACH:** Moves toward detected object
**AUTO_INVESTIGATE_HEIGHT:** Random vertical investigation (1-10 levels)
**AUTO_REACT:** Close-range reaction (eyebrows, sounds)
**AUTO_AVOID:** Backs away from obstacle
**AUTO_ORIENT:** Rotates to target compass heading
**AUTO_EXPLORE_LOOP:** Random wandering with compass-based navigation

## COMPILE ORDER

1. Compile sonar_sensor.cpp
2. Compile compass_sensor.cpp
3. Compile gps_module.cpp
4. Compile autonomy_engine.cpp
5. Update profiles.cpp
6. Update ui_draw.cpp
7. Update touch_input.cpp
8. Update wall_e_master_controller.ino

All non-blocking. All safe. Joystick always wins.

THIS IS A CHARACTER, NOT A ROBOT.
