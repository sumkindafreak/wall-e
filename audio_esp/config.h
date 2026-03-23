/**
 * config.h — Feature flags, timing, track IDs, thresholds
 */
#ifndef AUDIO_ESP_CONFIG_H
#define AUDIO_ESP_CONFIG_H

/*=============================================================================
 * UART (DFPlayer only; Base link is ESP-NOW)
 *===========================================================================*/
#define DFPLAYER_BAUD       9600
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
 * AUDIO — DFPlayer track IDs (001.mp3 = 1, 002.mp3 = 2, ...)
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

/*=============================================================================
 * AUDIO PRIORITY (higher = can interrupt lower)
 *===========================================================================*/
#define PRIO_LOWEST     0
#define PRIO_NORMAL     10
#define PRIO_ACK        20
#define PRIO_VOICE_CMD  30
#define PRIO_ERROR      40
#define PRIO_ESTOP      50

/*=============================================================================
 * DEBUG
 *===========================================================================*/
#define DEBUG_ENABLE        0   /* 1 = Serial debug output */
#define DEBUG_AUDIO         0
#define DEBUG_COMMS         0
#define DEBUG_MIC           0   /* Noisy if 1 */
#define DEBUG_IR            0
#define DIAGNOSTICS_ENABLE  0   /* 1 = periodic diag block to Serial */

/* Minimal "alive" print to Serial when no hardware — every N ms */
#define ALIVE_PRINT_ENABLE    1   /* 1 = print when running standalone */
#define ALIVE_PRINT_INTERVAL_MS 5000

#endif
