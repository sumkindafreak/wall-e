# EVE Dock/Base Controller (ESP32-C3 OLED Super Mini)

Standalone project — **not** part of the main Wall-E tree; safe to build on its own.

## Libraries

- Arduino-ESP32 board support (pick **ESP32S3** or **ESP32C3** to match your hardware)
- **Adafruit NeoPixel** (Library Manager)
- **Adafruit SSD1306** (Library Manager)
- **Adafruit GFX Library** (Library Manager)

## ESP32-C3 OLED board pinout

The current defaults target the small ESP32-C3 board with an onboard OLED like the one in your reference image:


| Function                 | GPIO                |
| ------------------------ | ------------------- |
| OLED SDA                 | GPIO5               |
| OLED SCL                 | GPIO6               |
| Dock UART RX             | GPIO3               |
| Dock UART TX             | GPIO4               |
| Charge MOSFET gate       | GPIO10              |
| NeoPixel chain data      | GPIO7               |
| Optional dock button     | disabled by default |


The onboard OLED uses I2C, so do not put the EVE pogo UART on GPIO5/GPIO6. If you wire a dock button later, set `DOCK_PIN_BUTTON` in `pin_config.h` to a spare safe GPIO.

## EVE to Dock UART contract

This dock uses the same pogo UART pins that EVE normally uses for the WALL-E hand link. WALL-E and the charger/dock are expected to be mutually exclusive physical peers on that connector.

The dock now speaks EVE's framed UART protocol:

1. EVE sends `MSG_EVE_HELLO` while waiting for a peer.
2. The dock replies with `MSG_WALL_E_ACK` and JSON like `{"session":123,"peer":"eve_dock_c3"}`.
3. EVE replies with `MSG_EVE_READY`.
4. The dock sends `MSG_MODE_DOCK`.
5. EVE sends `MSG_EVE_HEARTBEAT` every `EVE_HEARTBEAT_MS`.
6. Charging is enabled only after the framed handshake succeeds and is cut if heartbeats stop for `DOCK_LOST_NO_RX_MS`.
7. Button press on the dock sends `MSG_EVE_COMPANION` with `{"m":"dock_button"}` for future behavior hooks.

The local `eve_protocol.h` mirrors EVE's protocol constants so this sketch can still build as a standalone Arduino project.

## Wiring (typical)


| Signal                     | Notes                                                                             |
| -------------------------- | --------------------------------------------------------------------------------- |
| Dock TX / GPIO4 -> EVE RX  | Cross UART                                                                        |
| Dock RX / GPIO3 <- EVE TX  |                                                                                   |
| GND                        | Common                                                                            |
| MOSFET gate                | `DOCK_PIN_CHG_GATE` (default GPIO 10) — verify N/P-channel vs your charger wiring |
| NeoPixel chain             | `DOCK_PIN_NEOPIXEL` (default GPIO 7), two pixels daisy-chained                    |
| Button                     | Optional; `DOCK_PIN_BUTTON` is disabled by default                                |


Optional pack sense: set `DOCK_PIN_VBAT_SENSE` in `pin_config.h` and calibrate `DOCK_VBAT_SENSE_OK_`*.

Startup charge prime: `DOCK_STARTUP_CHARGE_PRIME_MS` defaults to `30000`, so the MOSFET turns on after dock boot. If EVE is so flat that there is no UART/status activity after that window, `DOCK_KEEP_CHARGE_ON_NO_EVE_ACTIVITY` keeps charging on and red-flashes the dock while waiting for EVE to init. Once EVE starts talking, normal UART-controlled dock logic takes over.

NeoPixel chain order:

1. Pixel 0 = status indicator.
2. Pixel 1 = dock aura / charging glow.

## Bench validation

1. Flash EVE and confirm it is waiting for a UART peer.
2. Flash the dock with the board set to your actual chip family.
3. Connect only GND, crossed UART TX/RX, and USB power first.
4. Confirm the OLED shows `EVE DOCK` and `CHARGING` / `MOSFET ON` while a flat/no-UART EVE is being primed; the dock should red-flash while waiting for EVE init.
5. Confirm dock serial logs `RX MSG_EVE_HELLO`, `WALL_E_ACK`, `RX MSG_EVE_READY`, then `MSG_MODE_DOCK`.
6. Confirm EVE logs `peer=eve_dock_c3` and `DOCK`.
7. Confirm the OLED changes to `CHARGING` and `MOSFET ON` when charging is enabled.
8. Connect the charge-gate hardware only after the framed handshake works.
9. Lift EVE off the pogo pins and verify charging turns off after `DOCK_LOST_NO_RX_MS`.
10. Put EVE back on WALL-E's pogo connection and confirm the normal WALL-E ACK path still works.

## Tuning

All primary constants live in `pin_config.h` (baud, pins, charge polarity, scan timing, heartbeat timeout).

## Watchdog (`rst:0x8` TG1WDT) — fixed in firmware

If the main loop starves the **task watchdog** (long UART drain, tight work), the chip resets. This project now: caps UART bytes per `loop()`, throttles NeoPixel `show()`, avoids resetting LED mode every frame, and calls `yield()` in `loop()`. If you still WDT, check pin wiring (floating UART RX) and that `DOCK_PIN_NEOPIXEL` is valid for your module.

## Build

1. Open `eve_dock_esp32s3.ino` in the Arduino IDE. Arduino requires the main `.ino` name to match this folder name; the firmware itself is configured for the ESP32-C3 OLED board.
2. **Tools → Board** must match the chip. If the boot log says **ESP32-C3** (e.g. `ESP-ROM:esp32c3-…`), choose **ESP32C3 Dev Module** (or your board package), not ESP32-S3.
3. For first boot, leave `DOCK_SAFE_BOOT_ONLY` set to `1` in `pin_config.h`. Serial Monitor should show `[BOOT] EVE dock sketch reached setup()` and `[SAFE] alive`.
4. If you only see the ESP-ROM banner, set **USB CDC On Boot** to **Enabled**, re-upload, open Serial Monitor at `115200`, then press reset.
5. After serial is proven, set `DOCK_SAFE_BOOT_ONLY` to `0` to enable OLED, MOSFET, NeoPixel, and EVE UART.

