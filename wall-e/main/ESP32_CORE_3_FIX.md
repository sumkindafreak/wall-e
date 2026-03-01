# ESP32 Arduino Core 3.x Compatibility Fix

## Issues Fixed

### 1. ESP-NOW Callback Signature Change ✅

**Old (ESP32 Core 2.x):**
```cpp
static void onRecv(const uint8_t* mac, const uint8_t* data, int len)
```

**New (ESP32 Core 3.x):**
```cpp
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len)
```

**Changes:**
- First parameter changed from `mac` to `info` struct
- MAC address now accessed via: `info->src_addr` instead of `mac`

### 2. Missing Header Include ✅

**Added:**
```cpp
#include "battery_monitor.h"
```

This provides the `BatteryData` struct definition needed for telemetry sending.

## Files Modified

- `main/espnow_receiver.cpp`
  - Updated callback signature to ESP32 Core 3.x format
  - Added `battery_monitor.h` include
  - Changed `memcpy(s_controllerMac, mac, 6)` → `memcpy(s_controllerMac, info->src_addr, 6)`

## Compatibility

✅ **ESP32 Arduino Core 3.0+** (current)  
⚠️ **Not compatible with Core 2.x** (old callback signature)

If you need to support both versions, use:
```cpp
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  memcpy(s_controllerMac, info->src_addr, 6);
#else
static void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  memcpy(s_controllerMac, mac, 6);
#endif
```

## Test Compilation

The code should now compile cleanly on ESP32 Arduino Core 3.x without errors.
