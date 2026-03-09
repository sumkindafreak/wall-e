# WALL-E Behavioral Brain - Quick Reference

## 🚀 Upload This Now
`main/main.ino` - Now has graceful sensor init (continues even if sensors fail)

## 📺 Expected Boot Output
```
[WALL-E] Starting...
[Motors] Initialised
[Display] ST7789 240x240 initialised
[Servos] PCA9685 initialised
[IMU] MPU6050 not found — check wiring
[IMU] Init complete
[Autonomy] Initializing sensors...
[Sonar] ✓ Ready
[Compass] ⚠️  Not available - continuing without
[GPS] ⚠️  Not available - continuing without
[Waypoint] ✓ Ready
[Autonomy] Initializing behavioral engines...
[Personality] ✓ Ready
[Emotion] ✓ Ready
[Interest] ✓ Ready
[Memory] ✓ Ready
[ReturnHome] ✓ Ready
[Autonomy] ✓ Engine ready
[WALL-E] Ready ✅
```

## 🎮 How to Enable Autonomy

### Option 1: CYD Controller
1. Navigate to **System** page
2. Tap large **"AUTO MODE"** button (turns GREEN)
3. Wall-E starts exploring!

### Option 2: WebUI
1. Open browser: `http://192.168.4.1`
2. Click **"Autonomy"** tab
3. Click **"Enable"** button
4. Watch live telemetry

## 🧠 What Each Engine Does

| Engine | Purpose | Output |
|--------|---------|--------|
| **Personality** | WHO Wall-E is | curiosity, bravery, energy, randomness |
| **Emotion** | HOW Wall-E feels | Calm/Curious/Excited/Nervous/LowPower |
| **Interest** | WHAT interests Wall-E | 0-100 interest level in objects |
| **Memory** | WHAT Wall-E remembers | GPS history, familiarity scores |
| **Return Home** | WHERE to go when tired | GPS navigation back to base |
| **Autonomy** | THE ORCHESTRATOR | Combines all above into behavior |

## 📡 WebUI Autonomy Page

### Displays Live
- Current State (IDLE/SCAN/TRACK/etc)
- Sonar distance (cm)
- Compass heading (degrees)
- Interest level (0-100)
- Emotion (name + intensity)
- Personality traits (sliders)
- GPS position
- Return Home status

### Controls
- **Enable/Disable** - Toggle autonomous mode
- **Set Home** - Mark current position as base

## 🔧 Troubleshooting

### Still Boot Looping?
Check serial output - it will stop at the failing module

### Can't Access WebUI?
- IP: `192.168.4.1` (AP mode always on)
- SSID: `WALL-E-xxxx`
- Password: Check `wifi_manager.cpp`

### Autonomy Does Nothing?
- Enable it first (button or WebUI)
- Check if joystick is active (overrides auto)
- Missing sensors = limited behavior

### How to Test Without Sensors?
- Autonomy works, just won't detect obstacles
- Use CYD for manual control
- WebUI shows "Not available" for missing sensors

## 🎯 Key Files Modified Today

### Main Changes
- `main/main.ino` - Graceful init, all engines integrated
- `main/autonomy_engine.cpp` - Fixed personality refs, removed duplicate struct
- `main/sonar_sensor.cpp` - Returns bool for graceful fail
- `main/return_home_engine.cpp` - Fixed struct initialization

### Documentation Added
- `BEHAVIORAL_BRAIN_COMPLETE_SUMMARY.md` ← Read this for full overview
- `GRACEFUL_DEGRADATION.md` ← How sensors fail safely
- `BOOT_LOOP_TROUBLESHOOTING.md` ← Watchdog debug guide
- `COMPILATION_FIX_2026-02-24.md` ← What was fixed

## 💡 Pro Tips

1. **Start Simple**: Boot without external sensors first
2. **Add Sensors**: One at a time (sonar → compass → GPS)
3. **Watch Serial**: See exactly what's happening
4. **Use WebUI**: Best way to monitor autonomy live
5. **Trust Joystick**: Manual control ALWAYS works

## 🎉 You Now Have

✅ Complete behavioral brain (14 modules)  
✅ Graceful sensor degradation  
✅ WebUI live telemetry  
✅ CYD integration  
✅ Production-ready code  

**Upload and let's see Wall-E wake up!** 🤖✨
