# WALL-E Audio Brain Setup

## DFPlayer Mini Wiring

| DFPlayer Mini | ESP32-S3 |
|---------------|----------|
| VCC           | 3.3V or 5V |
| GND           | GND      |
| RX            | GPIO 17 (ESP TX) |
| TX            | GPIO 16 (ESP RX) |

**Note:** DFPlayer RX ← ESP TX, DFPlayer TX → ESP RX. Baud rate: 9600.

## SD Card

1. Format microSD as **FAT32**
2. Create an `mp3` folder (or use root)
3. Name files: `0001.mp3`, `0002.mp3`, ... (or `001.mp3`, `002.mp3`)
4. Copy order = track number

| Track | File      | Event   |
|-------|-----------|---------|
| 1     | 0001.mp3  | Boot    |
| 2     | 0002.mp3  | Idle 1  |
| 3     | 0003.mp3  | Idle 2  |
| 4     | 0004.mp3  | Curious |
| 5     | 0005.mp3  | E-stop  |

## I2S Mic (INMP441) Wiring

| INMP441 | ESP32-S3 |
|---------|----------|
| VDD     | 3.3V     |
| GND     | GND      |
| SCK     | GPIO 5   |
| WS      | GPIO 25  |
| SD      | GPIO 26  |
| L/R     | GND      |

Use `micReadLevel()` to get 0–32767 peak level for voice/sound detection.

## Library

Install **DFRobotDFPlayerMini** via Arduino Library Manager.
