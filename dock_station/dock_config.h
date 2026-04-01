/*******************************************************************************
 * dock_config.h
 * Pin definitions and compile-time configuration for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_CONFIG_H
#define DOCK_CONFIG_H

/*=============================================================================
 * PIN DEFINITIONS (ESP32-S3 DevKit-style board)
 * NOTE: These GPIO numbers follow the original ESP32-WROOM-32 layout.
 *       Verify against your specific ESP32-S3 N16R8 carrier board and
 *       adjust if any peripherals are wired to different pins.
 *
 * PIN SUMMARY (one function per GPIO — no clashes):
 *   GPIO  1  ACS712 current sense (ADC)
 *   GPIO  2  NeoPixel data (WS2812B)
 *   GPIO  4  Charge MOSFET gate
 *   GPIO  5  Alignment right (IR input)
 *   GPIO  6  PIR / motion sensor (charging bay presence — OR’d into dock detect)
 *   GPIO  8  TFT CS (chip select)
 *   GPIO  9   (free)
 *   GPIO 10  Alignment left (IR input)
 *   GPIO 13  Obstacle front right
 *   GPIO 14  Obstacle front left
 *   GPIO 15  Call WALL-E switch
 *   GPIO 16  Obstacle back left
 *   GPIO 17  Obstacle back right
 *   GPIO 18  Arrow right MOSFET
 *   GPIO 19  Arrow left MOSFET
 *   GPIO 21  Internal LED MOSFET
 *
 *   GPIO  3  TFT DC    GPIO  7  TFT RST   GPIO 11  TFT MOSI (SPI)
 *   GPIO 12  TFT SCK   GPIO 20  TFT BL (LED)
 *   GPIO  9  VL6180 SDA (I2C)  GPIO 47  VL6180 SCL — if USE_VL6180_TOF
 * FREE: adjust 9/47 if you need those pins elsewhere.
 *===========================================================================*/

#define PIN_MOSFET_GATE      4   /* Charge enable MOSFET gate (GPIO4: output-capable on ESP32-S3) */
#define PIN_ACS712_ADC       1   /* ACS712 analog out (GPIO1: ADC-capable on ESP32-S3) */
/** Digital motion sensor (PIR etc.) on GPIO6 — “presence” for dock/charging state machine (OR with ToF/sonar/obstacles). */
#define PIN_MOTION_SENSOR    6
#define PIN_IR_BEAM          PIN_MOTION_SENSOR   /* legacy name */
/* 1 = motion when pin reads HIGH (typ. HC-SR501); 0 = motion when LOW */
#define MOTION_ACTIVE_HIGH   1
#define MOTION_DEBOUNCE_MS   60
/* Obstacle sensors — positions when looking at the dock box from the front */
#define PIN_OBSTACLE_1      14   /* Front left  (GPIO14) */
#define PIN_OBSTACLE_2      13   /* Front right (GPIO13) */
#define PIN_OBSTACLE_3      16   /* Back left   (GPIO16) */
#define PIN_OBSTACLE_4      17   /* Back right  (GPIO17) */
#define PIN_STATUS_NEOPIXEL  2
#define NEOPIXEL_STRIP_COUNT  10   /* Status strip (indices 0..9) */
#define NEOPIXEL_FAULT_LED_INDEX  10  /* Single NeoPixel on same data line = status LED (11th LED) */
#define NEOPIXEL_COUNT  (NEOPIXEL_STRIP_COUNT + 1)   /* 11 total: strip + status LED */

/* Status LED (10): green = all good (WALL-E docked, mouth clear), flash orange = sensor triggered (waiting/mouth blocked), red = fault. */
#define NEOPIXEL_STATUS_LED_LINK_FAULT  1   /* 1 = solid red when dock in FAULT */
#define NEOPIXEL_STATUS_LED_LINK_BEAM   1   /* 1 = green when docked + clear; else contributes to flash orange */
#define NEOPIXEL_STATUS_LED_LINK_MOUTH  1   /* 1 = mouth blocked → flash orange; clear required for green */
#define NEOPIXEL_STATUS_LED_ORANGE_FLASH_MS  500   /* Orange flash half-period (on/off) when sensor triggered */

/* Sensor / charging indicator layout (pixels linked to each sensor + loading bar) */
#define NEOPIXEL_BEAM_FIRST    0   /* first pixel of beam pair (0-1) */
#define NEOPIXEL_BEAM_COUNT    2   /* beam: green = present, red = not present */
#define NEOPIXEL_MOUTH_FIRST   2   /* first pixel of mouth pair (2-3) */
#define NEOPIXEL_MOUTH_COUNT   2   /* mouth/obstacles: green = clear, red = blocked */
#define NEOPIXEL_CHARGE_FIRST  4   /* first pixel of charging/load bar (4-9) */
#define NEOPIXEL_CHARGE_COUNT  6   /* bar fills as loading/charging progresses; green when charged */

/* Call WALL-E push button (INPUT_PULLUP: LOW = pressed). Press to start callout; stops when WALL-E docks or press again. */
#define PIN_CALL_SWITCH   15   /* avoid GPIO 0 (boot) */

/* Dock alignment — dual TSOP/VS1838B receivers (see dock_ir_guidance).
 *   WALL-E base firmware drives two modulated IR TX toward these (dock_ir_transmitters.h).
 *   Receiver output: LOW = IR seen, HIGH = idle. Not mixed with break-beam (PIN_IR_BEAM).
 */
#define PIN_ALIGN_LEFT    10   /* Left side of bay: IR receiver digital OUT */
#define PIN_ALIGN_RIGHT    5   /* Right side of bay: IR receiver digital OUT */

/* Arrow indicator MOSFET outputs (guide WALL-E left/right) */
#define PIN_ARROW_LEFT    19   /* 16 not broken out on this module */
#define PIN_ARROW_RIGHT   18   /* 17 not broken out on this module */

/* Internal LED (MOSFET) — e.g. dock interior lighting */
#define PIN_INTERNAL_LED  21

/* MOSFET module logic: 1 = active LOW (LOW = load ON). Try 0 if still no output. */
#define MOSFET_ARROW_ACTIVE_LOW    1
#define MOSFET_INTERNAL_ACTIVE_LOW 1

/* 1.8" TFT SPI 128x160 (ST7735) — pins match typical label: VCC GND LED SCK SDA A0 RESET CS */
#define PIN_TFT_MOSI    11   /* SDA (SPI data to TFT) */
#define PIN_TFT_SCK     12   /* SCK (SPI clock) */
#define PIN_TFT_DC       3   /* A0 (Data/Command) */
#define PIN_TFT_RST      7   /* RESET */
#define PIN_TFT_BL      20   /* LED (backlight) */
#define PIN_TFT_CS       8   /* CS (chip select) */
/* Adafruit_ST7735: 0=0°, 1=90°, 2=180°, 3=270° (128x160 ST7735 “green tab”) */
#define TFT_ROTATION       2

/*=============================================================================
 * FEATURE FLAGS
 *===========================================================================*/

/* 0 = disable WiFi/ESP-NOW/OTA (use if dock bootloops on ESP32-S3; try 1 when radio is stable) */
#define ENABLE_WIFI  0

/* With ENABLE_WIFI: 1 = arrows only after WALL-E sends DOCK_CMD_DOCKING_ARM. 0 = NOT_DOCKED only (no ESP-NOW arm). If ENABLE_WIFI is 0, arm is ignored (local testing). */
#define DOCK_ARROWS_REQUIRE_WALLE_ARM  1

#define USE_OBSTACLE_SENSORS  1   /* 1 = use obstacle sensors (mouth blocked) */

/* VL6180X time-of-flight (TOF050C etc.) — I2C dock presence, ~20–100 mm.
 * Mount facing the charge slot; when Wall-E is in position, range drops into band.
 * If begin() fails, dock falls back to USE_SONAR or mouth sensors. */
#define USE_VL6180_TOF        1
#define PIN_VL6180_SDA        9
#define PIN_VL6180_SCL        47
/** Inclusive mm band for “robot present” (tune after mounting). */
#define VL6180_DOCK_MIN_MM    12
#define VL6180_DOCK_MAX_MM    95

/* HC-SR04 ultrasonic — set 1 only if wired; otherwise VL6180 / mouth logic applies. */
#ifndef USE_SONAR
#define USE_SONAR             0
#endif

/* Test mode: exercise sensors, outputs, TFT, NeoPixel for bench testing.
 * DOCK_TEST_MODE 1 = run one full test sequence at end of setup(), then normal loop.
 * DOCK_TEST_SERIAL_CMD 1 = in loop, typing "TEST" in Serial Monitor runs one test.
 */
#define DOCK_TEST_MODE        0   /* 1 = run test at boot */
#define DOCK_TEST_SERIAL_CMD  1   /* 1 = accept "TEST" over Serial to run test anytime */

/* Obstacle pins use INPUT_PULLUP: unconnected = HIGH = clear. 0 = blocked when pin LOW, 1 = blocked when pin HIGH */
#define OBSTACLE_ACTIVE_HIGH  0

/*=============================================================================
 * ACS712
 *===========================================================================*/

#define ACS712_MV_PER_AMP       100   /* 100 = 20A module, 185 = 5A, 66 = 30A */
#define ACS712_MOVING_AVG_SAMPLES  20
#define ACS712_CALIB_SAMPLES      100
/* Match multimeter: if multimeter reads 0.37A but dock shows 0.20A, set scale = 0.37f/0.20f (1.85f). */
#define CURRENT_CALIB_SCALE    1.0f
#define CURRENT_CALIB_OFFSET   0.37f

/* On ESP32-S3 bring-up, set to 1 to run with ONLY the ACS712 connected
 * (beam and obstacles forced off). Set to 0 when IR beam (and obstacles) are wired.
 */
#define S3_CURRENT_ONLY_MODE      0

/*=============================================================================
 * CURRENT THRESHOLDS (Amps)
 *===========================================================================*/

#define CURRENT_CHARGING_A    0.20f   /* Current threshold to auto-enter CHARGING state (gate turns on first, then state follows when current seen) */
#define CURRENT_CHARGED_A     0.06f
#define CURRENT_OVERCURRENT_A 3.0f
#define CHARGED_STABLE_MS     90000  /* 90 seconds */
/* 1 = disable overcurrent fault (no FAULT OVC). Use until ACS712 is calibrated. */
#define OVERCURRENT_FAULT_DISABLED  1

/* Enter idle mode after being CHARGED for this long (dim display/LEDs, show Idle). 0 = disabled. */
#define IDLE_AFTER_CHARGED_MS  300000  /* 5 minutes */

/*=============================================================================
 * HOME WIFI — Dock connects as permanent base (WALL-E's home = dock location)
 * Set your network SSID and password. Leave SSID empty ("") to skip STA.
 *===========================================================================*/

#define WIFI_HOME_SSID     "YourNetwork"
#define WIFI_HOME_PASSWORD "YourPassword"

/*=============================================================================
 * DOCK ID (for command filtering; 0 = accept all)
 *===========================================================================*/

#define DOCK_ID  0x00000001

/* Timezone offset (seconds from UTC). e.g. -18000 = EST, 0 = GMT */
#define TIMEZONE_OFFSET_SEC  0

/*=============================================================================
 * TIMING
 *===========================================================================*/

/* After power-on: stay in BOOT this long (ms) while sensors settle, then pick DOCKED_IDLE vs NOT_DOCKED. */
#define BOOT_SETTLE_MS        600

/* In NOT_DOCKED with no dock presence this long → STANDBY (arrows off, quiet bay). 0 = disable. */
#define BAY_IDLE_AFTER_MS       10000

#define DOCK_DEBOUNCE_MS      1500   /* Wait before enabling charge after dock */
#define BEAM_BROKEN_DEBOUNCE_MS  400   /* Beam must be broken this long before NOT_DOCKED -> DOCKED_IDLE */
#define BEAM_CLEAR_DEBOUNCE_MS   2500  /* ToF/PIR jitter: must read “no robot in slot” this long before docked -> NOT_DOCKED */
#define CHARGING_TO_IDLE_DEBOUNCE_MS  800   /* Blocked must be true this long before CHARGING -> DOCKED_IDLE */
#define IDLE_TO_CHARGING_DEBOUNCE_MS  800   /* Current must be above threshold this long before DOCKED_IDLE -> CHARGING */
#define CHARGING_STUCK_MS             10000 /* No current for this long in CHARGING + mouth clear -> back to DOCKED_IDLE (sensor stuck recovery) */
#define APPROACH_STAGE_TIMEOUT_MS  2000  /* Revert to sensor fallback if no WALL-E update */
#define ESPNOW_BEACON_INTERVAL_MS  100   /* 10 Hz */
#define DEBUG_STATS_INTERVAL_MS    3000

/* 1 = pixels 8–9 on status strip show live L/R IR receiver detect (NOT_DOCKED only). */
#define DOCK_IR_NEOPIXEL_DEBUG  0

/* TSOP lost timer: pause arrow guidance after this many ms with no IR on either receiver. */
#define DOCK_IR_LOST_TIMEOUT_MS  2000

/*=============================================================================
 * NEOPIXEL
 *===========================================================================*/

#define NEOPIXEL_BRIGHTNESS_DEFAULT  60
#define NEOPIXEL_BREATHE_PERIOD_MS   3000
#define NEOPIXEL_CHASE_PERIOD_MS     800
#define NEOPIXEL_FAULT_BLINK_MS      150
#define NEOPIXEL_CHARGED_PULSE_MS    3000

#endif /* DOCK_CONFIG_H */
