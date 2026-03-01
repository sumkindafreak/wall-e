# I2C Joystick + Button Integration

## Added Files

1. **i2c_devices.h/cpp** - I2C bus init and scanner
2. **ads1115_input.h/cpp** - ADS1115 analog joystick processing
3. **sx1509_input.h/cpp** - SX1509 GPIO expander button handling

## Integration

### Hardware

```
ADS1115 (0x48):
  A0 → Joy1_X (Head Pan, future)
  A1 → Joy1_Y (Head Tilt, future)
  A2 → Joy2_X (Turn)
  A3 → Joy2_Y (Throttle)

SX1509 (0x3E):
  Pin 0 → BTN_JOY1 (Right eyebrow)
  Pin 1 → BTN_JOY2 (Left eyebrow)
  Pin 2-5 → BTN_EXTRA1-4 (Programmable)

I2C Bus:
  SDA = GPIO27
  SCL = GPIO22
```

### Software Changes

**wall_e_master_controller.ino:**

- Added includes for I2C modules
- `setup()` now calls:
  - `i2cInit()` - Initialize I2C bus
  - `i2cScan()` - Scan for devices
  - `ads1115Init()` - Calibrate joysticks (keep centered)
  - `sx1509Init()` - Configure buttons

- `loop()` now:
  - Calls `ads1115Update()` + `sx1509Update()` each cycle
  - **Physical joystick overrides touch input**
  - Adds button action: **Both joystick buttons = E-STOP**

## Features

### Joystick Processing

- **Auto-calibration** on boot (100 samples, keep centered)
- **Deadzone** (default 8%)
- **Exponential curve** (default 30% cubic)
- **Tank-drive mixing** (Joy2_Y = throttle, Joy2_X = turn)

### Button Handling

- **Debouncing** (30ms)
- **Edge detection** (pressed/released flags)
- **Held state** tracking

## Tuning

Add to your system settings page:

```cpp
setJoystickDeadzone(0.12f);  // 12% deadzone
setJoystickExpo(0.5f);       // 50% cubic response
setJoystickMaxOutput(0.8f);  // Limit to 80% speed
```

## Required Libraries

1. **Adafruit_ADS1X15** (Install via Library Manager)
2. **SparkFun SX1509** (Install via Library Manager)

## Testing

Upload and open Serial Monitor (115200 baud):

1. Boot should show:
   ```
   [I2C] Init OK (400kHz)
   --- I2C Bus Scan ---
     [0x48] found (ADS1115)
     [0x3E] found (SX1509)
   [ADS1115] ✓ Calibration complete
   [SX1509] ✓ Configured
   ```

2. Move joysticks - should see drive commands
3. Press buttons - should see `[Button] BTNx pressed`

## Priority System

**Control Authority (highest to lowest):**

1. Both joystick buttons held → E-STOP
2. Touch E-STOP zone → E-STOP
3. Physical joystick active → Physical control
4. Touch joystick active → Touch control
5. Timeout (200ms no input) → STOP

---

✅ **Your existing controller now supports dual input!**
