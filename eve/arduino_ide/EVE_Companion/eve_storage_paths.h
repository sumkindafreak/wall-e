/**
 * EVE — SD card directory layout (paths relative to SD mount root).
 */
#pragma once

#define EVE_SD_PATH_CONFIG "/config"
#define EVE_SD_PATH_AUDIO "/audio"
#define EVE_SD_PATH_IMAGES "/images"
#define EVE_SD_PATH_ANIMATIONS "/animations"
#define EVE_SD_PATH_LOGS "/logs"
#define EVE_SD_PATH_UPDATES "/updates"

#define EVE_SD_FILE_SETTINGS EVE_SD_PATH_CONFIG "/settings.json"
#define EVE_SD_FILE_ROBOT EVE_SD_PATH_CONFIG "/robot.json"
#define EVE_SD_FILE_AUDIO_CFG EVE_SD_PATH_CONFIG "/audio.json"
#define EVE_SD_FILE_DISPLAY EVE_SD_PATH_CONFIG "/display.json"
