/******************************************************************************
 * WALL-E AUDIO BRAIN v3.0 - Voice Commands
 * ESP32-S3 | DFPlayer Mini + I2S Mic (INMP441) | ESP-SR Wake Word
 *
 * Voice command: Say "Hi Wall-E" (or "Hi Wall E") -> plays track 2 (002.mp3)
 *
 * Build with PlatformIO (framework=espidf), NOT Arduino IDE.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "esp_afe_config.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const char *TAG = "wall_e_audio";

/* Pins - same as Arduino version */
#define DFPLAYER_UART_NUM    UART_NUM_1
#define DFPLAYER_TX_GPIO     17
#define DFPLAYER_RX_GPIO     16
#define I2S_MIC_BCK          5
#define I2S_MIC_WS           4
#define I2S_MIC_DATA         6
#define MIC_SW_GAIN          8

/* Tracks - 001.mp3, 002.mp3, etc. on SD root or mp3/ folder */
#define TRACK_BOOT    1
#define TRACK_VOICE_REPLY 2   /* Reply when "Hi Wall-E" detected */
#define TRACK_IDLE1   3
#define TRACK_IDLE2   4
#define TRACK_CURIOUS 5
#define TRACK_ESTOP   6

/* DFPlayer UART baud */
#define DFPLAYER_BAUD 9600

/* Globals */
static const esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static const esp_mn_iface_t *mn_handle = NULL;
static model_iface_data_t *mn_data = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static volatile uint8_t dfplayer_ready = 0;
static volatile int voice_task_running = 1;
static TickType_t last_wake_tick = 0;
static TickType_t command_listen_start = 0;
static TickType_t command_listen_until = 0;
static TickType_t playback_lock_until = 0;
static uint8_t current_dfplayer_volume = 30;

#define WAKE_COOLDOWN_MS     2500
#define COMMAND_LISTEN_MS    12000
#define COMMAND_START_DELAY_MS 25000
#define ALWAYS_LISTEN_COMMANDS 0

#define PLAY_GUARD_DEFAULT_MS 1800
#define PLAY_GUARD_WAKE_REPLY_MS 4500

#define CMD_VOL_UP            1
#define CMD_VOL_DOWN          2
#define CMD_VOL_MAX           3
#define CMD_VOL_MUTE          4
#define CMD_VOL_UP_ALT        6
#define CMD_VOL_DOWN_ALT      7
#define CMD_VOL_MAX_ALT       8
#define CMD_VOL_MUTE_ALT      9

/* Phrase commands: play track (7-26) as audio reply */
#define CMD_PHRASE_BASE       10
#define CMD_HELLO             10
#define CMD_HOW_ARE_YOU       11
#define CMD_WHAT_YOUR_NAME    12
#define CMD_TELL_JOKE         13
#define CMD_SING              14
#define CMD_PLAY_MUSIC        15
#define CMD_STOP              16
#define CMD_THANK_YOU         17
#define CMD_GOOD_JOB          18
#define CMD_YOU_ARE_CUTE      19
#define CMD_WAVE              20
#define CMD_DANCE             21
#define CMD_BE_HAPPY          22
#define CMD_BEEP              23
#define CMD_CURIOUS           24
#define CMD_EXCITED           25
#define CMD_SCARED            26
#define CMD_GOOD_NIGHT        27
#define CMD_GOOD_MORNING      28
#define CMD_I_LOVE_YOU        29
#define CMD_NICE_TO_MEET_YOU  30
#define CMD_ARE_YOU_THERE     31
#define CMD_YES               32
#define CMD_NO                33

/* Audio event (ESP-NOW) */
typedef struct {
    uint8_t event;
    uint8_t volume;
    uint8_t priority;
} AudioPacket_t;

/* Forward declarations */
static void dfplayer_init(void);
static void dfplayer_play(uint8_t track);
static void dfplayer_volume(uint8_t vol);
static bool i2s_mic_init(void);
static void voice_wakeup_callback(void);
static bool multinet_init(srmodel_list_t *models);
static void handle_voice_command(int command_id, const char *command_text);
static bool model_partition_sane(void);

static bool contains_token_ci(const char *text, const char *token)
{
    if (!text || !token || !token[0]) {
        return false;
    }

    size_t token_len = strlen(token);
    for (const char *scan = text; *scan; scan++) {
        size_t i = 0;
        while (i < token_len && scan[i] &&
               tolower((unsigned char)scan[i]) == tolower((unsigned char)token[i])) {
            i++;
        }
        if (i == token_len) {
            return true;
        }
    }
    return false;
}

static bool model_partition_sane(void)
{
    const esp_partition_t *model_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "model");
    if (!model_part) {
        ESP_LOGE(TAG, "Model partition not found");
        return false;
    }

    uint32_t model_count = 0;
    esp_err_t err = esp_partition_read(model_part, 0, &model_count, sizeof(model_count));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read model partition header: %s", esp_err_to_name(err));
        return false;
    }

    if (model_count == 0 || model_count > 64) {
        ESP_LOGE(TAG, "Invalid model partition header (count=%lu). Flash models with ESP-IDF flash target.", (unsigned long)model_count);
        return false;
    }

    ESP_LOGI(TAG, "Model partition looks valid (count=%lu)", (unsigned long)model_count);
    return true;
}

static void mic_apply_gain(int16_t *samples, size_t sample_count, int gain)
{
    for (size_t i = 0; i < sample_count; i++) {
        int32_t scaled = (int32_t)samples[i] * gain;
        if (scaled > 32767) {
            scaled = 32767;
        } else if (scaled < -32768) {
            scaled = -32768;
        }
        samples[i] = (int16_t)scaled;
    }
}

/******************************************************************************
 * DFPlayer Mini - minimal UART driver
 *****************************************************************************/
static void dfplayer_send_cmd(uint8_t cmd, uint8_t arg1, uint8_t arg2)
{
    uint8_t buf[10] = {
        0x7E, 0xFF, 0x06, cmd, 0x00, arg1, arg2,
        0, 0, 0xEF
    };
    uint16_t sum = -(0xFF + 0x06 + cmd + 0x00 + arg1 + arg2);
    buf[7] = (sum >> 8) & 0xFF;
    buf[8] = sum & 0xFF;
    uart_write_bytes(DFPLAYER_UART_NUM, buf, 10);
}

static void dfplayer_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = DFPLAYER_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(DFPLAYER_UART_NUM, &uart_config);
    uart_set_pin(DFPLAYER_UART_NUM, DFPLAYER_TX_GPIO, DFPLAYER_RX_GPIO, -1, -1);
    uart_driver_install(DFPLAYER_UART_NUM, 256, 256, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(300));
    for (int i = 0; i < 8; i++) {
        dfplayer_send_cmd(0x3F, 0, 0); /* query chip */
        vTaskDelay(pdMS_TO_TICKS(500));
        uint8_t rsp[10];
        int len = uart_read_bytes(DFPLAYER_UART_NUM, rsp, 10, 100);
        if (len > 0) {
            dfplayer_ready = 1;
            ESP_LOGI(TAG, "DFPlayer Mini READY");
            break;
        }
    }
    if (!dfplayer_ready) {
        ESP_LOGW(TAG, "DFPlayer init failed - check wiring RX=%d TX=%d", DFPLAYER_RX_GPIO, DFPLAYER_TX_GPIO);
        return;
    }
    dfplayer_volume(30);
    vTaskDelay(pdMS_TO_TICKS(200));
    dfplayer_play(TRACK_BOOT);
}

static void dfplayer_play(uint8_t track)
{
    if (!dfplayer_ready) return;

    TickType_t now = xTaskGetTickCount();
    bool is_estop = (track == TRACK_ESTOP);
    if (!is_estop && now < playback_lock_until) {
        ESP_LOGI(TAG, "PLAY skipped track %d (busy)", track);
        return;
    }

    dfplayer_send_cmd(0x03, 0, track);
    ESP_LOGI(TAG, "PLAY track %d", track);

    TickType_t guard_ms = (track == TRACK_VOICE_REPLY) ? PLAY_GUARD_WAKE_REPLY_MS : PLAY_GUARD_DEFAULT_MS;
    playback_lock_until = now + pdMS_TO_TICKS(guard_ms);
}

static void dfplayer_volume(uint8_t vol)
{
    if (!dfplayer_ready) return;
    if (vol > 30) vol = 30;
    if (vol == current_dfplayer_volume) return;
    current_dfplayer_volume = vol;
    dfplayer_send_cmd(0x06, 0, vol);
    ESP_LOGI(TAG, "DFPlayer volume=%u", vol);
}

/******************************************************************************
 * I2S Microphone (INMP441)
 *****************************************************************************/
static bool i2s_mic_init(void)
{
    ESP_LOGI(TAG, "I2S Mic pins: BCK=%d WS=%d DATA=%d", I2S_MIC_BCK, I2S_MIC_WS, I2S_MIC_DATA);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 256;
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    if (err != ESP_OK || rx_handle == NULL) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_MIC_BCK,
            .ws = I2S_MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC_DATA,
        },
    };
    err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "I2S Mic READY (16kHz mono)");
    return true;
}

/******************************************************************************
 * Voice wake-up callback - called when "Hi Wall-E" detected
 *****************************************************************************/
static void voice_wakeup_callback(void)
{
    TickType_t now = xTaskGetTickCount();
    if ((now - last_wake_tick) < pdMS_TO_TICKS(WAKE_COOLDOWN_MS)) {
        return;
    }
    last_wake_tick = now;

    ESP_LOGI(TAG, "VOICE: Hi Wall-E detected! -> Playing track %d (002.mp3)", TRACK_VOICE_REPLY);
    dfplayer_play(TRACK_VOICE_REPLY);
    command_listen_start = now + pdMS_TO_TICKS(COMMAND_START_DELAY_MS);
    command_listen_until = command_listen_start + pdMS_TO_TICKS(COMMAND_LISTEN_MS);
    ESP_LOGI(TAG, "Command mode in %d ms for %d ms", COMMAND_START_DELAY_MS, COMMAND_LISTEN_MS);
}

static void handle_voice_command(int command_id, const char *command_text)
{
    ESP_LOGI(TAG, "Handle command id=%d text=%s (current volume=%u, dfplayer_ready=%u)",
             command_id,
             command_text ? command_text : "<null>",
             current_dfplayer_volume,
             dfplayer_ready);

    bool is_volume_up = (command_id == CMD_VOL_UP || command_id == CMD_VOL_UP_ALT);
    bool is_volume_down = (command_id == CMD_VOL_DOWN || command_id == CMD_VOL_DOWN_ALT);
    bool is_volume_max = (command_id == CMD_VOL_MAX || command_id == CMD_VOL_MAX_ALT);
    bool is_volume_mute = (command_id == CMD_VOL_MUTE || command_id == CMD_VOL_MUTE_ALT);

    if (command_text) {
        bool has_volume = contains_token_ci(command_text, "volume");
        if (has_volume && (contains_token_ci(command_text, "up") ||
                           contains_token_ci(command_text, "increase") ||
                           contains_token_ci(command_text, "higher"))) {
            is_volume_up = true;
        }
        if (has_volume && (contains_token_ci(command_text, "down") ||
                           contains_token_ci(command_text, "decrease") ||
                           contains_token_ci(command_text, "lower"))) {
            is_volume_down = true;
        }
        if (has_volume && (contains_token_ci(command_text, "max") ||
                           contains_token_ci(command_text, "maximum"))) {
            is_volume_max = true;
        }
        if (has_volume && (contains_token_ci(command_text, "mute") ||
                           contains_token_ci(command_text, "silent"))) {
            is_volume_mute = true;
        }
    }

    if (is_volume_mute) {
        ESP_LOGI(TAG, "Command: volume mute");
        dfplayer_volume(0);
        return;
    }

    if (is_volume_max) {
        ESP_LOGI(TAG, "Command: volume max");
        dfplayer_volume(30);
        return;
    }

    if (is_volume_up) {
        uint8_t new_volume = (current_dfplayer_volume + 3 > 30) ? 30 : (uint8_t)(current_dfplayer_volume + 3);
        ESP_LOGI(TAG, "Command: volume up");
        dfplayer_volume(new_volume);
        return;
    }

    if (is_volume_down) {
        int new_volume = (int)current_dfplayer_volume - 3;
        if (new_volume < 0) new_volume = 0;
        ESP_LOGI(TAG, "Command: volume down");
        dfplayer_volume((uint8_t)new_volume);
        return;
    }

    /* Map phrase commands to audio reply tracks (007.mp3 - 026.mp3) */
    int track = 0;
    switch (command_id) {
        case CMD_HELLO:           track = 7;  break;
        case CMD_HOW_ARE_YOU:     track = 8;  break;
        case CMD_WHAT_YOUR_NAME:  track = 9;  break;
        case CMD_TELL_JOKE:       track = 10; break;
        case CMD_SING:            track = 11; break;
        case CMD_PLAY_MUSIC:      track = 12; break;
        case CMD_STOP:            track = 13; break;  /* or pause - use 013.mp3 as "stopping" reply */
        case CMD_THANK_YOU:       track = 14; break;
        case CMD_GOOD_JOB:        track = 15; break;
        case CMD_YOU_ARE_CUTE:    track = 16; break;
        case CMD_WAVE:            track = 17; break;
        case CMD_DANCE:           track = 18; break;
        case CMD_BE_HAPPY:        track = 19; break;
        case CMD_BEEP:            track = 20; break;
        case CMD_CURIOUS:         track = TRACK_CURIOUS; break;
        case CMD_EXCITED:         track = 21; break;
        case CMD_SCARED:          track = 22; break;
        case CMD_GOOD_NIGHT:      track = 23; break;
        case CMD_GOOD_MORNING:    track = 24; break;
        case CMD_I_LOVE_YOU:      track = 25; break;
        case CMD_NICE_TO_MEET_YOU: track = 26; break;
        case CMD_ARE_YOU_THERE:   track = 7;  break;  /* reuse hello/greeting */
        case CMD_YES:             track = 14; break;  /* reuse thank you / positive */
        case CMD_NO:              track = 22; break;  /* reuse scared / negative */
        default:
            ESP_LOGI(TAG, "Command: unknown id=%d text=%s", command_id, command_text ? command_text : "<null>");
            return;
    }

    if (command_id == CMD_STOP) {
        /* Stop/pause playback instead of playing a track */
        if (dfplayer_ready) {
            dfplayer_send_cmd(0x0E, 0, 0);  /* pause */
            ESP_LOGI(TAG, "Command: stop (paused)");
        }
    } else {
        dfplayer_play((uint8_t)track);
        ESP_LOGI(TAG, "Command: play track %d (id=%d)", track, command_id);
    }
}

/* Command phrase definitions for MultiNet - 4 volume + 24 phrases = 28 total */
#define COMMAND_COUNT 28

static const struct {
    const char *string;
    int16_t command_id;
} command_phrases[] = {
    { "volume up",        CMD_VOL_UP },
    { "volume down",      CMD_VOL_DOWN },
    { "max volume",       CMD_VOL_MAX },
    { "volume mute",      CMD_VOL_MUTE },
    { "hello",            CMD_HELLO },
    { "hi",               CMD_HELLO },
    { "how are you",      CMD_HOW_ARE_YOU },
    { "what is your name", CMD_WHAT_YOUR_NAME },
    { "tell me a joke",   CMD_TELL_JOKE },
    { "sing",             CMD_SING },
    { "sing a song",      CMD_SING },
    { "play music",       CMD_PLAY_MUSIC },
    { "stop",             CMD_STOP },
    { "thank you",        CMD_THANK_YOU },
    { "thanks",           CMD_THANK_YOU },
    { "good job",         CMD_GOOD_JOB },
    { "you are cute",     CMD_YOU_ARE_CUTE },
    { "wave",             CMD_WAVE },
    { "dance",            CMD_DANCE },
    { "be happy",         CMD_BE_HAPPY },
    { "beep",             CMD_BEEP },
    { "curious",          CMD_CURIOUS },
    { "excited",          CMD_EXCITED },
    { "scared",           CMD_SCARED },
    { "good night",       CMD_GOOD_NIGHT },
    { "good morning",     CMD_GOOD_MORNING },
    { "i love you",       CMD_I_LOVE_YOU },
    { "nice to meet you", CMD_NICE_TO_MEET_YOU },
    { "are you there",    CMD_ARE_YOU_THERE },
    { "yes",              CMD_YES },
    { "no",               CMD_NO },
};

#define PHRASE_COUNT (sizeof(command_phrases) / sizeof(command_phrases[0]))

static bool multinet_init(srmodel_list_t *models)
{
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    if (!mn_name) {
        ESP_LOGW(TAG, "No English MultiNet model in partition; voice commands disabled");
        return false;
    }

    mn_handle = esp_mn_handle_from_name(mn_name);
    if (!mn_handle) {
        ESP_LOGE(TAG, "esp_mn_handle_from_name failed for %s", mn_name);
        return false;
    }

    mn_data = mn_handle->create(mn_name, 6000);
    if (!mn_data) {
        ESP_LOGE(TAG, "MultiNet create failed");
        return false;
    }

    /* Build linked list of command nodes */
    static esp_mn_phrase_t cmd_phrases[PHRASE_COUNT];
    static esp_mn_node_t cmd_nodes[PHRASE_COUNT];

    for (size_t i = 0; i < PHRASE_COUNT; i++) {
        cmd_phrases[i] = (esp_mn_phrase_t){
            .string = command_phrases[i].string,
            .phonemes = NULL,
            .command_id = command_phrases[i].command_id,
            .threshold = 0.0f,
            .wave = NULL,
        };
        cmd_nodes[i].phrase = &cmd_phrases[i];
        cmd_nodes[i].next = (i + 1 < PHRASE_COUNT) ? &cmd_nodes[i + 1] : NULL;
    }

    esp_mn_error_t *cmd_err = mn_handle->set_speech_commands(mn_data, &cmd_nodes[0]);
    if (cmd_err && cmd_err->num > 0) {
        ESP_LOGW(TAG, "Some speech commands rejected by MultiNet (count=%d)", cmd_err->num);
    }

    mn_handle->set_det_threshold(mn_data, 0.30f);
    ESP_LOGI(TAG, "MultiNet READY: %s (%zu commands)", mn_name, PHRASE_COUNT);
    return true;
}

/******************************************************************************
 * AFE Feed Task - reads I2S and feeds AFE
 *****************************************************************************/
static void feed_task(void *arg)
{
    int chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    size_t buf_bytes = chunksize * nch * sizeof(int16_t);
    int16_t *i2s_buf = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL);
    if (!i2s_buf) {
        ESP_LOGE(TAG, "feed_task: malloc failed");
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_read;
    while (voice_task_running) {
        esp_err_t err = i2s_channel_read(rx_handle, i2s_buf, buf_bytes, &bytes_read, portMAX_DELAY);
        if (err == ESP_OK && bytes_read == buf_bytes) {
            mic_apply_gain(i2s_buf, (size_t)(chunksize * nch), MIC_SW_GAIN);
            afe_handle->feed(afe_data, i2s_buf);
        }
    }
    free(i2s_buf);
    vTaskDelete(NULL);
}

/******************************************************************************
 * AFE Detect Task - fetches results, triggers on wake word
 *****************************************************************************/
static void detect_task(void *arg)
{
    ESP_LOGI(TAG, "Voice detection started - say \"Hi Wall-E\" to play track 2");
    int last_wake_state = WAKENET_NO_DETECT;
    uint32_t yield_counter = 0;

    while (voice_task_running) {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (res->wakeup_state == WAKENET_DETECTED && last_wake_state != WAKENET_DETECTED) {
            ESP_LOGI(TAG, "Wake word detected (model %d, word %d)", res->wakenet_model_index, res->wake_word_index);
            voice_wakeup_callback();
        }

        TickType_t now = xTaskGetTickCount();
        bool command_window_active = (now >= command_listen_start) && (now <= command_listen_until);
        if (mn_handle && mn_data && res->data && res->data_size > 0 && (ALWAYS_LISTEN_COMMANDS || command_window_active)) {
            esp_mn_state_t mn_state = mn_handle->detect(mn_data, res->data);
            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_results = mn_handle->get_results(mn_data);
                if (mn_results && mn_results->num > 0) {
                    int command_id = mn_results->command_id[0];
                    ESP_LOGI(TAG, "Voice command detected: id=%d text=%s num=%d", command_id, mn_results->string, mn_results->num);
                    handle_voice_command(command_id, mn_results->string);
                    if (!ALWAYS_LISTEN_COMMANDS) {
                        command_listen_start = 0;
                        command_listen_until = 0;
                    }
                    mn_handle->clean(mn_data);
                }
            } else if (mn_state == ESP_MN_STATE_TIMEOUT) {
                mn_handle->clean(mn_data);
                if (!ALWAYS_LISTEN_COMMANDS) {
                    command_listen_start = 0;
                    command_listen_until = 0;
                    ESP_LOGI(TAG, "Command mode timeout");
                }
            }
        }

        last_wake_state = res->wakeup_state;
        if ((++yield_counter & 0x3F) == 0) {
            taskYIELD();
        }
    }
    vTaskDelete(NULL);
}


/******************************************************************************
 * ESP-NOW receive callback
 *****************************************************************************/
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    if (len < sizeof(AudioPacket_t)) return;
    AudioPacket_t *pkt = (AudioPacket_t *)data;
    ESP_LOGI(TAG, "ESP-NOW event %d", pkt->event);

    switch (pkt->event) {
        case 2:  /* AUDIO_IDLE */
            dfplayer_play(TRACK_IDLE1);
            break;
        case 3:  /* AUDIO_CURIOUS */
            dfplayer_play(TRACK_CURIOUS);
            break;
        case 4:  /* AUDIO_ESTOP */
            dfplayer_play(TRACK_ESTOP);
            break;
        default:
            break;
    }
}

/******************************************************************************
 * app_main
 *****************************************************************************/
void app_main(void)
{
    esp_err_t err;
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "WALL-E AUDIO BRAIN v3.0 - Voice Commands");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* DFPlayer */
    dfplayer_init();

    /* NVS (required by Wi-Fi/ESP-NOW) */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires erase (err=0x%x), erasing...", err);
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "NVS READY");

    /* ESP-NOW */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WIFI READY (STA)");
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "ESP-NOW READY");

    /* I2S Mic */
    if (!i2s_mic_init()) {
        ESP_LOGE(TAG, "I2S Mic init failed, stopping app_main");
        return;
    }

    /* ESP-SR AFE + WakeNet */
    if (!model_partition_sane()) {
        ESP_LOGE(TAG, "Model data missing/invalid. Use task: WALL-E: Flash full image (includes models)");
        return;
    }

    srmodel_list_t *models = esp_srmodel_init("model");
    if (!models) {
        ESP_LOGE(TAG, "esp_srmodel_init failed - ensure model partition & sdkconfig");
        return;
    }

    /* Single-mic format "M" for INMP441 */
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    if (!afe_config) {
        ESP_LOGE(TAG, "afe_config_init failed");
        return;
    }

    afe_config->wakenet_mode = DET_MODE_95;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_speech_ms = 96;
    afe_config->vad_min_noise_ms = 400;
    afe_config->afe_linear_gain = 1.8f;

    afe_handle = esp_afe_handle_from_config(afe_config);
    if (!afe_handle) {
        afe_config_free(afe_config);
        ESP_LOGE(TAG, "esp_afe_handle_from_config failed");
        return;
    }

    afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    if (!afe_handle || !afe_data) {
        ESP_LOGE(TAG, "AFE create failed");
        return;
    }

    int threshold_result = afe_handle->set_wakenet_threshold(afe_data, 1, 0.45f);
    ESP_LOGI(TAG, "WakeNet threshold set result=%d target=0.45 (MIC_SW_GAIN=%d)", threshold_result, MIC_SW_GAIN);

    multinet_init(models);

    ESP_LOGI(TAG, "Wake word: Hi Wall-E -> track %d", TRACK_VOICE_REPLY);
    ESP_LOGI(TAG, "Command mode: %s", ALWAYS_LISTEN_COMMANDS ? "ALWAYS_LISTEN" : "WAKE_WINDOW_12S");

    voice_task_running = 1;
    xTaskCreatePinnedToCore(feed_task, "feed", 8 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(detect_task, "detect", 6 * 1024, NULL, 6, NULL, 1);
}
