/**
 * EVE companion node — Arduino IDE sketch (ESP32-S3, same logic as ../ PlatformIO eve/)
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX to WALL-E (see config.h).
 *
 * Libraries (Library Manager): ArduinoJson 6.x; lvgl 9.x; GFX Library for Arduino (moononournation).
 * Battery/current: Adafruit INA219 + Adafruit BusIO (when EVE_BATTERY_INA219 in config.h).
 * Optional ToF: Pololu VL53L1X. Place lv_conf.h in this sketch folder when using LVGL 9 face.
 *
 * Board: ESP32S3 Dev Module (match your N16R8 module: 16 MB flash, OPI PSRAM in Tools menu).
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
  Serial.println(F(" EVE companion (Arduino IDE build)"));
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
  eveAwarenessTick();
  stateMachineTick();
  eveBehaviorManagerTick();
  eveAttachmentManagerTick();
  eveStatusManagerTick();
  systemStatusTick();
  eyesTick();
  tofTick();
  servoTick();
  neopixelTick();
  audioTick();
  eveWebServerTick(stateMachineIsDocked());
}
