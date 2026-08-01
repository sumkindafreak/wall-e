# EVE companion — Arduino IDE

## Folder layout

Arduino IDE 2.x expects **one sketch folder** whose name matches the `.ino` file:

```
arduino_ide/
  README.md          (this file)
  EVE_Companion/
    EVE_Companion.ino
    config.h
    eve_protocol.h
    uart_link.h
    uart_link.cpp
    state_machine.h
    state_machine.cpp
    ... (other .h / .cpp tabs)
```

**Open** `EVE_Companion/EVE_Companion.ino` in Arduino IDE (File → Open). All `.cpp` / `.h` files in the same folder appear as tabs and compile together.

## Board setup

1. Install **esp32** by Espressif Systems:  
   *File → Preferences → Additional boards manager URLs* (if needed), then *Tools → Board → Boards Manager* → search **esp32** → install current stable (2.x or 3.x).

2. Select board: **ESP32S3 Dev Module** (or your exact module).

3. For **N16R8** (16 MB flash, 8 MB OPI PSRAM), set:
   - **Flash size:** 16 MB (if listed)
   - **PSRAM:** OPI PSRAM / Enabled (wording varies by core version)
   - **USB CDC On Boot:** Enabled (often needed for USB serial on S3)

4. **Port:** pick the COM port for your USB cable.

## Libraries

### Bundled with the sketch (no extra install)

These `.cpp` / `.h` files live **inside** `EVE_Companion/` and compile as sketch tabs:

- **`walle_i2s_wav_player`** — SD → WAV → I2S audio (same as `lib/walle_i2s_audio` in PlatformIO)

After changing the shared library under repo `lib/walle_i2s_audio/`, refresh the IDE copy:

```bash
cd eve && bash scripts/sync_arduino_i2s_lib.sh
```

Optional: install as a global library instead — copy `arduino_ide/libraries/walle_i2s_audio` to your Arduino **libraries** folder (e.g. `Documents/Arduino/libraries/`), then you may remove the duplicate tabs from the sketch folder if you prefer.

### Install via Library Manager

- **ArduinoJson** version **6.x** (Benoit Blanchon)

### When `EVE_ENABLE_EYES` is 1

You also need **LVGL 9** and **Arduino_GFX** (see `PHASE_J_EVE_FACE.md`). With eyes off (default in `config.h`), you can verify without them.

### When `EVE_ENABLE_BATTERY_MONITOR` + INA219

Install **Adafruit INA219** and **Adafruit BusIO** from Library Manager.

## Compile & upload

1. Open `EVE_Companion.ino`.
2. Click **Verify** then **Upload**.
3. Open **Serial Monitor** at **115200** baud.

You should see `[EVE][UART]` and periodic `EVE_HELLO` until WALL-E answers with a framed `WALL_E_ACK`.

## Keeping in sync with PlatformIO

From repo `eve/` folder, run:

```bash
bash scripts/sync_arduino_ide.sh
```

This copies `include/`, `src/` (except `main.cpp`), `awareness/`, `walle_i2s_audio`, and regenerates `EVE_Companion.ino` from `src/main.cpp`.

After pulling git changes, **run sync again** before opening Arduino IDE.
