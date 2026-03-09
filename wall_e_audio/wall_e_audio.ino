/*******************************************************
   WALL-E AUDIO BRAIN v2.0
   ESP32-S3 Dev Module
   DFPlayer Mini + I2S Mic (INMP441 etc.)
********************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "driver/i2s.h"

/**************** PIN DEFINITIONS ****************/

#define DFPLAYER_RX 16   // ESP RX  ← DFPlayer TX
#define DFPLAYER_TX 17   // ESP TX  → DFPlayer RX

// I2S Mic (INMP441: SCK→BCK, WS→LRCLK, SD→DATA, L/R→GND for left)
#define I2S_MIC_BCK   5
#define I2S_MIC_WS   25
#define I2S_MIC_DATA 26
#define I2S_MIC_PORT I2S_NUM_0

/**************** AUDIO EVENT ENUM ****************/

enum AudioEvent {
  AUDIO_NONE = 0,
  AUDIO_BOOT,
  AUDIO_IDLE,
  AUDIO_CURIOUS,
  AUDIO_ESTOP
};

struct AudioPacket {
  uint8_t event;
  uint8_t volume;
  uint8_t priority;
};

/**************** GLOBAL STATE ****************/

HardwareSerial DFSerial(1);
DFRobotDFPlayerMini DFPlayer;
bool dfplayerReady = false;
unsigned long lastIdleTime = 0;
unsigned long idleInterval = 30000;

/**************** TRACK NUMBERS (SD card: 001.mp3, 002.mp3, ... in root or mp3 folder) ****************/
#define TRACK_BOOT    1
#define TRACK_IDLE1   2
#define TRACK_IDLE2   3
#define TRACK_CURIOUS 4
#define TRACK_ESTOP   5

/**************** FUNCTION DECLARATIONS ****************/

void micInit();
int  micReadLevel();   // 0-32767, quick peak
void playTrack(uint8_t track);
void stopAudio();
void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len);

/**************** SETUP ****************/

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("WALL-E AUDIO BRAIN BOOTING (DFPlayer Mini)...");

  delay(500);
  DFSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(300);

  for (int i = 0; i < 8; i++) {
    if (DFPlayer.begin(DFSerial, true, true)) {
      dfplayerReady = true;
      Serial.println("DFPlayer Mini READY.");
      break;
    }
    Serial.println("DFPlayer init retry...");
    delay(500);
  }

  if (!dfplayerReady) {
    Serial.printf("DFPlayer INIT FAILED! RX=%d TX=%d — check wiring & SD card.\n", DFPLAYER_RX, DFPLAYER_TX);
  } else {
    DFPlayer.volume(30);  // Max volume (0-30)
    delay(200);
    playTrack(TRACK_BOOT);
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);

  micInit();
}

/**************** I2S MIC ****************/

void micInit() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = 16000;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;

  i2s_pin_config_t pin = {};
  pin.bck_io_num = I2S_MIC_BCK;
  pin.ws_io_num = I2S_MIC_WS;
  pin.data_in_num = I2S_MIC_DATA;
  pin.data_out_num = I2S_PIN_NO_CHANGE;
  pin.mck_io_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(I2S_MIC_PORT, &cfg, 0, NULL) == ESP_OK &&
      i2s_set_pin(I2S_MIC_PORT, &pin) == ESP_OK) {
    i2s_zero_dma_buffer(I2S_MIC_PORT);
    Serial.println("I2S Mic READY.");
  } else {
    Serial.println("I2S Mic INIT FAILED.");
  }
}

int micReadLevel() {
  int16_t buf[64];
  size_t read = 0;
  if (i2s_read(I2S_MIC_PORT, buf, sizeof(buf), &read, 10) != ESP_OK) return 0;
  int peak = 0;
  for (size_t i = 0; i < read / 2; i++) {
    int v = abs((int)buf[i]);
    if (v > peak) peak = v;
  }
  return peak;
}

/**************** PLAY ****************/

void playTrack(uint8_t track) {
  if (!dfplayerReady) return;
  DFPlayer.play(track);
  Serial.print("PLAYING: track ");
  Serial.println(track);
}

void stopAudio() {
  if (!dfplayerReady) return;
  DFPlayer.pause();
}

/**************** AUDIO UPDATE ****************/

void audioUpdate() {
  if (!dfplayerReady) return;

  if (millis() - lastIdleTime > idleInterval) {
    int r = random(1, 3);
    playTrack(r == 1 ? TRACK_IDLE1 : TRACK_IDLE2);
    lastIdleTime = millis();
    idleInterval = random(20000, 60000);
  }
}

/**************** ESP-NOW RECEIVE ****************/

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
  (void)info;
  AudioPacket packet;
  memcpy(&packet, incomingData, sizeof(packet));

  Serial.print("EVENT RECEIVED: ");
  Serial.println(packet.event);

  switch (packet.event) {
    case AUDIO_IDLE:
      playTrack(TRACK_IDLE1);
      break;
    case AUDIO_CURIOUS:
      playTrack(TRACK_CURIOUS);
      break;
    case AUDIO_ESTOP:
      playTrack(TRACK_ESTOP);
      break;
  }
}

/**************** LOOP ****************/

void loop() {
  audioUpdate();
}
