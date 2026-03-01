# WALL-E Master Controller — Enhanced CYD UI

## Overview
Professional ESP32-2432S028R (CYD) WALL-E controller with direct TFT_eSPI drawing, state machine architecture, animations, audio feedback, and zero-flicker rendering.

## Features Implemented

### 1. State Machine Architecture
- **InputMode**: `TOUCHSCREEN` / `PHYSICAL_JOYSTICK`
- **Page**: `DRIVE` / `BEHAVIOUR` / `SYSTEM`
- **ControlAuthority**: `LOCAL` / `AUTONOMOUS` / `SUPERVISED` / `SAFETY`
- Compile-time flag: `USE_PHYSICAL_JOYSTICKS` (0=touchscreen, 1=physical)

### 2. Live Telemetry Strip (Top Bar)
- Battery percentage bar (green when connected, amber when disconnected)
- Voltage, temperature, current, packet rate display
- Mode indicator (MANUAL / AUTO / E-STOP)
- Updates only when values change (zero-flicker)

### 3. Control Authority Indicator
- Color-coded: GREEN (local), BLUE (auto), YELLOW (supervised), RED (safety)
- Subtle pulse effect for AUTO/SUPERVISED/SAFETY modes
- Displays at top-right corner

### 4. Animated Eye System
- 32×32 pixel eye region (top-right)
- Random blink every 4–8 seconds
- Mood-based appearance:
  - Curious: Normal (8px wide)
  - Happy/Excited: Wide (12px, yellow glow)
  - Shy/Tired: Narrow (5px, orange glow)
  - E-STOP: Flash wide (16px, red glow)
- Mood glow background changes color

### 5. Audio Feedback (Non-Blocking)
- Sounds: CLICK, WARNING, CONFIRM, ERROR, MODE_CHANGE, E-STOP
- `tone()` on GPIO 4
- Never blocks packet transmission
- State-driven timing via `millis()`

### 6. Advanced Input Gestures
- **Long-press (2s)** bottom-right corner → Quick Action overlay
- **Triple-tap** top-left corner → Toggle Advanced Mode

### 7. Quick Action Overlay
Opens on long-press:
- Calibrate IMU
- Reset Motors
- Supervised Mode
- Reboot Base

### 8. Advanced Mode (Hidden Feature)
Triple-tap top-left to reveal:
- Raw motor output %
- IMU tilt
- CPU load
- Packet latency

### 9. Physical Joystick Layout (Future)
When `USE_PHYSICAL_JOYSTICKS 1`:
- Left half: Battery voltage graph (scrolling)
- Right half: Behaviour grid
- Drive page hidden (no virtual joysticks)
- CYD becomes telemetry + command console

## Architecture

### Files
- **`ui_state.h/cpp`** — Global state machine, input mode, page, control authority
- **`ui_draw.h/cpp`** — All rendering: static pages, dynamic updates, overlays
- **`animation_system.h/cpp`** — Eye blink, pulse effects, millis-driven
- **`audio_system.h/cpp`** — Non-blocking tone playback
- **`touch_input.h/cpp`** — Joystick math, long-press, triple-tap detection
- **`packet_control.h/cpp`** — 50 Hz ESP-NOW packet send
- **`espnow_control.h/cpp`** — ESP-NOW send/receive, packet rate tracking
- **`protocol.h`** — ControlPacket, TelemetryPacket, DriveState

### Rendering Strategy
- **Static draw**: Called once per page change (`uiDrawCurrentPage()`)
- **Dynamic update**: Region-based redraws only when values change
- **No full-screen flicker**: Separate static/dynamic layers
- **No blocking**: All animations use `millis()` timers

### Main Loop Flow
```
1. touchUpdate() → detect zones, gestures
2. Safety lock: 200ms no touch → zero speeds
3. Zone actions: E-STOP, page nav, overlay, advanced mode
4. packetUpdate(50 Hz)
5. audioUpdate()
6. Static redraw if g_needStaticRedraw
7. Dynamic update: telemetry strip, control authority, joystick dots, eye
8. delay(5)
```

## Performance
- ✅ 50 Hz packet transmission maintained
- ✅ Responsive touch input
- ✅ No `delay()` blocking
- ✅ Region-based redraw only
- ✅ State-driven logic

## What You Should See

### Drive Page (Touchscreen Mode)
- **Top bar**: "WALL-E" title, CTRL authority indicator (top-right)
- **Telemetry strip**: Battery bar, voltage, temp, current, packet rate, mode
- **Eye**: 32×32 region (top-right) with blink animation and mood glow
- **Joysticks**: Left/Right circles with white dots
- **E-STOP**: Red button (center bottom)
- **Nav buttons**: "Behav" and "System" (bottom-right)

### Behaviour Page
- Grid of mood buttons
- Eye visible
- "Back" button

### System Page
- Telemetry display
- "Back" button

## Testing
1. Boot → Should see telemetry strip with "0.00V | 0.0C | 0.00A | 0p/s | MANUAL"
2. Touch joysticks → White dots should move
3. Watch eye → Should blink every 4–8 seconds
4. Long-press bottom-right → Quick action overlay
5. Triple-tap top-left → Advanced mode overlay

## Notes
- `character_layer.h/cpp` is deprecated (replaced by `animation_system`)
- Telemetry will show zeros until ESP-NOW connection established
- Audio requires speaker/buzzer on GPIO 4
