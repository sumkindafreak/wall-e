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

Install with *Sketch → Include Library → Manage Libraries…*:

| Library | Version | Needed for |
|---|---|---|
| ArduinoJson (Benoit Blanchon) | 6.21.x (not 7) | UART protocol, status JSON |
| Adafruit NeoPixel | 1.12+ | onboard RGB LED (GPIO48) and the optional strip |
| Adafruit INA219 + Adafruit BusIO | 1.2.3+ / 1.14+ | battery voltage / current |
| lvgl | 9.2.x | eye face (only compiled when `EVE_ENABLE_EYES` is 1) |
| GFX Library for Arduino (moononournation) | **1.6.9+** | eye panels. Older releases do not compile on Arduino-ESP32 3.3.x |
| Pololu VL53L1X | 1.3+ | only when `EVE_ENABLE_TOF` is 1 |

Also install the repo's shared audio library: copy (or symlink) `lib/walle_i2s_audio` into your
Arduino `libraries` folder.

With the eyes enabled, LVGL needs `lv_conf.h`. Copy `EVE_Companion/lv_conf.h` to your Arduino
`libraries` folder, next to the `lvgl` folder. It keeps C headers away from LVGL's assembler
files, so it is safe for the IDE to compile them.

**Partition:** the current build (eyes off) fits the default 1.2 MB app partition (about 80 %).
With the eyes and the other drivers enabled it is about 1.4 MB, so choose
*Tools → Partition Scheme → 16M Flash (3MB APP/9.9MB FATFS)* (Flash Size 16 MB).

## Compile & upload

1. Open `EVE_Companion.ino`.
2. Click **Verify** then **Upload**.
3. Open **Serial Monitor** at **115200** baud.

You should see `[EVE][CONFIG]`, `[EVE][UART]` and `BOOT -> WAIT_ATTACH`. Once WALL-E answers,
expect `UART link up …` and `WAIT_ATTACH -> IDLE`. Type `help` in the Serial Monitor for the
console (`diag`, `stop`, `clearstop`, `routine scan`, …). See `../EVE_LINK_SAFETY_AND_BENCH_TEST.md`.

## One source for both builds

PlatformIO compiles this same folder (`eve/platformio.ini` sets `src_dir` and `include_dir` to
`arduino_ide/EVE_Companion`). There is nothing to copy between trees any more. The UART message
IDs come from `firmware_common/include/eve_link_protocol.h` (via `eve_protocol.h`), which WALL-E's
base uses too.
