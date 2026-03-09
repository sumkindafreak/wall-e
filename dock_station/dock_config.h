/*******************************************************************************
 * dock_config.h
 * Pin definitions and compile-time configuration for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_CONFIG_H
#define DOCK_CONFIG_H

/*=============================================================================
 * PIN DEFINITIONS (ESP32-WROOM-32 30-pin DevKit)
 *===========================================================================*/

#define PIN_MOSFET_GATE   25   /* Charge enable MOSFET gate */
#define PIN_ACS712_ADC    34   /* ACS712 analog out (ADC1, input only) */
#define PIN_IR_BEAM       27   /* Straight IR beam sensor (beam broken = present) */
#define PIN_OBSTACLE_1    14
#define PIN_OBSTACLE_2    13
#define PIN_OBSTACLE_3    33
#define PIN_OBSTACLE_4    32
#define PIN_NEOPIXEL      2
#define NEOPIXEL_COUNT    8

/*=============================================================================
 * FEATURE FLAGS
 *===========================================================================*/

#define USE_OBSTACLE_SENSORS  1   /* 1 = use obstacle sensors, 0 = ignore */

/* Obstacle active level: 1 = HIGH when blocked, 0 = LOW when blocked */
#define OBSTACLE_ACTIVE_HIGH  1

/*=============================================================================
 * ACS712
 *===========================================================================*/

#define ACS712_MV_PER_AMP       100   /* 100 = 20A module, 185 = 5A, 66 = 30A */
#define ACS712_MOVING_AVG_SAMPLES  20
#define ACS712_CALIB_SAMPLES      100

/*=============================================================================
 * CURRENT THRESHOLDS (Amps)
 *===========================================================================*/

#define CURRENT_CHARGING_A    0.20f
#define CURRENT_CHARGED_A     0.06f
#define CURRENT_OVERCURRENT_A 3.0f
#define CHARGED_STABLE_MS     90000  /* 90 seconds */

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

#define DOCK_DEBOUNCE_MS      1500   /* Wait before enabling charge after dock */
#define ESPNOW_BEACON_INTERVAL_MS  100   /* 10 Hz */
#define DEBUG_STATS_INTERVAL_MS    3000

/*=============================================================================
 * NEOPIXEL
 *===========================================================================*/

#define NEOPIXEL_BRIGHTNESS_DEFAULT  60
#define NEOPIXEL_BREATHE_PERIOD_MS   3000
#define NEOPIXEL_CHASE_PERIOD_MS     800
#define NEOPIXEL_FAULT_BLINK_MS      150
#define NEOPIXEL_CHARGED_PULSE_MS    3000

#endif /* DOCK_CONFIG_H */
