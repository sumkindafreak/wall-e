# 🔍 Auto Mode Button Troubleshooting Guide

## Where is the AUTO MODE button?

The button is on the **System Page**, NOT the main screen.

### Step-by-Step Navigation:

1. **Start from Main Screen** (with physical joysticks)
   - You should see:
     - "WALL-E Console" title
     - Battery info on LEFT
     - Behaviour buttons on RIGHT
     - **System button** (bottom left, under "Battery")

2. **Tap the "System" button**
   - Location: Left side, below battery graph
   - Should navigate you to System page

3. **On System Page, you'll see:**
   ```
   ┌─────────────────────────────┐
   │ System                      │
   ├─────────────────────────────┤
   │ Battery: -- V               │
   │ Current: -- A               │
   │ Temp: -- C                  │
   │                             │
   │ ┌─────────┐  ┌─────────┐   │
   │ │Profiles │  │ Servos  │   │
   │ └─────────┘  └─────────┘   │
   │                             │
   │ ┌─────────┐  ┌─────────┐   │  ← THIS ROW
   │ │AUTO MODE│  │Auto Info│   │
   │ │DISABLED │  │         │   │
   │ └─────────┘  └─────────┘   │
   │                             │
   │        ┌──────┐             │
   │        │ Back │             │
   │        └──────┘             │
   └─────────────────────────────┘
   ```

## Current Code Status

✅ **Code is correct and complete:**
- AUTO MODE button defined in `ui_draw.cpp` lines 467-471
- Touch zones defined in `touch_input.cpp` lines 337-344
- Toggle handler in `wall_e_master_controller.ino` lines 208-220
- ESP-NOW communication set up in `packet_control.cpp`
- Base receives flag in `espnow_receiver.cpp` line 104

## If you STILL don't see the button:

### Option 1: Code Not Uploaded
**You need to compile and upload the updated code!**

The Arduino IDE doesn't automatically upload changes. You must:
1. Open `wall_e_master_controller.ino` in Arduino IDE
2. Click **"Upload"** button (→ arrow icon)
3. Wait for compilation and upload to complete
4. Reset CYD controller

### Option 2: Not on System Page
Make sure you're actually on the System page:
- From main screen, tap the **"System"** button on the left side
- The title should change from "WALL-E Console" to "System"
- You should see battery stats at top

### Option 3: Screen Not Refreshing
Try:
- Press the BACK button to go to main screen
- Press System button again
- Or reset the CYD controller

### Option 4: USE_PHYSICAL_JOYSTICKS Check
The code I wrote works for `USE_PHYSICAL_JOYSTICKS 1` (which is your setting).

Verify in `ui_state.h` line 15:
```cpp
#define USE_PHYSICAL_JOYSTICKS 1  // Should be 1
```

## Debug: Add Serial Output

If you still can't find it, add this to your `loop()` function temporarily:

```cpp
// In wall_e_master_controller.ino loop(), add after touch zone handling:
if (zone != TOUCH_ZONE_NONE) {
  Serial.printf("[DEBUG] Page=%d Zone=%d\n", g_currentPage, zone);
}
```

Then open Serial Monitor (115200 baud) and watch for:
- When you tap System button: `Page=2 Zone=...`
- When on System page, tapping buttons should show zones

## Button Coordinates

**System Page (page=2) Touch Zones:**
- AUTO MODE toggle: X=16-116, Y=168-200
- Auto Info button: X=130-230, Y=168-200
- Back button: X=110-210, Y=204-236

## Quick Test

1. Upload the code
2. Reset CYD
3. Tap "System" (left side under Battery)
4. Look for two buttons in the 3rd row:
   - Left: AUTO MODE (RED with "DISABLED")
   - Right: Auto Info (blue/cyan)

---

## If It Works:

You should be able to:
1. Tap AUTO MODE button
2. Button turns GREEN and shows "ENABLED"
3. Base receives the autonomy flag
4. WALL-E starts autonomous behavior

## Need More Help?

Check Serial Monitor output:
- `[Autonomy] Toggled: ENABLED` - Button press worked
- `[Packet] Autonomy ENABLED` - Command sent to Base
- Watch for autonomy state changes on Base serial

---

**Most likely issue**: You haven't uploaded the updated code yet! 
Make sure to compile and upload to the CYD.
