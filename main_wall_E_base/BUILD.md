# WALL-E Base Brain

**Flash this project** for the main robot controller (base brain).  
The WebUI shows **Base v1.2** in the top bar when you’re on the correct page.

## Build & upload
```bash
cd main_wall_E_base
pio run -t upload
```

For OTA, set `upload_port` in `platformio.ini` to your device IP (e.g. `192.168.4.1`), then:
```bash
pio run -t upload
```

## WebUI
Connect to WALL-E AP → `http://192.168.4.1`  
If Settings look wrong, hard-refresh (Ctrl+F5) or add `?v=1` to the URL to bypass cache.
