# Physical Joystick Visual Feedback

## 🎮 On-Screen Joystick Indicators

Visual feedback for physical ADS1115 joysticks is now displayed at the bottom of the CYD screen!

## Display Layout

```
┌─────────────────────────────────────┐
│  WALL-E       [Telemetry Strip]     │
├─────────────────────────────────────┤
│                                     │
│         [Main Content Area]         │
│                                     │
│                                     │
├─────────────────────────────────────┤
│  HEAD           🎯           DRIVE  │
│    ○                           ●    │  ← Joystick indicators
│   [Joy1]                    [Joy2]  │
└─────────────────────────────────────┘
```

## Visual Elements

### Left Indicator (Joy1 - Head Control)
- **Position**: Bottom-left (x=60, y=210)
- **Color**: Dim amber (C_ACCENT_DIM)
- **Label**: "HEAD"
- **Status**: Future - head pan/tilt control
- **Display**: Shows X/Y position even though not yet controlling head

### Right Indicator (Joy2 - Tank Drive)
- **Position**: Bottom-right (x=260, y=210)
- **Color**: Bright amber when active (C_ACCENT)
- **Label**: "DRIVE"
- **Status**: ACTIVE - controls tank tracks
- **Features**:
  - Stick lights up bright when moving
  - Line drawn from center to stick position when active
  - Shows exact throttle/turn input

## Features

### Real-Time Position
- Both indicators show live joystick positions
- Updates smoothly every frame (~200Hz)
- X-axis: Left/Right movement
- Y-axis: Up/Down movement (inverted for screen coordinates)

### Visual Feedback
- **Center dot**: Reference point (neutral position)
- **Circle boundary**: Maximum stick deflection
- **Stick indicator**: Current position (6px radius)
- **Direction line**: Joy2 only, shows input direction

### Activity Indication
- **Joy1 (dim)**: Not yet active, but shows position for testing
- **Joy2 (bright)**: Active control indicator
  - Bright amber = moving (>5% deflection)
  - Dim amber = centered (neutral)

### Constrained Display
- Stick indicators constrain to circle boundary
- Prevents visual overflow even with extreme inputs
- Mathematically accurate position mapping

## Technical Details

### Update Rate
- Called every loop iteration
- Efficiently erases old position, draws new position
- No flickering due to smart erase strategy

### Coordinate Mapping
```cpp
Screen X = Center X + (Joy X × radius)
Screen Y = Center Y - (Joy Y × radius)  // Y inverted
```

### Active Detection
```cpp
Active if: |Joy2_X| > 0.05 OR |Joy2_Y| > 0.05
```

## Color Scheme

| Element | Color | Hex | Purpose |
|---------|-------|-----|---------|
| Joy1 base | Dim Amber | 0xB360 | Inactive/future |
| Joy2 base | Bright Amber | 0xFD20 | Active drive |
| Active stick | White | 0xFFFF | Current position |
| Direction line | Dim Amber | 0xB360 | Movement vector |
| Labels | Gray | 0xAD55 | Text identification |

## Code Reference

**Function**: `uiDrawPhysicalJoystickIndicators()`
**File**: `ui_draw.cpp`
**Parameters**:
- `joy1X, joy1Y` - Normalized -1.0 to 1.0 (processed values)
- `joy2X, joy2Y` - Normalized -1.0 to 1.0 (processed values)

**Integration**:
```cpp
const JoystickState& joyVis = getJoystickState();
uiDrawPhysicalJoystickIndicators(
  joyVis.processed[JOY1_X], joyVis.processed[JOY1_Y],
  joyVis.processed[JOY2_X], joyVis.processed[JOY2_Y]
);
```

## User Experience

### At Rest
- Both circles visible with center dots
- Joy1 stick centered (dim)
- Joy2 stick centered (dim)
- Labels clearly visible

### During Movement
- Joy2 stick moves within circle
- Lights up bright amber
- Direction line appears
- Position updates smoothly

### Benefits
1. **Visual confirmation** - Know joystick is working
2. **Calibration check** - See if sticks center properly
3. **Input visualization** - Understand what robot receives
4. **Debugging** - Spot deadzone, expo, and mapping issues
5. **Professional look** - Adds polish to the interface

---

✅ **Always visible on all screens - no mode switching needed!**
