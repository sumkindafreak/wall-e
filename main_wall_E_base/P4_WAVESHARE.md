# WALL-E Base Brain — ESP32-P4 Wiring

Primary target: **Waveshare ESP32-P4-Module-DEV-KIT**.

This document is the hardware contract for the P4 Base. The authoritative firmware definitions are in [`main/base_board_pins.h`](main/base_board_pins.h).

## Design rules

- ESP32-P4 owns motors, servos, local sensors, safety, autonomy and telemetry.
- CYD, audio, vision and dock remain ESP-NOW nodes.
- Because ESP32-P4 has no native ESP-NOW radio, a small ESP32-C6/C3/S3 radio gateway carries opaque ESP-NOW frames over UART.
- The P4 radio bridge uses **UART1**.
- GPS uses **UART2**.
- The Waveshare CH343 programming/debug UART remains **UART0 on GPIO37/38**.
- P4 I2C uses the Waveshare board default **SDA GPIO7 / SCL GPIO8**.
- The old Base ST7789 SPI display is disabled on P4. CYD and LROS remain the operator displays. This frees five GPIOs and avoids carrying an S3-only display pin map into the P4 build.
- GPIO53 is reserved for the Waveshare onboard audio amplifier enable and is not assigned to WALL-E external hardware.

## P4 GPIO map

| WALL-E function | ESP32-P4 GPIO | Direction / notes |
|---|---:|---|
| Left motor IN1 | 0 | Output |
| Left motor IN2 | 1 | Output |
| Left motor ENA | 2 | PWM output |
| Flashlight MOSFET | 3 | Output |
| Right motor IN3 | 4 | Output |
| Right motor IN4 | 5 | Output |
| Right motor ENB | 6 | PWM output |
| Shared I2C SDA | 7 | PCA9685 + MPU6050 + compass + VL53L1X |
| Shared I2C SCL | 8 | PCA9685 + MPU6050 + compass + VL53L1X |
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
| GPS TX | 36 | UART2 TX; external GPS must not drive this P4 pin |
| CH343 / debug TX | 37 | **Reserved by board — do not wire WALL-E peripheral here** |
| CH343 / debug RX | 38 | **Reserved by board — do not wire WALL-E peripheral here** |
| Obstacle front-left | 45 | Digital input |
| Obstacle front-right | 46 | Digital input |
| Obstacle rear-left | 47 | Digital input |
| Obstacle rear-right | 48 | Digital input |
| Onboard audio PA enable | 53 | **Reserved by board** |
| GPS RX | 54 | UART2 RX |

The P4 build has a compile-time duplicate-pin check. If two WALL-E functions are accidentally assigned the same GPIO, compilation stops with a GPIO collision error.

## Radio gateway wiring

Default companion firmware: [`../wall_e_radio_gateway_c6/wall_e_radio_gateway_c6.ino`](../wall_e_radio_gateway_c6/wall_e_radio_gateway_c6.ino).

| P4 | ESP32-C6 gateway | Purpose |
|---|---|---|
| GPIO25 TX | GPIO17 RX | P4 → gateway |
| GPIO24 RX | GPIO16 TX | gateway → P4 |
| GND | GND | Common reference |

UART baud: **921600**.

The gateway is transport-only. It does not understand motor, servo, vision, audio or docking commands. It forwards raw radio packets and reports gateway health/status to the P4.

## USB / programming consequence

GPIO24 and GPIO25 are the P4 module's USB full-speed pair. In this WALL-E map they are intentionally repurposed for the radio UART. Therefore:

- use the Waveshare **UART Type-C / CH343** connection for flashing and Serial Monitor;
- do not rely on native USB-FS CDC on GPIO24/25 while the radio gateway is wired;
- GPIO37/38 are left reserved for that CH343 path.

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
- USB CDC on boot: **Default / disabled for this wiring**

The exact CI FQBN is stored in `.github/workflows/walle-p4-base-ci.yml` so the build settings are version-controlled.

## Bring-up order

1. Flash the ESP32-C6 radio gateway and confirm its Serial output says ESP-NOW is ready on channel 11.
2. Connect only P4 ↔ gateway TX/RX/GND and boot the P4 Base.
3. Confirm `[Radio/P4] UART1 gateway ...` followed by fresh gateway status.
4. Power the Base with motor enable power disconnected and verify I2C devices first.
5. Verify battery/current ADC readings against a multimeter before enabling over-current logic.
6. Verify each servo individually at neutral positions.
7. Connect the L298N logic inputs but keep WALL-E raised / tracks clear for first drive test.
8. Verify CYD E-stop before enabling autonomous movement.
9. Add sonar, GPS, obstacle sensors and docking IR one subsystem at a time.

## Safety notes

- ESP32-P4 GPIO is 3.3 V logic. Do **not** feed a 5 V HC-SR04 echo directly into GPIO27; use a divider or level shifter.
- Motor and servo power grounds must share a reference with the P4, but high-current motor/servo power should not be sourced from a P4 GPIO or 3.3 V rail.
- The GPIO map deliberately leaves the board's CH343 UART and audio PA enable untouched.
- Do not merge the P4 migration into `main` until the P4 Base, radio gateway and E-stop path have passed hardware bring-up.
