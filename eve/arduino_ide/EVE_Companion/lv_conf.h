/**
 * LVGL 9 configuration — EVE companion face (RGB565, partial flush).
 * Included via -DLV_CONF_INCLUDE_SIMPLE and -I include
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * COLOR
 *====================*/
#define LV_COLOR_DEPTH 16

/*====================
 * STDLIB (Arduino / ESP32)
 *====================*/
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB
#define LV_STDINT_INCLUDE <stdint.h>
#define LV_STDDEF_INCLUDE <stddef.h>
#define LV_STDBOOL_INCLUDE <stdbool.h>
#define LV_INTTYPES_INCLUDE <inttypes.h>
#define LV_LIMITS_INCLUDE <limits.h>
#define LV_STDARG_INCLUDE <stdarg.h>

#define LV_USE_OS LV_OS_NONE

#define LV_MEM_SIZE (96U * 1024U)

/*====================
 * HAL
 *====================*/
#define LV_DEF_REFR_PERIOD 33
#define LV_DPI_DEF 130

/*====================
 * LOG
 *====================*/
#define LV_USE_LOG 1
#if LV_USE_LOG
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1
#endif

/*====================
 * FONT
 *====================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
 * WIDGETS / DRAW
 *====================*/
#define LV_USE_ANIMATION 1
#define LV_USE_CANVAS 1
#define LV_USE_ARC 1
#define LV_USE_IMAGE 1
#define LV_USE_LABEL 1
#define LV_USE_OBJ 1

#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE

#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1

#endif /* LV_CONF_H */
