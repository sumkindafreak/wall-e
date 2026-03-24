# Paludarium 4-Relay Controller

Firmware for **standard ESP32** + **4-channel relay board**. Web UI for manual control, optional DHT11/LDR/water sensor, WiFi with AP fallback.

## Hardware

- **ESP32** (any dev board with ESP32-WROOM)
- **4-channel relay module** (typical pinout: Relay1=GPIO25, Relay2=GPIO26, Relay3=GPIO33, Relay4=GPIO32)

Edit `config.h` to match your board:

- `RELAY_PINS` – GPIOs for each relay
- `RELAY_LABELS` – names shown in the UI (e.g. "Rain", "Mist", "Heater", "Fan")
- `PIN_DHT`, `PIN_LDR`, `PIN_WATER` – set to `-1` to disable that sensor

## Build

**PlatformIO:** from this folder run `pio run` (and `pio run -t upload` to flash).

**Arduino IDE:** open `paludarium_4relay.ino`, install libraries (ArduinoJson, DHT sensor library, Adafruit Unified Sensor), select board "ESP32 Dev Module", then compile/upload.

## WiFi

1. If the ESP32 has no saved WiFi, it starts an AP: **SSID** `Paludarium_Setup`, **password** `paludarium`.
2. Connect a phone/PC to that AP, open **http://192.168.4.1**.
3. Use **Wi-Fi** section: Scan → select your network → enter password → Save & Restart.
4. Reconnect your device to your normal WiFi and find the ESP32’s IP (router DHCP list or serial monitor). Open **http://&lt;IP&gt;** to use the dashboard.

## Web UI

- **Mode:** Auto / Manual (manual = relay toggles from web only).
- **Relays:** Four buttons to turn each relay ON/OFF.
- **Sensors:** Temp, humidity, light %, water level (if pins configured).
- **Wi-Fi:** Scan, select network, password, save & restart.

Relay logic in `config.h`: `RELAY_ON` / `RELAY_OFF` (usually LOW = ON for optocoupler boards).
