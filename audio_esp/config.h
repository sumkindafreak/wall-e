/**
 * config.h — Feature flags, timing, track IDs, thresholds
 */
#ifndef AUDIO_ESP_CONFIG_H
#define AUDIO_ESP_CONFIG_H

/*=============================================================================
 * SD card + I2S WAV audio (Base link is ESP-NOW)
 *===========================================================================*/
#define I2S_AUDIO_PORT_INDEX 1
#define VOICE_UART_BAUD     9600

/*=============================================================================
 * TIMING
 *===========================================================================*/
#define HEARTBEAT_INTERVAL_MS    500
#define MIC_READ_INTERVAL_MS     20
#define IR_DEBOUNCE_MS           50
#define VOICE_COOLDOWN_MS        1500
#define DIAGNOSTICS_INTERVAL_MS  2000

/*=============================================================================
 * MIC
 *===========================================================================*/
#define MIC_SMOOTH_SAMPLES   8
#define MIC_DIR_THRESHOLD    1.3f   /* L/R ratio above this = LEFT or RIGHT */
#define MIC_NOISE_FLOOR      50     /* Below this = UNKNOWN */
#define MIC_SURGE_THRESHOLD  200    /* Sudden noise event */

/*=============================================================================
 * IR DOCK
 *===========================================================================*/
#define IR_ACTIVE_LOW       1   /* 1 = LOW when IR detected; 0 = HIGH when detected */

/*=============================================================================
 * AUDIO — WAV track IDs map to /audio/NNN.wav on SD card
 *===========================================================================*/
#define TRACK_STARTUP       1
#define TRACK_HELLO         2
#define TRACK_ACK           3
#define TRACK_ERROR         4
#define TRACK_CURIOUS       5
#define TRACK_SLEEP         6
#define TRACK_WAKE          7
#define TRACK_DOCK_GUIDE    8
#define TRACK_STOP          9
/* Character WAV cues (010.wav–016.wav on SD under /audio/) */
#define TRACK_CHAR_PLAY         10
#define TRACK_CHAR_STOP         11
#define TRACK_CHAR_REWIND       12
#define TRACK_CHAR_RECORD_FAIL  13
#define TRACK_MENU_ENTER_OK     14
#define TRACK_MENU_EXIT_OK      15
#define TRACK_MENU_TICK         16  /* optional tick during long hold */

/*=============================================================================
 * AUDIO PRIORITY (higher = can interrupt lower)
 *===========================================================================*/
#define PRIO_LOWEST     0
#define PRIO_NORMAL     10
#define PRIO_ACK        20
#define PRIO_VOICE_CMD  30
#define PRIO_ERROR      40
#define PRIO_ESTOP      50
#define PRIO_MENU       25

/*=============================================================================
 * Physical menu timing
 *===========================================================================*/
#define MENU_COMBO_HOLD_MS    6000u
#define MENU_COMBO_BEEP_MS    2000u
#define MENU_PAGE_TIMEOUT_MS  120000u
#define BTN_DEBOUNCE_MS       45u

/*=============================================================================
 * DEBUG
 *===========================================================================*/
#define DEBUG_ENABLE        1   /* 1 = Serial debug output */
#define DEBUG_AUDIO         1
#define DEBUG_COMMS         1
#define DEBUG_MIC           0   /* Noisy if 1 */
#define DEBUG_IR            0
#define DIAGNOSTICS_ENABLE  0   /* 1 = periodic diag block to Serial */

/* Minimal "alive" print to Serial when no hardware — every N ms */
#define ALIVE_PRINT_ENABLE    1   /* 1 = print when running standalone */
#define ALIVE_PRINT_INTERVAL_MS 5000

#endif
