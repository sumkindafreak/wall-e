/**
 * EVE companion — single place for pins, feature flags and tuning.
 * ESP32-S3 N16R8 full PCB: values MUST match your schematic (defaults = DevKitC-1 style UART1).
 *
 * HOW TO READ THIS FILE
 *  - A pin set to -1 means "not wired / not configured". The matching driver stays OFF and never
 *    drives that GPIO, even if its EVE_ENABLE_* flag is 1.
 *  - Values marked UNVERIFIED are starting points from existing code, not measurements of your
 *    build. Check them on the bench before relying on them.
 *  - Every setting is wrapped in #ifndef so PlatformIO build_flags (-DNAME=value) can override it.
 */
#pragma once

#include <Arduino.h>

// =============================================================================
// 1. UART hand link to WALL-E (full duplex, 3.3 V, common GND on pogo)
//    Crossover: WALL-E TX -> EVE RX, WALL-E RX <- EVE TX
// =============================================================================
static const unsigned long EVE_UART_BAUD = 115200;
// UART1: default GPIO17 = TX (from EVE toward WALL-E), GPIO18 = RX (from WALL-E into EVE)
#ifndef EVE_UART_TX_PIN
#define EVE_UART_TX_PIN 17
#endif
#ifndef EVE_UART_RX_PIN
#define EVE_UART_RX_PIN 18
#endif

// Optional 4th pogo: digital presence (EVE input). Set to -1 if not wired.
// Without this pin EVE can only know the UART link is up — NOT that she is physically held.
#ifndef EVE_PRESENT_PIN
#define EVE_PRESENT_PIN (-1)
#endif
/** 1 = presence pin reads LOW when mated (pull-up to GND through the pogo), 0 = reads HIGH. */
#ifndef EVE_PRESENT_ACTIVE_LOW
#define EVE_PRESENT_ACTIVE_LOW 1
#endif

// =============================================================================
// 2. SD card (SPI) — primary asset storage for audio, config, graphics
// =============================================================================
#ifndef EVE_SD_SPI_CS
#define EVE_SD_SPI_CS (-1)
#endif
#ifndef EVE_SD_SPI_MOSI
#define EVE_SD_SPI_MOSI (-1)
#endif
#ifndef EVE_SD_SPI_MISO
#define EVE_SD_SPI_MISO (-1)
#endif
#ifndef EVE_SD_SPI_SCK
#define EVE_SD_SPI_SCK (-1)
#endif

// =============================================================================
// 3. I2S audio output (speaker amplifier). Use port 1 if I2S mic uses port 0.
//    Signal path: SD → WAV decoder → I2S → amplifier → speaker
// =============================================================================
#ifndef EVE_I2S_BCLK_PIN
#define EVE_I2S_BCLK_PIN (-1)
#endif
#ifndef EVE_I2S_LRCK_PIN
#define EVE_I2S_LRCK_PIN (-1)
#endif
#ifndef EVE_I2S_DOUT_PIN
#define EVE_I2S_DOUT_PIN (-1)
#endif
#ifndef EVE_I2S_PORT_INDEX
#define EVE_I2S_PORT_INDEX 1
#endif

// =============================================================================
// 4. Eyes — shared SPI bus; left eye and right eye each have a dedicated chip select.
//    Driver: Arduino_GFX ST7789, EVE_FACE_LCD_HOR_RES x EVE_FACE_LCD_VER_RES per panel.
// =============================================================================
#ifndef EVE_TFT_SPI_MOSI
#define EVE_TFT_SPI_MOSI (-1)
#endif
#ifndef EVE_TFT_SPI_SCK
#define EVE_TFT_SPI_SCK (-1)
#endif
#ifndef EVE_TFT_SPI_MISO
#define EVE_TFT_SPI_MISO (-1)
#endif
#ifndef EVE_LEFT_EYE_CS
#define EVE_LEFT_EYE_CS (-1)
#endif
#ifndef EVE_RIGHT_EYE_CS
#define EVE_RIGHT_EYE_CS (-1)
#endif
#ifndef EVE_TFT_DC
#define EVE_TFT_DC (-1)
#endif
#ifndef EVE_TFT_RST
#define EVE_TFT_RST (-1)
#endif
/** Legacy aliases (same pins). */
#define EVE_TFT_LEFT_CS EVE_LEFT_EYE_CS
#define EVE_TFT_RIGHT_CS EVE_RIGHT_EYE_CS

// =============================================================================
// 5. I2C for dual ToF (VL53L1X or similar)
//    Layout: sensor on EVE's physical LEFT looks left; RIGHT looks right.
//    Optional center: third sensor or fused "forward" — set EVE_TOF_HAS_CENTER 0/1
// =============================================================================
#ifndef EVE_I2C_SDA
#define EVE_I2C_SDA (-1)
#endif
#ifndef EVE_I2C_SCL
#define EVE_I2C_SCL (-1)
#endif
#define EVE_TOF_HAS_CENTER 0
/** GPIO tied to second VL53L1X XSHUT (LOW = sensor off). -1 = single forward sensor only. */
#define EVE_TOF_XSHUT_SECOND (-1)
/** I2C addresses after init (first moved off 0x29, second stays 0x29) */
#define EVE_TOF_ADDR_FIRST 0x30
#define EVE_TOF_ADDR_SECOND 0x29
/** "Noticed" range — beyond = ignore for awareness */
#define EVE_TOF_NEAR_MM 850
#define EVE_TOF_FAR_IGNORE_MM 2400
/** Poll period when ToF enabled */
#define EVE_TOF_POLL_MS 40u
/** 1 = synthetic motion for bench (no VL53L1X library needed) */
#define EVE_TOF_SIMULATE 0

// =============================================================================
// 6. Servos (LEDC PWM) — existing mapping: L = head pan, R = right arm.
//    UNVERIFIED limits/poses: taken from the existing servo_control.cpp and docked web poses.
//    Calibrate on YOUR mechanism (start with the servo horn off) before widening anything.
// =============================================================================
#ifndef EVE_SERVO_L_PIN
#define EVE_SERVO_L_PIN (-1)
#endif
#ifndef EVE_SERVO_R_PIN
#define EVE_SERVO_R_PIN (-1)
#endif
/** Head pan travel (degrees, servo frame). Existing code limited it to 45..135. */
#ifndef EVE_SERVO_HEAD_MIN_DEG
#define EVE_SERVO_HEAD_MIN_DEG 45
#endif
#ifndef EVE_SERVO_HEAD_MAX_DEG
#define EVE_SERVO_HEAD_MAX_DEG 135
#endif
#ifndef EVE_SERVO_HEAD_REST_DEG
#define EVE_SERVO_HEAD_REST_DEG 90
#endif
/** 1 = reverse direction (mirror-mounted servo). */
#ifndef EVE_SERVO_HEAD_INVERT
#define EVE_SERVO_HEAD_INVERT 0
#endif
/** Right arm travel. Existing code allowed 0..180 — narrow this once you know the safe range. */
#ifndef EVE_SERVO_ARM_MIN_DEG
#define EVE_SERVO_ARM_MIN_DEG 0
#endif
#ifndef EVE_SERVO_ARM_MAX_DEG
#define EVE_SERVO_ARM_MAX_DEG 180
#endif
#ifndef EVE_SERVO_ARM_REST_DEG
#define EVE_SERVO_ARM_REST_DEG 90
#endif
#ifndef EVE_SERVO_ARM_INVERT
#define EVE_SERVO_ARM_INVERT 0
#endif
/** Routine poses (docked web UI already uses arm 132 for "hello" and 120 for "attention"). */
#ifndef EVE_SERVO_ARM_GREET_DEG
#define EVE_SERVO_ARM_GREET_DEG 132
#endif
#ifndef EVE_SERVO_ARM_PROTECT_DEG
#define EVE_SERVO_ARM_PROTECT_DEG 120
#endif
/** Scan animation: head glances this many degrees either side of rest. */
#ifndef EVE_SCAN_SWEEP_DEG
#define EVE_SCAN_SWEEP_DEG 20
#endif
/** Pulse width for 0° and 180°. UNVERIFIED: 544/2400 µs are the Arduino Servo library defaults. */
#ifndef EVE_SERVO_MIN_US
#define EVE_SERVO_MIN_US 544
#endif
#ifndef EVE_SERVO_MAX_US
#define EVE_SERVO_MAX_US 2400
#endif
/** Default easing speed for commands that do not give one (degrees per second). */
#ifndef EVE_SERVO_DEFAULT_DPS
#define EVE_SERVO_DEFAULT_DPS 90
#endif
/** Hard ceiling on any requested speed (degrees per second). */
#ifndef EVE_SERVO_MAX_DPS
#define EVE_SERVO_MAX_DPS 180
#endif
/** Servo update period (ms). */
#ifndef EVE_SERVO_TICK_MS
#define EVE_SERVO_TICK_MS 20u
#endif
/**
 * What a STOP does to the servo signal:
 *   0 = HOLD    — keep sending the last commanded pulse (servo keeps holding torque).
 *   1 = RELEASE — stop sending pulses (most analog servos go limp; arms may drop!).
 * HOLD is the default because removing torque is not automatically safe for EVE's arm slides.
 */
#define EVE_SERVO_STOP_HOLD 0
#define EVE_SERVO_STOP_RELEASE 1
#ifndef EVE_SERVO_STOP_POLICY
#define EVE_SERVO_STOP_POLICY EVE_SERVO_STOP_HOLD
#endif

// =============================================================================
// 7. NeoPixels
// =============================================================================
#ifndef EVE_NEOPIXEL_PIN
#define EVE_NEOPIXEL_PIN (-1)
#endif
#ifndef EVE_NEOPIXEL_COUNT
#define EVE_NEOPIXEL_COUNT 8
#endif
/** UNVERIFIED: GRB order at 800 kHz is the common WS2812B default — check your strip. */
#ifndef EVE_NEOPIXEL_TYPE
#define EVE_NEOPIXEL_TYPE (NEO_GRB + NEO_KHZ800)
#endif
/** Brightness cap for the external strip (0-255). */
#ifndef EVE_NEOPIXEL_BRIGHTNESS
#define EVE_NEOPIXEL_BRIGHTNESS 48u
#endif
/** LED refresh period (ms) — limits RMT traffic instead of pushing every loop. */
#ifndef EVE_NEOPIXEL_FRAME_MS
#define EVE_NEOPIXEL_FRAME_MS 20u
#endif
// ESP32-S3 dev boards commonly expose the onboard addressable RGB LED on GPIO48.
// This is independent of the external EVE NeoPixel strip enable below.
#ifndef EVE_ONBOARD_RGB_HEARTBEAT
#define EVE_ONBOARD_RGB_HEARTBEAT 1
#endif
#ifndef EVE_ONBOARD_RGB_PIN
#define EVE_ONBOARD_RGB_PIN 48
#endif
#ifndef EVE_ONBOARD_RGB_BRIGHTNESS
#define EVE_ONBOARD_RGB_BRIGHTNESS 24u
#endif

// =============================================================================
// 8. Battery: INA219 I2C breakout (bus voltage + bidirectional current), OR legacy ADC
//    INA219: wire per breakout (V+ to supply high side, V- toward load, same GND as ESP32).
//    The INA219 measures bus voltage at V-; pack voltage = bus + shunt (done in battery_monitor).
//    Library: PlatformIO lib_deps "adafruit/Adafruit INA219" + "adafruit/Adafruit BusIO".
//    When EVE_BATTERY_INA219 is 1, EVE_BAT_ADC_PIN / EVE_CUR_ADC_PIN are ignored.
//    If you also use ToF on I2C, set EVE_I2C_SDA/SCL to the same pins as below so one bus.
// =============================================================================
#ifndef EVE_BATTERY_INA219
#define EVE_BATTERY_INA219 1
#endif
#ifndef EVE_INA219_I2C_ADDR
#define EVE_INA219_I2C_ADDR 0x40
#endif
#ifndef EVE_INA219_SDA
#define EVE_INA219_SDA 8
#endif
#ifndef EVE_INA219_SCL
#define EVE_INA219_SCL 9
#endif
/**
 * Sign of the INA219 current while the pack is CHARGING:
 *   0 = unknown -> EVE never claims "charging" or "full" from current (default, honest).
 *  -1 = charging current reads negative (charger connects on the V- / load side — the usual case).
 *  +1 = charging current reads positive.
 * Measure once on the bench (plug the charger in, watch "[EVE][BAT]" lines) before setting this.
 */
#ifndef EVE_INA219_CHARGE_SIGN
#define EVE_INA219_CHARGE_SIGN 0
#endif

/**
 * Legacy voltage divider on ADC (only when EVE_BATTERY_INA219 is 0). R1 upper, R2 to GND.
 * CHECK BEFORE USING: the ESP32-S3 ADC reads 0-3.1 V at 12 dB attenuation (Espressif ADC docs).
 * With R1 = R2 = 10k a full 2S pack (8.4 V) puts 4.2 V on the pin — above the ADC range and the
 * 3.3 V pin limit. The firmware marks clipped readings INVALID and warns at boot, but the divider
 * itself must keep full-pack voltage under ~3.1 V (e.g. a ratio of about 1/3) before this path is used.
 */
#define EVE_BAT_ADC_PIN 4
#define EVE_CUR_ADC_PIN 5
#ifndef EVE_ENABLE_BATTERY_MONITOR
#define EVE_ENABLE_BATTERY_MONITOR 1
#endif

#define EVE_BAT_R1 10000
#define EVE_BAT_R2 10000
/** 2S Li-ion / LiPo (~7.4 V nominal on pack). Tune if you use NiMH or different chemistry. */
#ifndef EVE_BAT_MIN_V
#define EVE_BAT_MIN_V 6.0f
#endif
#ifndef EVE_BAT_MAX_V
#define EVE_BAT_MAX_V 8.4f
#endif
#define EVE_BAT_CALIB 1.0f
/**
 * Readings outside this window are treated as INVALID (sensor / wiring / wrong pack setting),
 * never as a real battery level. Defaults bracket the 6.0-8.4 V pack range above.
 */
#ifndef EVE_BAT_PLAUSIBLE_MIN_V
#define EVE_BAT_PLAUSIBLE_MIN_V (EVE_BAT_MIN_V - 1.0f)
#endif
#ifndef EVE_BAT_PLAUSIBLE_MAX_V
#define EVE_BAT_PLAUSIBLE_MAX_V (EVE_BAT_MAX_V + 0.4f)
#endif

/**
 * Legacy hall-effect current sense (only when EVE_BATTERY_INA219 is 0). UNVERIFIED values.
 * 1.65 V zero and 0.122 V/A equal an ACS712-05B (185 mV/A at 5 V, ratiometric) scaled to a 3.3 V
 * supply. The ACS712 datasheet specifies a 4.5-5.5 V supply, so a 3.3 V-powered ACS712 is outside
 * its rating; powered at 5 V its zero is 2.5 V and its output can exceed the 3.1 V ADC range.
 * The INA219 path (the configured one) does not use these values.
 */
#define EVE_CUR_ZERO_V 1.65f
#define EVE_CUR_SENSITIVITY_V_PER_A 0.122f

#define EVE_BAT_SAMPLES 12
#define EVE_CUR_SAMPLES 8
#define EVE_BAT_POLL_MS 2000u

#define EVE_BAT_WARN_PCT 25
#define EVE_BAT_CRIT_PCT 15
/** A status only recovers once the level is this many percent above its threshold. */
#ifndef EVE_BAT_HYST_PCT
#define EVE_BAT_HYST_PCT 4
#endif
/** Current magnitude (A, in the charging direction) that counts as "charging". */
#ifndef EVE_BAT_CHARGE_DETECT_A
#define EVE_BAT_CHARGE_DETECT_A 0.05f
#endif
/** "Full" = charging, within 0.05 V of EVE_BAT_MAX_V and charge current tapered below this (A). */
#ifndef EVE_BAT_FULL_TAPER_A
#define EVE_BAT_FULL_TAPER_A 0.10f
#endif
#define EVE_BAT_LOW_REPORT_MIN_MS 20000u
/** Minimum spacing of routine "[EVE][BAT]" log lines (status changes always log). */
#ifndef EVE_BAT_LOG_MS
#define EVE_BAT_LOG_MS 30000u
#endif

// =============================================================================
// 9. EVE face (LVGL 9) — one panel per physical eye (same resolution each).
//    When TFT pins are unset (<0), LVGL runs with stub flush (CI / bring-up).
// =============================================================================
#define EVE_FACE_LCD_HOR_RES 240
#define EVE_FACE_LCD_VER_RES 280
/** Partial buffer height (lines); larger = smoother, more RAM */
#define EVE_FACE_LVGL_BUF_LINES 40
/** Serial bench: type 0–9 / n p s h c a l r u d w / ? for help */
#define EVE_FACE_DEBUG_BENCH 1
#define EVE_FACE_TFT_HAS_PIN(p) ((p) >= 0)
/** Both eye CS pins wired → two independent physical panels (not mirrored). */
#define EVE_FACE_DUAL_PHYSICAL                                                                 \
  (EVE_FACE_TFT_HAS_PIN(EVE_LEFT_EYE_CS) && EVE_FACE_TFT_HAS_PIN(EVE_RIGHT_EYE_CS))
#define EVE_FACE_GFX_READY                                                                 \
  (EVE_FACE_TFT_HAS_PIN(EVE_TFT_SPI_MOSI) && EVE_FACE_TFT_HAS_PIN(EVE_TFT_SPI_SCK) &&      \
   EVE_FACE_TFT_HAS_PIN(EVE_LEFT_EYE_CS) && EVE_FACE_TFT_HAS_PIN(EVE_TFT_DC))

// =============================================================================
// 10. Feature enables — turn on only after hardware matches pins above.
//     A driver whose pins are still -1 stays off even when its flag is 1.
// =============================================================================
#ifndef EVE_ENABLE_EYES
#define EVE_ENABLE_EYES 0
#endif
#ifndef EVE_ENABLE_TOF
#define EVE_ENABLE_TOF 0
#endif
#ifndef EVE_ENABLE_SERVOS
#define EVE_ENABLE_SERVOS 0
#endif
#ifndef EVE_ENABLE_NEOPIXEL
#define EVE_ENABLE_NEOPIXEL 0
#endif
#ifndef EVE_ENABLE_AUDIO
#define EVE_ENABLE_AUDIO 0
#endif

// Docked WebUI: EVE hosts a local AP only while on her dock.
#define EVE_ENABLE_DOCKED_WEBUI 1
#define EVE_WEBUI_AP_SSID "EVE-Desk"
#define EVE_WEBUI_AP_PASS "evefriend"
#define EVE_WEBUI_AP_CHANNEL 6
#define EVE_WEBUI_AP_MAX_CLIENTS 2
// Small grace avoids AP flapping on brief UART jitter; it still shuts down after undock/link loss.
#define EVE_DOCKED_WEBUI_OFF_GRACE_MS 3000u
// Bench-only dock mimic. Keep off for real use. Serial Monitor also supports:
//   docktest on
//   docktest off
#define EVE_ENABLE_SERIAL_CONSOLE 1
// Keep normal Serial Monitor output quiet. Set to 1 only when debugging UART frames.
#ifndef EVE_SERIAL_VERBOSE_LOGS
#define EVE_SERIAL_VERBOSE_LOGS 0
#endif
#define EVE_ENABLE_DOCK_MIMIC_TEST 1
#define EVE_DOCK_MIMIC_ON_BOOT 0
#define EVE_DOCK_MIMIC_FAKE_CHARGING 1

// =============================================================================
// 11. Sound cues used by routines (track numbers resolved by the SD card's /config/audio.json).
//     The SD template maps 1 hello, 2 curious, 3 ack, 4 soft, 5 alert, 6 error.
//     No recordings are supplied by this firmware. A missing WAV is logged and skipped; set a cue
//     to 0 to keep that routine silent.
// =============================================================================
#ifndef EVE_SND_WAKE
#define EVE_SND_WAKE 1
#endif
#ifndef EVE_SND_GREET
#define EVE_SND_GREET 1
#endif
#ifndef EVE_SND_LOST
#define EVE_SND_LOST 2
#endif
#ifndef EVE_SND_SCAN
#define EVE_SND_SCAN 2
#endif
#ifndef EVE_SND_PLAY
#define EVE_SND_PLAY 2
#endif
#ifndef EVE_SND_STANDBY
#define EVE_SND_STANDBY 4
#endif
#ifndef EVE_SND_LOW_POWER
#define EVE_SND_LOW_POWER 4
#endif
#ifndef EVE_SND_PROTECT
#define EVE_SND_PROTECT 5
#endif

// =============================================================================
// 12. Protocol timing
// =============================================================================
static const uint32_t EVE_HELLO_RETRY_MS = 500;
static const uint32_t EVE_HEARTBEAT_MS = 250;
/**
 * Link supervision for peers that do NOT announce heartbeats in their ACK (older WALL-E firmware,
 * the EVE dock): no valid frame for this long in ESCORT / INTERACT / DOCKED -> link lost.
 * Peers that announce heartbeats (this WALL-E firmware) are supervised in EVERY linked state
 * with EVE_LINK_TIMEOUT_MS from eve_link_protocol.h.
 */
static const uint32_t EVE_LINK_LOST_MS = 8000;
/** Linked and no command for this long -> standby (SLEEP). 0 = never (default). */
#ifndef EVE_IDLE_STANDBY_MS
#define EVE_IDLE_STANDBY_MS 0u
#endif
/** Rate limit for repeated "[EVE][CMD] rejected" log lines. */
#ifndef EVE_LOG_RATE_MS
#define EVE_LOG_RATE_MS 1000u
#endif
