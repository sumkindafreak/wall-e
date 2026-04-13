/**
 * pins.h — Pin definitions for Audio ESP
 * Replace TBD values with your final GPIO assignments.
 */
#ifndef AUDIO_ESP_PINS_H
#define AUDIO_ESP_PINS_H

/*=============================================================================
 * DFPlayer Mini (UART1)
 * DFPlayer RX <- ESP TX,  DFPlayer TX -> ESP RX
 *===========================================================================*/
#define PIN_DFPLAYER_TX   17   /* ESP TX -> DFPlayer RX */
#define PIN_DFPLAYER_RX   18   /* ESP RX <- DFPlayer TX */

/* Base link: ESP-NOW (no UART pins) */

/*=============================================================================
 * Voice command serial (optional external module)
 * If not used, voice commands come from Base. Set to -1 to disable.
 *===========================================================================*/
#define PIN_VOICE_TX      -1   /* -1 = not used */
#define PIN_VOICE_RX      -1   /* -1 = not used */

/*=============================================================================
 * Left and right microphones (analog, ADC1)
 * ESP32-S3: use ADC1 channels (avoid ADC2 when WiFi active)
 *===========================================================================*/
#define PIN_MIC_LEFT      4    /* ADC1_CHANNEL_3 */
#define PIN_MIC_RIGHT     5    /* ADC1_CHANNEL_4 */

/*=============================================================================
 * IR dock receivers (digital, INPUT_PULLUP)
 * Typically: LOW = IR detected, HIGH = no signal (depends on receiver)
 *===========================================================================*/
#define PIN_IR_DOCK_LEFT  6
#define PIN_IR_DOCK_RIGHT 7

/*=============================================================================
 * Status LED (optional)
 *===========================================================================*/
#define PIN_STATUS_LED    8    /* -1 to disable */

/*=============================================================================
 * Character control buttons (INPUT_PULLUP, contact to GND when pressed)
 * ESP32-S3: chosen to avoid DFPlayer UART (17/18), mics (4/5), IR (6/7), LED (8).
 *===========================================================================*/
#define PIN_BTN_1         9
#define PIN_BTN_2         10
#define PIN_BTN_3         11
#define PIN_BTN_4         12

/** Menu enter/exit combo: hold these two together for 6 s (named for readability). */
#define PIN_MENU_COMBO_A  PIN_BTN_1
#define PIN_MENU_COMBO_B  PIN_BTN_2

#endif
