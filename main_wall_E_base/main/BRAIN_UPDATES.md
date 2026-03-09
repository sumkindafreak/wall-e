# WALL-E Brain (main.ino) — Updated for Enhanced CYD Controller

## Changes Made

### 1. Updated ESP-NOW Protocol (`espnow_receiver.cpp`)

**Old packet (3 fields):**
```cpp
struct {
  int8_t leftSpeed;
  int8_t rightSpeed;
  uint8_t buttons;
}
```

**New packet (7 fields):**
```cpp
struct {
  int8_t   leftSpeed;       // -100 to +100
  int8_t   rightSpeed;      // -100 to +100
  uint8_t  driveMode;       // 0=manual, 1=precision
  uint8_t  behaviourMode;   // mood override (0-4)
  uint8_t  action;          // trigger event (scan, beep, etc.)
  uint16_t systemFlags;     // E-STOP, autonomous, etc.
}
```

### 2. New Features Handled

#### E-STOP (Priority 1)
- Checks `systemFlags & FLAG_ESTOP`
- Immediately stops motors
- Logs to serial

#### Precision Mode
- When `driveMode == 1`, speed is halved
- Allows fine control for delicate maneuvers

#### Actions (Future Expansion)
- ACTION_SCAN → servo scan sequence
- ACTION_BEEP → sound feedback
- ACTION_LOOKAROUND → head movement
- ACTION_IMU_CAL → recalibrate IMU
- ACTION_MOTOR_RESET → reset motor controller

#### System Flags
- `FLAG_ESTOP` — Emergency stop
- `FLAG_AUTONOMOUS` — Autonomous mode active
- `FLAG_PRECISION` — Precision drive mode
- `FLAG_SUPERVISED` — Supervised autonomous
- `FLAG_SLEEP` — Low-power state

### 3. Telemetry Sending

**New function: `espnowSendTelemetry()`**

Sends back to controller every 100ms:
- Battery voltage (from `battery_monitor`)
- Current draw (from `battery_monitor`)
- Temperature (placeholder: 25.0°C — add sensor later)
- Mood state (placeholder: 0 = curious)
- Autonomous state (0 = manual)
- Safety state (0 = ok)

**Added to `main.ino` loop:**
```cpp
if ((millis() - lastTelemSendMs) >= TELEM_SEND_INTERVAL_MS) {
  espnowSendTelemetry();
  lastTelemSendMs = millis();
}
```

### 4. Controller MAC Tracking

The Brain now remembers the controller's MAC address when it receives a packet, so it can send telemetry back to the correct device.

## What Now Works

### From Controller → Brain
- ✅ Tank drive (left/right speed)
- ✅ E-STOP button (instant motor stop)
- ✅ Precision mode (future: half speed)
- ✅ Behaviour mood selection (future: triggers animations)
- ✅ Action commands (future: scan, beep, etc.)

### From Brain → Controller
- ✅ Battery voltage → telemetry strip
- ✅ Current draw → telemetry strip
- ✅ Temperature (placeholder) → telemetry strip
- ✅ Connection status → "connected" indicator
- ✅ Packet rate → shows 50 Hz when connected

## Testing

1. **Upload to Brain first**, then controller
2. Controller should show:
   - Telemetry strip updating with real battery voltage
   - Packet rate climbing to ~50 Hz
   - "connected" status
3. Test E-STOP → motors should stop instantly
4. Drive joysticks → Brain display should show stick position

## Future Enhancements

Add to Brain as needed:
- Temperature sensor (DS18B20, BME280, etc.)
- Action handlers (scan, beep, lookaround)
- Mood-based servo animations
- Autonomous navigation mode
- Sleep/wake power management

## Files Modified

- `main/espnow_receiver.h` — Added `espnowSendTelemetry()`
- `main/espnow_receiver.cpp` — Updated packet structure, E-STOP, telemetry send
- `main/main.ino` — Added 10 Hz telemetry send in loop

## Protocol Compatibility

**Controller:** `wall_e_master_controller/protocol.h`  
**Brain:** `main/espnow_receiver.cpp`  
**Status:** ✅ SYNCHRONIZED
