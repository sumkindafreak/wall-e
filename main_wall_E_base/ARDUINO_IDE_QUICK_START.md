# Arduino IDE Quick Start (WALL-E Base)

## Which sketch to open

The Base has two possible layouts. **Use the one that matches your setup:**

### Option A: `main_wall_E_base` folder (root sketch)
- **File → Open** → select the `main_wall_E_base` folder
- Open `main.ino` (in that folder)
- Arduino compiles: `main.ino` + `espnow_receiver.cpp` (and other .cpp in that folder)

### Option B: `main_wall_E_base/main` subfolder
- **File → Open** → select the `main_wall_E_base/main` folder
- Open `main.ino` (in the `main` subfolder)
- Arduino compiles: `main.ino` + all .cpp in the `main` folder (including `espnow_receiver.cpp`)

---

## Upload to Base (WALL-E Brain)

1. Connect the **Base** ESP32-S3 via USB
2. **Tools → Board** → ESP32 Arduino → **ESP32S3 Dev Module**
3. **Tools → Port** → select the Base’s COM port
4. **Sketch → Upload**
5. Open Serial Monitor at 115200 baud

---

## How to tell the new firmware is running

On boot you should see:
```
[ESP-NOW] Receiver ready. Use this MAC for controller: XX:XX:XX:XX:XX:XX
[ESP-NOW] Servo targets from Joy1 ENABLED (st0=pan st1=tilt)
```

If that line is missing, the servo-enabled firmware is not on the Base. Check that:
- You opened the correct sketch folder
- You chose the Base’s COM port (not the Controller’s)
- The upload completed without errors
