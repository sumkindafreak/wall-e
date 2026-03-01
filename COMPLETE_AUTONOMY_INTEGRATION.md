// ============================================================
//  WALL-E AUTONOMY - COMPLETE INTEGRATION GUIDE
//  Base ESP32-S3 + CYD Controller
// ============================================================

## ✅ COMPLETED FILES:

### Base ESP32 (/main):
1. ✅ sonar_sensor.h/cpp
2. ✅ compass_sensor.h/cpp
3. ✅ gps_module.h/cpp
4. ✅ waypoint_nav.h/cpp
5. ✅ autonomy_engine.h/cpp (COMPLETE - 600+ lines)

### CYD Controller:
6. ✅ protocol.h (UPDATED with autonomy telemetry)

---

## 🔧 STEP 1: Update Base main.ino

Add includes at top:
```cpp
#include "sonar_sensor.h"
#include "compass_sensor.h"
#include "gps_module.h"
#include "waypoint_nav.h"
#include "autonomy_engine.h"
```

In setup() after servoInit():
```cpp
void setup() {
  // ... existing code ...
  
  servoInit();
  servoNeutral(SERVO_SLOW_SPEED);
  beginIMU();
  
  // NEW: Initialize sensors
  sonarInit();
  compassInit();
  gpsInit();
  waypointInit();
  autonomyInit();
  
  // ... rest of existing code ...
}
```

In loop() after updateIMU():
```cpp
void loop() {
  // ... existing code ...
  
  updateIMU();
  
  // NEW: Update sensors
  uint32_t now = millis();
  sonarUpdate(now);
  compassUpdate(now);
  gpsUpdate(now);
  
  // Update waypoint navigation if GPS has fix
  if (gpsHasFix()) {
    waypointUpdate(gpsGetLatitude(), gpsGetLongitude(), compassGetHeading());
  }
  
  // Get autonomy drive commands
  int8_t autoLeft = 0, autoRight = 0;
  autonomyUpdate(now, &autoLeft, &autoRight);
  
  // Check if manual control is active (from ESP-NOW)
  bool manualActive = (abs(lastReceivedLeft) > 5 || abs(lastReceivedRight) > 5);
  autonomySetManualOverride(manualActive);
  
  // Apply motor commands (autonomy or manual)
  if (autonomyIsEnabled() && !manualActive) {
    motorSet(autoLeft, autoRight);
  } else {
    motorSet(lastReceivedLeft, lastReceivedRight);
  }
  
  // ... rest of existing code ...
}
```

---

## 🔧 STEP 2: Update ESP-NOW Telemetry (Base)

In espnow_receiver.cpp (or wherever telemetry is sent):

```cpp
void sendTelemetry() {
  TelemetryPacket telem = {};
  
  // Existing telemetry
  telem.batteryVoltage = batteryGetVoltage();
  telem.currentDraw = batteryGetCurrent();
  telem.temperature = batteryGetTemp();
  
  // NEW: Autonomy telemetry
  const AutoContext* ctx = autonomyGetContext();
  const LocationState* loc = autonomyGetLocation();
  const NavState* nav = waypointGetNavState();
  
  telem.autonomyEnabled = autonomyIsEnabled() ? 1 : 0;
  telem.autonomyState = (uint8_t)autonomyGetState();
  telem.sonarDistanceCm = ctx->detectedDistance;
  telem.compassHeading = loc->heading;
  telem.gpsLatitude = (float)loc->latitude;
  telem.gpsLongitude = (float)loc->longitude;
  telem.gpsValid = loc->gpsValid ? 1 : 0;
  telem.waypointMode = autonomyIsWaypointMode() ? 1 : 0;
  telem.waypointDistanceM = nav->distanceToWaypoint;
  telem.waypointBearingDeg = nav->bearingToWaypoint;
  telem.currentWaypoint = waypointGetCurrent();
  telem.totalWaypoints = waypointGetCount();
  
  // Send via ESP-NOW
  esp_now_send(peerMac, (uint8_t*)&telem, sizeof(telem));
}
```

---

## 🔧 STEP 3: Add Autonomy Control Commands

Add to ControlPacket or create new AutonomyCommand:

```cpp
// In protocol.h (or new autonomy_protocol.h):
typedef struct __attribute__((packed)) {
  uint8_t command;        // 0=disable, 1=enable explore, 2=enable waypoint, 3=add waypoint, 4=clear waypoints
  uint8_t waypointIndex;  // Target waypoint index
  float waypointLat;      // Latitude for add waypoint
  float waypointLon;      // Longitude for add waypoint
  char waypointName[16];  // Waypoint name
} AutonomyCommand;
```

Handle on Base:
```cpp
void onAutonomyCommand(const AutonomyCommand* cmd) {
  switch (cmd->command) {
    case 0: // Disable
      autonomySetEnabled(false);
      break;
    case 1: // Enable explore mode
      autonomySetWaypointMode(false);
      autonomySetEnabled(true);
      break;
    case 2: // Enable waypoint mode
      autonomySetWaypointMode(true);
      waypointStartNavigation();
      autonomySetEnabled(true);
      break;
    case 3: // Add waypoint
      waypointAdd(cmd->waypointLat, cmd->waypointLon, cmd->waypointName);
      waypointSave();  // Save to flash
      break;
    case 4: // Clear waypoints
      waypointClearAll();
      waypointSave();
      break;
  }
}
```

---

## 🔧 STEP 4: CYD UI Updates

### Add PAGE_AUTONOMY enum:
```cpp
// In ui_draw.h
enum Page {
  PAGE_DRIVE,
  PAGE_BEHAVIOUR,
  PAGE_SYSTEM,
  PAGE_PROFILE,
  PAGE_SERVO_EDITOR,
  PAGE_SERVO_TEST,
  PAGE_AUTONOMY,    // NEW
  PAGE_WAYPOINTS    // NEW
};
```

### Create Autonomy Page:
```cpp
// In ui_draw.cpp
void uiDrawPageAutonomy(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  
  // Title
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Autonomy", 10, 6);
  
  // Get telemetry
  TelemetryPacket telem;
  packetGetTelemetry(&telem);
  
  // Enable/Disable button
  bool enabled = telem.autonomyEnabled;
  g_tft->fillRect(200, 50, 100, 32, enabled ? C_GREEN : C_RED);
  g_tft->setTextColor(C_WHITE, enabled ? C_GREEN : C_RED);
  g_tft->setTextSize(1);
  g_tft->drawString(enabled ? "ENABLED" : "DISABLED", 215, 60);
  
  // State
  g_tft->setTextColor(C_ACCENT, C_BG);
  char buf[32];
  snprintf(buf, sizeof(buf), "State: %s", getAutonomyStateName(telem.autonomyState));
  g_tft->drawString(buf, 16, 56);
  
  // Sonar
  snprintf(buf, sizeof(buf), "Sonar: %.1fcm", telem.sonarDistanceCm);
  g_tft->drawString(buf, 16, 80);
  
  // Compass
  snprintf(buf, sizeof(buf), "Heading: %.0f°", telem.compassHeading);
  g_tft->drawString(buf, 16, 104);
  
  // GPS
  if (telem.gpsValid) {
    snprintf(buf, sizeof(buf), "GPS: %.6f,%.6f", telem.gpsLatitude, telem.gpsLongitude);
  } else {
    snprintf(buf, sizeof(buf), "GPS: No fix");
  }
  g_tft->drawString(buf, 16, 128);
  
  // Waypoint info
  if (telem.waypointMode) {
    snprintf(buf, sizeof(buf), "Waypoint: %d/%d", telem.currentWaypoint + 1, telem.totalWaypoints);
    g_tft->drawString(buf, 16, 152);
    snprintf(buf, sizeof(buf), "Distance: %.1fm @ %.0f°", telem.waypointDistanceM, telem.waypointBearingDeg);
    g_tft->drawString(buf, 16, 176);
  }
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}
```

### Create Waypoints Page:
```cpp
void uiDrawPageWaypoints(void) {
  // Display list of waypoints
  // Buttons: Add Current Position, Clear All, Navigate
  // Show waypoint list with lat/lon/name
  // Touch to select waypoint
}
```

---

## 🔧 STEP 5: Hardware Wiring

### Sonar HC-SR04:
- Trig → GPIO 26
- Echo → GPIO 27
- VCC → 5V
- GND → GND

### Compass QMC5883L:
- SDA → GPIO 21 (I2C shared)
- SCL → GPIO 22 (I2C shared)
- VCC → 3.3V
- GND → GND

### GPS NEO-6M/7M/8M:
- RX → GPIO 16 (Base TX)
- TX → GPIO 17 (Base RX)
- VCC → 3.3V or 5V
- GND → GND

---

## 🔧 STEP 6: Library Dependencies

Add to platformio.ini:
```ini
lib_deps = 
    TinyGPSPlus @ ^1.0.3
    Adafruit MPU6050 @ ^2.2.4
    Adafruit PWM Servo Driver Library @ ^2.4.1
    Wire
    Preferences
```

---

## 🎯 USAGE FLOW:

### Free Exploration Mode:
1. Navigate to Autonomy page on CYD
2. Tap "Enable" button
3. WALL-E begins exploring randomly
4. Uses compass for heading
5. Avoids obstacles with sonar
6. Investigates interesting objects

### Waypoint Navigation Mode:
1. Navigate to Waypoints page
2. Add waypoints (tap "Add Current" or enter GPS coords)
3. Tap "Navigate" button
4. WALL-E follows waypoint sequence
5. Auto-advances on arrival
6. Pauses for obstacles
7. Resumes after clear

### Manual Override:
- Touch joystick on CYD → autonomy pauses immediately
- Release joystick → autonomy resumes

---

## 🛡️ SAFETY FEATURES:

1. **Manual Override**: Joystick ALWAYS wins
2. **E-STOP**: Disables autonomy instantly
3. **Sonar Timeout**: 2s without valid reading → stop
4. **IMU Tilt**: Excessive tilt → emergency stop
5. **GPS Loss**: Falls back to exploration mode
6. **Compass Failure**: Falls back to random wandering
7. **Obstacle Detection**: Emergency pause and avoidance

---

## 🎭 PERSONALITY INTEGRATION:

Set personality via profiles or direct commands:
```cpp
Personality pers;
pers.curiosityLevel = 0.8f;  // High curiosity
pers.braveryLevel = 0.6f;    // Moderately brave
pers.energyLevel = 0.7f;     // Energetic
pers.randomness = 0.5f;      // Some randomness

autonomySetPersonality(&pers);
```

---

## 🚀 TESTING CHECKLIST:

- [ ] Sonar readings appear in telemetry
- [ ] Compass heading updates
- [ ] GPS acquires fix
- [ ] Waypoint add/remove works
- [ ] Free exploration moves and avoids
- [ ] Waypoint navigation reaches targets
- [ ] Manual override works instantly
- [ ] E-STOP disables autonomy
- [ ] IMU tilt detection works
- [ ] Telemetry displays on CYD

---

## 📊 DEBUG OUTPUT:

Enable serial monitoring on Base:
```
[Sonar] 45.2cm valid
[Compass] Heading: 187°
[GPS] Fix: 51.123456,-0.123456 Sats:8
[Waypoint] Distance: 15.3m Bearing: 045°
[Autonomy] EXPLORE → ORIENT
[Autonomy] Waypoint: 12.8m @ 42° (heading: 38°)
[Autonomy] Arrived at #2: Home
```

---

ALL CODE IS NON-BLOCKING.
ALL SENSORS DEGRADE GRACEFULLY.
MANUAL CONTROL ALWAYS WINS.
WALL-E IS NOW TRULY AUTONOMOUS. 🤖✨
