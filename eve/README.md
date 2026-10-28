# EVE companion firmware (ESP32-S3)

UART-framed link to WALL-E (hand pogo), supervised handshake, latched stop, state machine, expression routines, and drivers for eyes / ToF / servos / NeoPixel / **SD-card WAV audio over I2S** / INA219 battery monitor.

**Start here:** [`EVE_LINK_SAFETY_AND_BENCH_TEST.md`](EVE_LINK_SAFETY_AND_BENCH_TEST.md). It covers the link rules, the stop latch, modes and routines, the sound track map, battery handling, the config checklist and a bench-test sequence.

**One source tree:** PlatformIO and the Arduino IDE both compile `arduino_ide/EVE_Companion/`. There is no separate `src/` or `include/` copy.

## Requirements

- [PlatformIO](https://platformio.org/) (or Arduino IDE with manual lib install — PIO recommended).
- Board: **ESP32-S3** with **16 MB flash / 8 MB PSRAM** (N16R8 class), same family as WALL-E.

## Arduino IDE (alternative)

Open the sketch folder **`arduino_ide/EVE_Companion/`** and follow **`arduino_ide/README.md`**.

## Build (PlatformIO)

```bash
cd eve
pio run
```

## Upload

```bash
pio run -t upload
pio device monitor
```

## Configuration

Edit `arduino_ide/EVE_Companion/config.h` (used by both builds):

- `EVE_UART_TX_PIN` / `EVE_UART_RX_PIN` — hand link (default 17/18; **must match your PCB**).
- Feature flags: `EVE_ENABLE_NEOPIXEL`, `EVE_ENABLE_SERVOS`, `EVE_ENABLE_AUDIO`, etc. (keep off until wired). A driver whose pins are still `-1` stays off even when its flag is 1.
- Servo calibration (limits, rest, invert, pulse range, HOLD/RELEASE on stop), battery plausibility window, `EVE_INA219_CHARGE_SIGN`, and sound cues: see the sections in `config.h`.
- SD card SPI pins (`EVE_SD_SPI_*`) and I2S speaker pins (`EVE_I2S_*`) — see `PHASE_M_EVE_SD_I2S_AUDIO.md`.
- Copy `sd_card_template/` to the microSD root for `/config` and `/audio` layout.

## WALL-E side

Peer UART on the base must use the **crossover** convention: WALL-E TX → EVE RX, WALL-E RX ← EVE TX, **GND** common.

The base (`main_wall_E_base`) can optionally run **`eve_uart_bridge`** on **UART2** (default **TX GPIO 18**, **RX GPIO 17** — see `main/eve_uart_bridge.h`) and expose link status as **`GET /api/eve/status`** for LROS. Pins must match your harness; keep **EVE** and **base** `config` in agreement.

The base now completes the handshake (ACK with session + heartbeat capability), sends WALL-E heartbeats, supervises the link, shows EVE's reported state on the LROS EVE page, and forwards STOP (including CYD E-STOP) to EVE.

## Tests

`tests/eve_host/run_tests.sh` builds EVE's control core, safety, routines and battery logic together with WALL-E's peer link on a PC, and runs them over a simulated UART. These are simulated checks only; they do not exercise any hardware.
