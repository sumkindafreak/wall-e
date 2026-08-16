# WALL-E Base Brain — ESP32-P4 Wiring

Primary target: **Waveshare ESP32-P4-Module-DEV-KIT**.

The authoritative firmware definitions are in [`main/base_board_pins.h`](main/base_board_pins.h).

## Architecture

- ESP32-P4 owns motors, servos, local sensors, safety, autonomy and telemetry.
- CYD, audio, vision and dock remain ESP-NOW nodes.
- A small ESP32-C6/C3/S3 radio gateway carries opaque ESP-NOW packets between the radio network and the P4 over UART1.
- GPS uses UART2 receive-only.
- The Waveshare CH343 programming/debug path remains UART0 on GPIO37/38.
- The onboard C6 remains available for ESP-Hosted Wi-Fi/LROS. Its reset GPIO54 and SDIO resources are not reused by WALL-E peripherals.
- Shared I2C is SDA GPIO7 / SCL GPIO8.
- The old Base ST7789 SPI display is disabled on P4. CYD and LROS remain the operator displays; a P4 DSI display can be added behind the existing display API later.

## Why the P4 map differs from the old S3 Base

The Waveshare board exposes fewer completely free 3.3 V GPIOs than the old S3 wiring assumed. In particular:

- GPIO37/38 belong to the onboard CH343 UART;
- GPIO53 controls the onboard audio power amplifier;
- GPIO54 is required by the P4↔C6 hosted-Wi-Fi reset path;
- GPIO36 is boot/strapping related;
- GPIO39–48 are deliberately avoided for external 3.3 V WALL-E peripherals because they are in the P4's separately powered I/O domain and several are tied into the board's microSD circuitry.

Instead of forcing old wiring onto those pins, WALL-E uses hardware already well suited to expansion:

- spare **PCA9685 channel 9** drives the flashlight MOSFET;
- a **PCF8574 at I2C address 0x20** receives the four obstacle sensors;
- GPS is **RX-only**, because normal NMEA reception does not require a Base→GPS transmit line.

## Direct P4 GPIO map

| WALL-E function | ESP32-P4 GPIO | Direction / notes |
|---|---:|---|
| Left motor IN1 | 0 | Output |
| Left motor IN2 | 1 | Output |
| Left motor ENA | 2 | PWM output |
| GPS RX | 3 | UART2 RX — connect to GPS TX |
| Right motor IN3 | 4 | Output |
| Right motor IN4 | 5 | Output |
| Right motor ENB | 6 | PWM output |
| Shared I2C SDA | 7 | PCA9685 + MPU6050 + compass + VL53L1X + PCF8574 |
| Shared I2C SCL | 8 | Same bus |
| Battery/5 V rail ADC | 20 | Analog input |
| Current sensor ADC | 21 | Analog input |
| LDR ADC | 22 | Analog input |
| Eye laser MOSFET/PWM | 23 | Output |
| Radio gateway RX | 24 | UART1 RX — from gateway TX |
| Radio gateway TX | 25 | UART1 TX — to gateway RX |
| Sonar trigger | 26 | Output |
| Sonar echo | 27 | Input — level-shift to 3.3 V if sensor echo is 5 V |
| Dock IR TX left | 32 | ~38 kHz output |
| Dock IR TX right | 33 | ~38 kHz output |

### Deliberately unused / reserved

| GPIO / range | Reason |
|---|---|
| 36 | Boot/strapping-related pin; no WALL-E external load |
| 37 / 38 | Waveshare CH343 debug/programming UART |
| 39–48 | Separately powered I/O domain / onboard board-resource region; not used for 3.3 V WALL-E peripherals |
| 53 | Onboard speaker amplifier enable |
| 54 | Onboard C6 ESP-Hosted reset |

The P4 build has compile-time checks for duplicate pins, reserved pins and accidental GPIO39–48 use.

## PCA9685 assignments

The PCA9685 remains at address **0x40**.

| PCA9685 channel | Function |
|---:|---|
| 0 | Head pan |
| 1 | Upper neck / head tilt |
| 2 | Lower neck |
| 3 | Right eye |
| 4 | Left eye |
| 5 | Left arm |
| 6 | Right arm |
| 7 | Left eyebrow |
| 8 | Right eyebrow |
| 9 | Flashlight MOSFET |
| 10–15 | Spare auxiliary outputs |

Channel 9 is driven as full-off/full-on and does not enter the servo interpolation engine.

## PCF8574 obstacle expander

Use a **3.3 V PCF8574** with A0, A1 and A2 tied LOW, giving address **0x20**.

| PCF8574 pin | WALL-E sensor |
|---|---|
| P0 | Front-left obstacle |
| P1 | Front-right obstacle |
| P2 | Rear-left obstacle |
| P3 | Rear-right obstacle |
| P4–P7 | Spare |

The firmware writes `0xFF` to release the quasi-bidirectional PCF8574 pins as inputs before reading them. If the configured expander stops responding, WALL-E treats the obstacle sensors as **blocked** rather than clear.

If your obstacle modules output 5 V logic, level-shift them or use open-collector/open-drain outputs pulled up to the PCF8574's 3.3 V supply.

## Radio gateway wiring

Default companion firmware: [`../wall_e_radio_gateway_c6/wall_e_radio_gateway_c6.ino`](../wall_e_radio_gateway_c6/wall_e_radio_gateway_c6.ino).

| P4 | ESP32-C6 gateway | Purpose |
|---|---|---|
| GPIO25 TX | GPIO17 RX | P4 → gateway |
| GPIO24 RX | GPIO16 TX | gateway → P4 |
| GND | GND | Common reference |

UART baud: **921600**.

The gateway is transport-only. It does not interpret motor, servo, vision, audio or docking commands.

## USB / programming consequence

GPIO24 and GPIO25 are intentionally repurposed from the P4 USB full-speed pair for the radio UART. Therefore:

- flash and monitor WALL-E through the Waveshare **UART Type-C / CH343** connection;
- do not rely on native USB-FS CDC on GPIO24/25 while the radio gateway is connected;
- GPIO37/38 remain reserved for the CH343 path.

## Arduino ESP32 settings

Target configuration used by CI:

- Board: **ESP32-P4 Dev Board**
- Arduino-ESP32: **3.3.11**
- Upload speed: **921600**
- Flash: **16 MB**
- Flash mode: **QIO**
- Flash frequency: **80 MHz**
- Partition: **3 MB app / 9 MB FAT (16 MB)**
- PSRAM: **Enabled**
- Chip variant: **post-v3**
- USB CDC on boot: **Default / not used for WALL-E diagnostics**

The CI FQBN is version-controlled in `.github/workflows/walle-p4-base-ci.yml`.

## Bring-up order

1. Fit the PCF8574 and confirm the I2C scan sees **0x20**, PCA9685 **0x40**, and each fitted I2C sensor.
2. Flash the ESP32-C6 radio gateway and confirm ESP-NOW is ready on channel 11.
3. Connect only P4 ↔ gateway TX/RX/GND and boot the P4 Base.
4. Confirm `[Radio/P4] UART1 gateway ...` followed by fresh gateway status.
5. Power the Base with motor power/enable disconnected and verify I2C devices and ADC readings first.
6. Verify battery/current readings against a multimeter.
7. Verify the flashlight MOSFET from PCA9685 channel 9.
8. Verify each servo individually at neutral positions.
9. Connect the L298N logic inputs with WALL-E raised / tracks clear.
10. Verify CYD E-stop before enabling autonomous movement.
11. Add sonar, GPS, obstacle sensors and docking IR one subsystem at a time.

## Safety notes

- ESP32-P4 external GPIO is not 5 V tolerant. Do **not** feed a 5 V HC-SR04 echo directly into GPIO27.
- Motor and servo grounds must share a reference with the P4, but high-current motor/servo power must not be sourced from a P4 GPIO or 3.3 V rail.
- Keep the PCF8574 and its pull-ups at **3.3 V**.
- Do not merge this migration into `main` until the P4 Base, radio gateway and E-stop path have passed hardware bring-up.
