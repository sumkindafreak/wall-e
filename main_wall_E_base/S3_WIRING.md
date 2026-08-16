# WALL-E Base Brain — ESP32-S3 Wiring

Production target: **ESP32-S3 Dev Module / ESP32-S3-DevKitC-1 class board**.

The authoritative firmware definitions live in `main/base_board_pins.h`.

## Architecture

- The ESP32-S3 is the WALL-E Base Brain.
- Wi-Fi and ESP-NOW run directly on the S3.
- CYD, audio, vision and dock remain ESP-NOW nodes.
- No ESP32-P4 is required.
- No ESP32-C6 radio gateway is required.
- The old local ST7789 Base display is not used; CYD/LROS remain the operator interfaces.

## GPIO map

| WALL-E function | ESP32-S3 GPIO | Notes |
|---|---:|---|
| Left motor IN1 | 4 | L298N |
| Left motor IN2 | 5 | L298N |
| Left motor ENA | 6 | PWM |
| Right motor IN3 | 7 | L298N |
| Right motor IN4 | 8 | L298N |
| Right motor ENB | 9 | PWM |
| Battery monitor ADC | 1 | ADC1 |
| Current monitor ADC | 2 | ADC1 |
| LDR ADC | 10 | ADC1 |
| Flashlight MOSFET | 11 | Digital output |
| Eye laser | 12 | PWM output |
| Sonar trigger | 13 | Output |
| Sonar echo | 14 | Input — level shift if sensor echo is 5 V |
| Dock IR left | 15 | ~38 kHz PWM |
| Dock IR right | 16 | ~38 kHz PWM |
| I2C SDA | 17 | PCA9685 + MPU6050 + compass + VL53L1X |
| I2C SCL | 18 | Shared I2C bus |
| GPS RX | 21 | Connect to GPS TX; GPS is receive-only |
| Obstacle front-left | 38 | Input |
| Obstacle front-right | 39 | Input |
| Obstacle rear-left | 40 | Input |
| Obstacle rear-right | 41 | Input |

## Pins deliberately left alone

- GPIO0, GPIO3, GPIO45 and GPIO46: boot/strapping pins.
- GPIO19 and GPIO20: kept free for native USB / USB-JTAG.
- GPIO26 through GPIO37: not used because these may be occupied by flash/PSRAM depending on the S3 module.
- GPIO43 and GPIO44: kept free for UART0 / serial diagnostics.
- GPIO47 and GPIO48: left spare.

## PCA9685 servo channels

| Channel | Servo |
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
| 9–15 | Spare |

## First bench test order

1. Flash the S3 with motor power disconnected.
2. Confirm Serial shows `[WALL-E/S3] Starting Base Brain...` and native ESP-NOW starts.
3. Verify the I2C bus and PCA9685 before connecting servo power.
4. Verify battery/current ADC readings against a multimeter.
5. Verify the CYD link and E-stop.
6. Test each servo individually.
7. Connect the L298N logic lines with WALL-E lifted so the tracks cannot drive him off the bench.
8. Add sonar, GPS, obstacle sensors and dock IR one subsystem at a time.

## Safety

ESP32-S3 GPIO is 3.3 V logic. Do not connect a 5 V ultrasonic echo or other 5 V logic output directly to an S3 GPIO. Use a divider or level shifter where needed.
