/*
 * Paludarium 4-Relay Controller - Configuration
 * Target: Standard ESP32 + 4-channel relay board
 *
 * Common board pinouts:
 *   RobotDyn ESP32R4:  Relay1=25, Relay2=26, Relay3=33, Relay4=32
 *   Generic modules:  often 4, 5, 18, 19 or 25, 26, 27, 14
 * Adjust below to match your board.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ────────────────────────────── RELAY PINS (4-channel board) ──────────────────────────────
#define RELAY_COUNT 4
static const int RELAY_PINS[RELAY_COUNT] = { 25, 26, 33, 32 };  // GPIO for Relay 1..4

// Relay logic: most boards use LOW = ON (optocoupler active low)
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

// Display names (change to match your wiring: Rain, Mist, Heater, Fan, etc.)
static const char* RELAY_LABELS[RELAY_COUNT] = { "Relay 1", "Relay 2", "Relay 3", "Relay 4" };

// ────────────────────────────── OPTIONAL SENSORS ──────────────────────────────
// Set to -1 to disable. LDR removed; day/night from NTP only.
#define PIN_DHT    4   // DHT11 data (or -1 to disable)
#define PIN_WATER  35  // Water level high (LOW when high, or -1 to disable)

// ────────────────────────────── FASTLED / 30x4 MATRIX ──────────────────────────────
#define LED_ENABLED           1
#define PIN_LED               27  // GPIO for LED data (avoid 0, 2, 12, 15 on ESP32)
#define MATRIX_WIDTH          30
#define MATRIX_HEIGHT         4
#define LED_COUNT             (MATRIX_WIDTH * MATRIX_HEIGHT)  // 120
#define LED_TYPE              WS2812B
#define LED_COLOR_ORDER       GRB
#define LED_BRIGHTNESS_DEFAULT 120
#define MATRIX_ZIGZAG         0   // 0 = row-major; 1 = alternating rows reversed

// ────────────────────────────── WIFI ──────────────────────────────
#define WIFI_AP_SSID     "Paludarium_Setup"
#define WIFI_AP_PASSWORD "paludarium"
#define WIFI_CONNECT_TIMEOUT_MS  (20 * 500)  // 20 * 500ms
#define WIFI_RETRY_INTERVAL_MS   300000UL    // 5 min when in AP mode

// ────────────────────────────── TIMING ──────────────────────────────
#define SENSOR_READ_INTERVAL_MS  30000
#define WIFI_CHECK_INTERVAL_MS   30000
#define DEBUG_SERIAL             true

#endif // CONFIG_H
