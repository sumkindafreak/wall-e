/**
 * menu_protocol.h — Physical 4-button menu + fake MP3 player (telemetry to base).
 */
#ifndef WALL_E_MENU_PROTOCOL_H
#define WALL_E_MENU_PROTOCOL_H

#include <stdint.h>

#define WALLE_MENU_PROTO_VERSION 1u

/** Audio ESP -> Base: UI state + button events. Magic "WAUI" */
#define WALLE_AUDIO_UI_MAGIC 0x57415549u

typedef enum {
  WALLE_BTN_MODE_NORMAL = 0,   /* fake MP3 player */
  WALLE_BTN_MODE_MENU = 1,
} walle_btn_mode_t;

typedef enum {
  WALLE_MENU_PAGE_STATUS = 0,
  WALLE_MENU_PAGE_AUDIO_TEST = 1,
  WALLE_MENU_PAGE_EXPRESSIONS = 2,
  WALLE_MENU_PAGE_DOCK_STATUS = 3,
  WALLE_MENU_PAGE_EVE_LINK = 4,
  WALLE_MENU_PAGE_VOICE_BOX = 5,
  WALLE_MENU_PAGE_MEMORY_LOG = 6,
  WALLE_MENU_PAGE_SYSTEM_INFO = 7,
  WALLE_MENU_PAGE_SAFE_REBOOT = 8,
  WALLE_MENU_PAGE_COUNT = 9,
} walle_menu_page_t;

typedef enum {
  WALLE_UI_EVT_NONE = 0,
  WALLE_UI_EVT_BTN_PLAY = 1,
  WALLE_UI_EVT_BTN_STOP = 2,
  WALLE_UI_EVT_BTN_REWIND = 3,
  WALLE_UI_EVT_BTN_RECORD = 4,
  WALLE_UI_EVT_MENU_ENTER = 10,
  WALLE_UI_EVT_MENU_EXIT = 11,
  WALLE_UI_EVT_MENU_NAV = 12,
  WALLE_UI_EVT_SAFE_REBOOT_ARM = 20,
} walle_ui_event_t;

typedef enum {
  WALLE_UI_PAIR_NONE = 0,
  WALLE_UI_PAIR_EVE_PLAY_ACK = 1,
  WALLE_UI_PAIR_EVE_STOP_SETTLE = 2,
  WALLE_UI_PAIR_EVE_REWIND_CONFUSED = 3,
  WALLE_UI_PAIR_EVE_RECORD_REACT = 4,
  WALLE_UI_PAIR_MODE_TRANSITION = 5,
  WALLE_UI_PAIR_EVE_EXPRESSION = 6,
} walle_ui_pair_request_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t version;
  uint8_t btn_mode;       /* walle_btn_mode_t */
  uint8_t menu_page;      /* walle_menu_page_t */
  uint8_t voicebox_mode;  /* echo walle_voicebox_mode_t */
  uint8_t last_ui_event;  /* walle_ui_event_t */
  uint8_t combo_hold_pct; /* 0–100 for 6s enter/exit hold feedback */
  uint8_t menu_sel_idx;   /* cursor within page */
  uint8_t pair_request;   /* walle_ui_pair_request_t */
  uint16_t seq;           /* monotonic per audio boot */
} WalleAudioUiTelemPacket_t;

#endif
