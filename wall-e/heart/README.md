# WALL-E Audio Core (ESP32-S3)

Uses the **full IAN audio engine** from showduino — same implementation as ian_v1.0.
SD playback + PCM5102 + RTC + 3 buttons.

**Board:** ESP32-S3 Dev Module  
**Library:** ESP32-audioI2S (schreibfaul1)

## Files

- `wall-e-heart.ino` — main sketch
- `audio_engine.cpp` / `audio_engine.h` — IAN audio engine
- `ian_pins.h` / `ian_config.h` — pin and config (IAN layout)

## Required Libraries

1. **ESP32-audioI2S** (schreibfaul1)
2. **RTClib** (Adafruit)
3. **Adafruit BusIO**

## Pinout (IAN v1.0 layout)

| Function  | GPIO | Notes                    |
|----------|------|--------------------------|
| SD CS    | 9    | SPI chip select          |
| SD SCK   | 12   | SPI clock                |
| SD MOSI  | 10   | SPI data out             |
| SD MISO  | 11   | SPI data in              |
| I2S BCLK | 8    | PCM5102 bit clock        |
| I2S LRC  | 6    | PCM5102 LRCLK / word sel |
| I2S DOUT | 7    | PCM5102 data in          |
| Button Record  | 4  | Record (future)          |
| Button Stop    | 13 | Stop playback            |
| Button Preview | 14 | Play last file           |
| RTC SDA  | 21   | DS3231 I2C (optional)    |
| RTC SCL  | 20   | DS3231 I2C (optional)    |

Buttons use internal pull‑up; connect to GND when pressed.

## SD Card

- Format: FAT32
- Create folder `/music/` and add MP3 files
- Default test file: `/music/test.mp3`
- Log file: `/log.txt`

## Wiring Notes

- Buttons 4, 5, 6 avoid UART pins 1/2 on ESP32-S3
- RTC: DS3231 on I2C (SDA 8, SCL 9)
- PCM5102: I2S, 3.3V logic
