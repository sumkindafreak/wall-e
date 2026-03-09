# Graceful Degradation Implementation - Feb 24, 2026

## Goal
WALL-E should boot and operate even when optional sensors are not present.

## Implementation Strategy

### 1. Fail-Safe Initialization
All sensor init functions now return `bool`:
- `true` = sensor initialized successfully
- `false` = sensor not present/failed, continue without it

### 2. Main.ino Changes
```cpp
if (!sonarInit()) {
  Serial.println("[Sonar] ⚠️  Not available - continuing without");
} else {
  Serial.println("[Sonar] ✓ Ready");
}
```

### 3. Sensor Behavior Without Hardware

| Sensor | Without Hardware | Autonomy Impact |
|--------|------------------|-----------------|
| **Sonar** | Always returns `0cm`, `valid=false` | No obstacle detection, exploration continues |
| **Compass** | Returns heading `0°`, `valid=false` | No orientation awareness, wander mode affected |
| **GPS** | No fix, `lat/lon = 0`, `valid=false` | No waypoint navigation, no return-home |
| **IMU** | Already shows "not found" and continues | No tilt safety, basic operation unaffected |

### 4. Behavioral Engines
All behavioral engines (personality, emotion, interest, memory, return home) initialize with default values regardless of sensor availability:

- **Personality**: Uses saved preferences or defaults
- **Emotion**: Starts in CALM state
- **Interest**: Starts at 0
- **Memory**: Empty history, no home set
- **Return Home**: Idle until home position is set

### 5. Autonomy Engine Degraded Modes

#### Full Sensor Suite
- Sonar + Compass + GPS → Full autonomous exploration with waypoints

#### Missing GPS
- Sonar + Compass → Local exploration, no waypoint navigation

#### Missing Compass  
- Sonar only → Simple forward/backward exploration

#### Missing All Sensors
- Autonomous mode effectively disabled
- Manual control via CYD always works

### 6. WebUI Status
The WebUI `/api/autonomy` endpoint reports sensor validity:
```json
{
  "sonarValid": false,
  "compassValid": false,
  "gpsValid": false,
  "heading": 0.0,
  "sonar": 0.0
}
```

## Boot Sequence (Current)
```
[WALL-E] Starting...
[Motors] Initialised
[Display] ST7789 240x240 initialised
[Servos] PCA9685 initialised at 0x40, 9 channels
[IMU] MPU6050 not found — check wiring and I2C address
[IMU] Init complete
[Autonomy] Initializing sensors...
[Sonar] ✓ Ready  (or ⚠️ Not available)
[Compass] ⚠️ Not available - continuing without
[GPS] ⚠️ Not available - continuing without
[Waypoint] ✓ Ready
[Autonomy] Initializing behavioral engines...
[Personality] ✓ Ready
[Emotion] ✓ Ready
[Interest] ✓ Ready
[Memory] ✓ Ready
[ReturnHome] ✓ Ready
[Autonomy] ✓ Engine ready
[Battery] Initialised
[WiFi] AP mode: WALL-E-xxxx
[WebServer] Started on http://192.168.4.1
[ESP-NOW] Receiver initialised
[WALL-E] Ready
```

## Safety Features

### Watchdog Protection
- Each init function is non-blocking
- No long delays or blocking I2C waits
- ESP32 watchdog won't trigger

### Sensor Validity Checks
All update functions check initialization status:
```cpp
void sonarUpdate(uint32_t now) {
  // Safe to call even if init failed
  if (!s_initialized) return;
  // ...
}
```

### Autonomous Override
- Joystick ALWAYS has priority
- If sensors fail during operation, autonomy safely stops
- Manual control never affected by sensor failures

## Testing Checklist

### Minimal Configuration (No External Sensors)
- [ ] WALL-E boots successfully
- [ ] WebUI accessible at 192.168.4.1
- [ ] CYD controller connects via ESP-NOW
- [ ] Manual drive works perfectly
- [ ] Servos respond to CYD inputs
- [ ] Autonomous mode can be enabled (but does nothing useful)

### With Sonar Only
- [ ] Sonar readings shown in WebUI
- [ ] Basic obstacle awareness works
- [ ] Exploration possible without GPS/Compass

### With Full Sensor Suite
- [ ] All sensors report ✓ Ready
- [ ] Full autonomous exploration works
- [ ] Waypoint navigation functional
- [ ] Return-to-home works when battery low

## Hardware Requirements

### Minimum (Core Functionality)
- ESP32-S3 Base Brain ✓
- L298N Motor Driver ✓
- ST7789 Display ✓
- PCA9685 Servo Driver ✓

### Optional (Enhanced Autonomy)
- HC-SR04 Sonar (GPIO 26/27)
- HMC5883L/QMC5883L Compass (I2C 0x1E/0x0D)
- NEO-6M GPS Module (UART2)
- MPU6050 IMU (I2C 0x68)

## Next Steps
1. Upload and verify boot sequence completes
2. Test WebUI access
3. Test CYD manual control
4. Add sensors one-by-one to verify each works
5. Test full autonomous mode with all sensors

## Debug Commands
```
ESP.getFreeHeap()      // Check available RAM
esp_get_free_heap_size() // Alt heap check
esp_task_wdt_reset()   // Manual watchdog feed (if needed)
```
