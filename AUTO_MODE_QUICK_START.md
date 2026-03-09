# 🤖 WALL-E Auto Mode Quick Start Guide

## How to Enable Autonomous Mode

### **Quick Access from System Page:**

1. From the **Main Drive Screen**, tap **"System"** button (bottom left)
2. On the **System Page**, you'll see the **"AUTO MODE"** button (bottom left)
   - **GREEN + "ENABLED"** = Autonomous mode is ON
   - **RED + "DISABLED"** = Autonomous mode is OFF
3. **Tap the AUTO MODE button** to toggle on/off

That's it! 🎉

---

## Button Layout on System Page

```
┌─────────────────────────────────────┐
│ System                              │
├─────────────────────────────────────┤
│                                     │
│ Battery: 12.4 V                     │
│ Current: 2.3 A                      │
│ Temp: 28 C                          │
│                                     │
│ ┌─────────┐  ┌─────────┐           │
│ │Profiles │  │ Servos  │           │
│ └─────────┘  └─────────┘           │
│                                     │
│ ┌─────────┐  ┌─────────┐           │
│ │AUTO MODE│  │Auto Info│           │
│ │ ENABLED │  │         │           │
│ └─────────┘  └─────────┘           │
│                                     │
│         ┌──────┐                    │
│         │ Back │                    │
│         └──────┘                    │
└─────────────────────────────────────┘
```

- **AUTO MODE button** (left): Toggle autonomy on/off
  - Changes color: GREEN (enabled) / RED (disabled)
  - Shows current state in real-time

- **Auto Info button** (right): Opens detailed autonomy page
  - Shows state, sonar, heading, GPS, waypoints
  - Full telemetry display

---

## What Happens When Auto Mode is Enabled?

### ✅ **WALL-E will:**
- **Scan** environment using neck servo
- **Detect** obstacles with ultrasonic sonar (right eye)
- **Navigate** using electronic compass
- **Approach** objects of interest
- **Investigate** different heights (curiosity behavior)
- **Wander** and explore randomly
- **Navigate** to GPS waypoints (if configured)
- **React** to nearby objects with personality

### 🛡️ **Safety Features:**
- **Your joystick ALWAYS overrides** - just move the sticks!
- Auto mode pauses for 500ms after any manual input
- Stops automatically if sensors fail (timeout protection)
- IMU tilt detection triggers emergency stop
- E-STOP works at all times

---

## Autonomous States

When enabled, WALL-E cycles through these states:

| State | Description |
|-------|-------------|
| **IDLE** | Waiting, subtle life behaviors (head sway, blinking) |
| **SCAN** | Sweeping neck left-right, measuring distances |
| **EVALUATE** | Processing scan results, choosing next action |
| **APPROACH** | Moving toward object of interest |
| **INVESTIGATE_HEIGHT** | Looking up/down at different angles |
| **REACT** | Responding to close obstacle (backup, tilt head) |
| **WANDER** | Random exploration, picking new heading |
| **AVOID** | Backing away from close obstacle |
| **ORIENT** | Turning toward target heading using compass |
| **EXPLORE_LOOP** | Continuous exploration cycle |
| **NAVIGATE_WAYPOINT** | GPS-guided navigation to waypoint |

---

## Profile Personalities

Different profiles have different autonomy personalities:

### **Kid Mode** (Profile 0)
- Lower energy (slower movement)
- Medium curiosity
- Cautious approach
- Safe for beginners

### **Demo Mode** (Profile 1)
- High curiosity (investigates more)
- Medium energy
- Balanced exploration
- Great for showing off

### **Advanced Mode** (Profile 2)
- Full autonomy features
- High energy
- Maximum randomness
- Adventurous behavior

---

## Monitoring Auto Mode

### On the System Page:
- Button color shows enabled/disabled state
- Updates in real-time from Base telemetry

### On the Auto Info Page (tap "Auto Info"):
- **State**: Current autonomy state name
- **Sonar**: Distance reading in cm
- **Heading**: Compass direction (0-360°)
- **GPS**: Latitude/Longitude (if GPS has fix)
- **Waypoint**: Distance and bearing to target (if navigating)

---

## Tips & Tricks

1. **Quick Override**: Just move any joystick - auto mode pauses instantly
2. **Test Safety**: Enable auto mode, let WALL-E scan, then move the joystick - you'll see immediate response
3. **Watch the State**: Open Auto Info to see WALL-E's "thought process" in real-time
4. **Experiment with Profiles**: Switch profiles to see different exploration behaviors
5. **Indoor Exploration**: Works great without GPS - uses compass and sonar only
6. **Outdoor Navigation**: Add GPS waypoints for autonomous outdoor missions

---

## Hardware Required

For full autonomous functionality:

| Component | Location | Purpose |
|-----------|----------|---------|
| **HC-SR04 Sonar** | Right eye | Obstacle detection |
| **HMC5883L/QMC5883L Compass** | Base I2C | Heading awareness |
| **GPS Module (NEO-6M/7M/8M)** | Base Serial | Waypoint navigation |
| **MPU6050 IMU** | Base I2C | Tilt safety detection |
| **Neck Pan Servo** | Base PCA9685 Ch0 | Scanning motion |

*Note: System works with partial sensors - gracefully degrades if components missing*

---

## Communication Architecture

```
┌──────────────────┐         ESP-NOW          ┌──────────────────┐
│                  │◄─────────50Hz─────────────┤                  │
│  CYD Controller  │      Control Packets      │   Base ESP32     │
│  (Master UI)     │──────────────────────────►│   (Brain)        │
│                  │◄────────10Hz──────────────┤                  │
└──────────────────┘    Telemetry Packets     └──────────────────┘
        │                                               │
        │                                               │
    User Input                                    Sensors + Motors
    - Touch                                       - Sonar
    - Joysticks                                   - Compass
    - Buttons                                     - GPS
                                                  - IMU
                                                  - Motors
                                                  - Servos
```

---

## Need Help?

**Auto mode not working?**
1. Check sensors are connected (see hardware table above)
2. Open Auto Info page - check for sensor readings
3. Look at Serial Monitor on Base for autonomy debug messages
4. Verify ESP-NOW connection (should see telemetry updates)

**WALL-E acting strange?**
- Try different profile for different personality
- Disable/re-enable auto mode to reset state machine
- Check sonar isn't blocked or giving false readings

**Want more control?**
- Manual override ALWAYS works - just use joysticks
- E-STOP instantly disables everything
- Autonomous mode respects all safety features

---

**Enjoy your autonomous WALL-E!** 🤖✨
