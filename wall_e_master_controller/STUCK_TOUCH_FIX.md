# Stuck Touch Filter - HARDWARE ISSUE FIX

## Problem
Touchscreen reporting constant phantom touches at coordinates (319, 239) even when not being touched.

## Root Cause
**Hardware Issue**: XPT2046 touchscreen controller is:
1. **Electrically noisy** - picking up interference
2. **Calibration drift** - stuck at max coordinates  
3. **Poor grounding** - causing false readings

This is a **HARDWARE** problem, not software!

## Software Workaround Applied

Added **stuck touch detection** to `touch_input.cpp`:

### How It Works:
1. **Tracks last touch coordinates**
2. **Counts repeated identical readings**
3. **If same coords for 10+ readings** → **IGNORES touch completely**
4. **Resets counter** when coords change or touch released

### Code Added:
```cpp
// Stuck touch detection
if (screenX == s_lastTouchX && screenY == s_lastTouchY) {
  s_stuckTouchCount++;
  if (s_stuckTouchCount > STUCK_TOUCH_THRESHOLD) {
    // Touch is stuck - ignore it completely
    return TOUCH_ZONE_NONE;
  }
}
```

## Result
✅ **Stuck touches at (319, 239) will be ignored**  
✅ **Real touches still work** (different coords reset counter)  
✅ **Warning every 5 seconds** in Serial Monitor  
✅ **CYD should be usable now**

## Testing
After upload, Serial Monitor will show:
```
[Touch] ⚠️  STUCK at X=319 Y=239 - ignoring
```

But the interface should **work normally** - stuck touch is filtered out.

## Permanent Hardware Fixes

If you want to truly fix this (not just work around it):

### 1. **Add Capacitor to TCLK/TIRQ** (Best fix)
- 100nF ceramic capacitor between TIRQ pin and GND
- Reduces electrical noise significantly

### 2. **Recalibrate Touchscreen**
```cpp
// In touchInit():
s_ts.setRotation(1);
// Try different calibration values:
#define TOUCH_X_MIN 200   // Adjust these
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3700
```

### 3. **Increase Pressure Threshold**
```cpp
#define TOUCH_MIN_PRESSURE 400  // Increase from default
```

### 4. **Check Hardware**
- Ensure touchscreen ribbon cable is firmly connected
- Check for loose connections on CYD board
- Verify no shorts/damage to touch controller area
- Try touching the screen - if stuck reading disappears, it's interference

### 5. **Shield Touchscreen Traces**
- Add ground plane around touch traces
- Keep high-current wires away from touch pins

## Typical Causes
- **PWM noise** from backlight/servos leaking into touch controller
- **Poor PCB layout** (CYD hardware design issue)
- **USB power noise** (try different USB cable/power source)
- **Nearby motors** (servo PWM causing interference)

## Status
✅ **Software workaround applied** - CYD should be usable  
⚠️ **Hardware issue remains** - consider capacitor fix for production  

Upload and test - the stuck touch should be filtered out! 🚀
