/**
 * EVE companion node — Arduino IDE (ESP32-S3 N16R8)
 *
 * Same logic as PlatformIO `eve/src/main.cpp`. Regenerate after PIO edits:
 *   bash scripts/sync_arduino_ide.sh
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX (see config.h).
 *
 * Libraries (Library Manager):
 *   - ArduinoJson 6.x
 *   - Adafruit INA219 + Adafruit BusIO (battery, when enabled in config.h)
 *   - lvgl 9 + Arduino_GFX (eyes only, when EVE_ENABLE_EYES)
 *   - Pololu VL53L1X (ToF, when EVE_ENABLE_TOF)
 *
 * Bundled in this sketch folder: walle_i2s_wav_player.* (SD → WAV → I2S; no DFPlayer).
 *
 * Board: ESP32S3 Dev Module — 16 MB flash, OPI PSRAM enabled in Tools menu.
 */
#include <Arduino.h>
#include "config.h"
#include "uart_link.h"
#include "state_machine.h"
#include "system_status.h"
#include "eyes_control.h"
#include "tof_control.h"
#include "servo_control.h"
#include "neopixel_control.h"
#include "audio_control.h"
#include "battery_monitor.h"
#include "eve_behavior_manager.h"
#include "eve_attachment_manager.h"
#include "eve_status_manager.h"
#include "eve_spatial_awareness.h"
#include "eve_desktop_companion.h"
#include "eve_web_server.h"
#include "mic_input.h"
#include "eve_serial_console.h"
#include "awareness/eve_awareness.h"

static void onUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
  stateMachineOnUartRx(type, payload, len, seq);
}

void setup() {
  Serial.begin(115200);
  randomSeed((uint32_t)micros());
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" EVE companion node (ESP32-S3)"));
  Serial.println(F(" UART hand link: Serial1"));
  Serial.println(F("========================================"));

  systemStatusInit();
  eveBatteryInit();
  eveAwarenessInit();
  uartLinkInit();
  uartLinkSetRxCallback(onUartRx);

  eyesInit();
  eveSpatialAwarenessInit();
  tofInit();
  servoInit();
  neopixelInit();
  audioInit();
  initMic();

#if EVE_PRESENT_PIN >= 0
  pinMode(EVE_PRESENT_PIN, INPUT_PULLUP);
#endif

  stateMachineInit();

  eveBehaviorManagerInit();
  eveAttachmentManagerInit();
  eveStatusManagerInit();
  eveWebServerInit();
  eveSerialConsoleInit();

  Serial.print(F("[EVE] Free heap: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  eveSerialConsoleTick();
  uartLinkPoll();
  updateMic();
  eveBatteryTick();
  stateMachineTick();
  eveBehaviorManagerTick();
  eveAttachmentManagerTick();
  eveStatusManagerTick();
  systemStatusTick();
  eyesTick();
  tofTick();
  eveAwarenessTick();
  servoTick();
  neopixelTick();
  audioTick();
  eveWebServerTick(stateMachineIsDocked());
}
