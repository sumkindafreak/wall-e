# Profile System Complete!

## 🎯 3 Profiles Implemented

### Profile 0: Kid Mode 👶
**Safe, slow, limited**
- 50% max speed
- 15% deadzone (large)
- 50% expo (very gentle)
- Slow head movements (0.5x)
- Only animations 0, 1, 2 allowed
- 3 second auto-stop timeout
- **Button Mapping**:
  - Button 2 → Bootup animation
  - Button 3 → Inquisitive animation
  - Button 4 → Happy sound
  - Button 7 → E-STOP

### Profile 1: Demo Mode 🎭
**Balanced for demonstrations** (DEFAULT)
- 80% max speed
- 8% deadzone (normal)
- 20% expo (light)
- Normal head movements (1.0x)
- All animations allowed
- 5 second auto-stop timeout
- **Button Mapping**:
  - Button 2 → Bootup animation
  - Button 3 → Inquisitive animation
  - Button 4 → Surprised animation
  - Button 5 → Sound effect
  - Button 6 **(was deadman, now also)** → Profile cycle

### Profile 2: Advanced Mode ⚡
**Full control, no limits**
- 100% max speed
- 3% deadzone (minimal)
- 0% expo (linear, fastest response)
- Fast head movements (1.5x)
- All animations allowed
- No auto-stop timeout
- **Button Mapping**:
  - Button 2 → Bootup animation
  - Button 3 → Inquisitive animation
  - Button 4 → Surprised animation
  - Button 5 → Reset animation
  - Button 6 → Profile cycle

## 🎮 How to Use

### Switching Profiles:
**Option 1**: Press Button 6 (cycles through profiles)
**Option 2**: Via code:
```cpp
profileSet(PROFILE_KID);      // Set Kid mode
profileSet(PROFILE_DEMO);     // Set Demo mode
profileSet(PROFILE_ADVANCED); // Set Advanced mode
```

### Current Profile:
```cpp
Profile* p = profileGet();
Serial.println(p->name);  // Prints "Kid", "Demo", or "Advanced"
```

## 💾 Automatic Saving

- **Profile selection is saved to flash** (ESP32 Preferences)
- **Persists through power cycles**
- **Loads automatically on boot**

## 🔧 Features Implemented

### Per-Profile Settings:
✅ Joystick deadzone (0.03 - 0.15)
✅ Joystick expo curve (0.0 - 0.5)
✅ Max speed limit (0.5 - 1.0)
✅ Head sensitivity multiplier (0.5 - 1.5)
✅ Servo speed limits
✅ Animation permissions (allow list)
✅ Button action mappings
✅ Safety timeouts

### Button Action Types:
- `BTN_ACTION_NONE` - Disabled
- `BTN_ACTION_ANIMATION` - Trigger animation by ID
- `BTN_ACTION_ESTOP` - Emergency stop
- `BTN_ACTION_PROFILE_CYCLE` - Switch profiles
- `BTN_ACTION_SOUND` - Play UI sound
- `BTN_ACTION_CUSTOM` - Future expansion

### Safety Features:
- Animation permissions per profile (Kid mode restricted)
- Auto-stop timeout (Kid: 3s, Demo: 5s, Advanced: disabled)
- Deadman button required (all profiles)
- Speed limiting per profile

## 📊 Profile Comparison

| Feature | Kid | Demo | Advanced |
|---------|-----|------|----------|
| Max Speed | 50% | 80% | 100% |
| Deadzone | 15% | 8% | 3% |
| Expo | 50% | 20% | 0% |
| Head Speed | 0.5x | 1.0x | 1.5x |
| Animations | 3 only | All | All |
| Auto-Stop | 3s | 5s | None |
| Response | Gentle | Balanced | Instant |

## 🎯 Current Behavior

### On Boot:
```
[Profile] Loaded profile: 1 (Demo)
[Profile] Applied: Demo
  Deadzone: 0.08, Expo: 0.20, MaxSpeed: 0.80
  HeadSens: 1.00, ServoLimit: 1.00
```

### Button Press:
```
[Profile] Button 0 → Animation 1
[Motion] Playing animation 1: Bootup (9 frames on track 1)
```

### Profile Cycle:
```
[Profile] Switched to: Advanced
[Profile] Applied: Advanced
  Deadzone: 0.03, Expo: 0.00, MaxSpeed: 1.00
  HeadSens: 1.50, ServoLimit: 1.00
```

## 🔮 Future Enhancements (Not Yet Implemented)

### Profile Selection UI:
- Touchscreen profile selector
- Visual indication of current profile
- Profile details on screen

### Servo Tuning UI:
- Per-servo calibration screens
- Min/max position adjustment
- Speed curve tuning

### Advanced Button Programming:
- Touchscreen button mapper
- Animation preview
- Custom action sequences

## 🧪 Testing

### Upload and Test:
1. **Default profile = Demo** (loads from flash)
2. **Press Button 2** → Bootup animation plays
3. **Press Button 3** → Inquisitive animation plays
4. **Hold Button 6** → Cycles to Advanced mode
5. **Notice faster response** → 100% speed, linear
6. **Power cycle** → Profile persists!

### Serial Output:
```
[Profile] Loaded profile: 1 (Demo)
[Profile] Applied: Demo
[Profile] Button 0 → Animation 1
[Profile] Switched to: Advanced
[Profile] Saved to flash
```

## 📝 Adding Custom Profiles

Edit `profiles.cpp` to add Profile 3:

```cpp
Profile profiles[PROFILE_COUNT] = {
  // ... existing profiles ...
  {
    .name = "Custom",
    .joystickDeadzone = 0.05f,
    .joystickExpo = 0.1f,
    .joystickMaxSpeed = 0.9f,
    // ... your settings ...
  }
};
```

Update `PROFILE_COUNT` in `profiles.h`:
```cpp
#define PROFILE_COUNT 4
```

---

## 🎉 Status Update

✅ **Motion Engine** (Head control + 9 servos)
✅ **Animation System** (6 animations + Simon Bluett format)
✅ **Profile System** (Kid/Demo/Advanced modes)
✅ **Button Mapping** (Per-profile actions)
✅ **Preferences Storage** (Persistent across reboots)

**Your WALL-E controller now has a complete profile system!** 🤖✨

The only remaining TODO is the **Profile Selection UI** which would add touchscreen controls for switching profiles and viewing settings.
