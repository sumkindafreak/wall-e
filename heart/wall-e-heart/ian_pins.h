/***************************************************************
  IAN Pin Definitions (wall-e-heart)
  Same layout as showduino IAN v1.0
***************************************************************/

#ifndef IAN_PINS_H
#define IAN_PINS_H

#include <stdint.h>

// I2S PCM5102
#define PIN_I2S_BCLK    8
#define PIN_I2S_LRCLK   6
#define PIN_I2S_DATA    7

// SD Card (SPI)
#define PIN_SD_CS       9
#define PIN_SD_MOSI    10
#define PIN_SD_MISO    11
#define PIN_SD_SCK     12

#endif
