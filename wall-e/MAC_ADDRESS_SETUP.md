# ESP-NOW MAC Address Setup Guide

## Quick Answer: **It works as-is, but you should configure it properly**

### Current Status
✅ **Broadcast mode is enabled** — The controller will work immediately but sends to ALL ESP-NOW devices nearby.

### For Production Use (Recommended)

Follow these steps to pair your specific devices:

---

## Step 1: Upload to Brain First

1. Upload `main/main.ino` to your **WALL-E Brain** (ESP32-S3)
2. Open Serial Monitor at **115200 baud**
3. Look for this line on boot:
   ```
   [ESP-NOW] Receiver ready. MAC: 12:34:56:78:9A:BC
   ```
4. **Copy that MAC address** (your actual MAC will be different)

---

## Step 2: Configure Controller

Open `wall_e_master_controller/espnow_control.cpp`

Find this section (around line 20):

```cpp
// OPTION 1: Broadcast (default - works immediately, less secure)
static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// OPTION 2: Specific MAC (recommended - uncomment and set your Brain's MAC)
// Example: {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}
// static uint8_t s_peerMac[6] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
```

**Comment out Option 1 and uncomment Option 2:**

Example if Brain MAC was `A1:B2:C3:D4:E5:F6`:

```cpp
// OPTION 1: Broadcast (default - works immediately, less secure)
// static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// OPTION 2: Specific MAC (recommended - uncomment and set your Brain's MAC)
static uint8_t s_peerMac[6] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};
```

---

## Step 3: Upload to Controller

1. Upload `wall_e_master_controller.ino` to your **CYD controller**
2. Open Serial Monitor at **115200 baud**
3. You should see:
   ```
   [ESP-NOW] Controller MAC: XX:XX:XX:XX:XX:XX
   [ESP-NOW] Target Brain MAC: A1:B2:C3:D4:E5:F6
   ```

---

## Step 4: Test Connection

1. Power on both devices
2. Controller telemetry strip should show:
   - Battery voltage updating
   - Packet rate climbing to ~50 Hz
   - Green battery bar (connected)
3. Move joysticks → Brain should respond

---

## Troubleshooting

### "No connection" / Telemetry shows zeros
- ✅ Check MAC address is correct (no typos)
- ✅ Both devices must be on same WiFi channel (0 = auto)
- ✅ Brain must be powered on and ESP-NOW initialized
- ✅ MAC format: `0xA1` not `A1` (needs `0x` prefix)

### Works in broadcast, fails with specific MAC
- Verify you copied the correct MAC from Brain's serial output
- Make sure you commented out the broadcast line
- MAC addresses are case-insensitive but must have `0x` prefix

### How to return to broadcast mode
Change back to:
```cpp
static uint8_t s_peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
```

---

## Why Set Specific MAC?

### Broadcast Mode (`0xFF:FF:FF:FF:FF:FF`)
✅ Works immediately  
✅ No configuration needed  
❌ Any ESP-NOW device receives commands  
❌ Less secure  
❌ More WiFi interference  

### Specific MAC
✅ Secure - only your Brain responds  
✅ Less interference  
✅ Better for multiple WALL-E robots nearby  
❌ Requires one-time setup  

---

## For Testing: Use Broadcast

For production/competition: Use specific MAC.
