# Boot Loop Troubleshooting - Feb 24, 2026

## Symptom
ESP32 Base Brain is in a continuous boot loop with watchdog timer reset (`rst:0x8 TG1WDT_SYS_RST`).

## Boot Sequence Observed
```
[WALL-E] Starting...
[Motors] Initialised
[Display] ST7789 240x240 initialised (Adafruit)
[Servos] PCA9685 initialised at 0x40, 9 channels
[IMU] MPU6050 not found — check wiring and I2C address
<CRASH - Watchdog Reset>
```

## Analysis
The crash occurs immediately after the IMU initialization message, before any of the new autonomy sensors can initialize.

##Possible Causes

### 1. Watchdog Timer Timeout
- **Most Likely**: One of the new init functions is blocking for too long (>5-8 seconds)
- **Location**: Between `beginIMU()` and the next serial print

### 2. Stack Overflow
- Adding many new modules might have exceeded ESP32 stack size
- Solution: Increase stack size in Arduino IDE settings or reduce local variable usage

### 3. Heap Exhaustion
- Too many dynamic allocations (Preferences, TinyGPS++, etc.)
- Check with `ESP.getFreeHeap()` before each init

### 4. I2C Bus Conflict
- Multiple sensors trying to initialize on same I2C bus
- Compass init might be hanging if no device present

### 5. Missing Hardware
- GPS module on UART2 might have initialization issues
- Compass sensor not present but code expects response

## Added Debugging

Added detailed serial logging before/after each init function:
- `[INFO] <module> init starting...`
- `[<module>] Init complete`

This will pinpoint the exact function causing the crash.

## Temporary Fixes to Test

### Option A: Comment Out Autonomy Init (TEST ONLY)
```cpp
// Comment out in setup():
// sonarInit();
// compassInit();
// gpsInit();
// etc...
```

### Option B: Add Watchdog Feed
```cpp
// In setup() between each init:
esp_task_wdt_reset();
```

### Option C: Increase Watchdog Timeout
```cpp
// At top of main.ino:
#include "esp_task_wdt.h"

// In setup(), before inits:
esp_task_wdt_init(30, false);  // 30 second timeout
```

## Next Steps

1. **Upload** the new version with detailed logging
2. **Observe** serial output to see which init function crashes
3. **Fix** the specific module causing the issue
4. **Consider** adding `yield()` or `delay(1)` between init functions to feed watchdog

## Hardware Checklist

- [ ] GPS module connected to UART2 (RX/TX pins defined correctly)?
- [ ] Compass sensor on I2C bus (address 0x1E or 0x0D)?
- [ ] Sonar sensor GPIO pins (26, 27) not conflicting with other hardware?
- [ ] MPU6050 IMU present (currently shows "not found")?

## Code Changes Made

1. Added initialization for all behavioral brain engines in `main.ino`
2. Fixed `Personality` struct duplication
3. Added detailed serial debug logging
4. Ensured proper initialization order (sensors → engines → autonomy)
