# EVE companion firmware (ESP32-S3)

UART-framed link to WALL-E (hand pogo), state machine, and hooks for eyes / ToF / servos / NeoPixel / DFPlayer.

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

Edit `include/config.h`:

- `EVE_UART_TX_PIN` / `EVE_UART_RX_PIN` — hand link (default 17/18; **must match your PCB**).
- Feature flags: `EVE_ENABLE_NEOPIXEL`, `EVE_ENABLE_SERVOS`, etc. (keep off until wired).
- DFPlayer UART pins must **not** overlap the hand UART.

## WALL-E side

Peer UART on the base must use the **crossover** convention: WALL-E TX → EVE RX, WALL-E RX ← EVE TX, **GND** common.

The base (`main_wall_E_base`) can optionally run **`eve_uart_bridge`** on **UART2** (default **TX GPIO 18**, **RX GPIO 17** — see `main/eve_uart_bridge.h`) and expose link status as **`GET /api/eve/status`** for LROS. Pins must match your harness; keep **EVE** and **base** `config` in agreement.
