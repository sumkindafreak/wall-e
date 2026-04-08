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

## Library

Install **ArduinoJson** version **6.x**:

- *Sketch → Include Library → Manage Libraries…* → search **ArduinoJson** → install **ArduinoJson** by **Benoit Blanchon** (v6.21.x or newer).

## Compile & upload

1. Open `EVE_Companion.ino`.
2. Click **Verify** then **Upload**.
3. Open **Serial Monitor** at **115200** baud.

You should see `[EVE][UART]` and periodic `EVE_HELLO` until WALL-E answers with a framed `WALL_E_ACK`.

## Keeping in sync with PlatformIO

The files under `EVE_Companion/` are meant to stay **logically identical** to `eve/src/` + `eve/include/` from the repo root. After editing one tree, copy changes to the other if you use both tools.
