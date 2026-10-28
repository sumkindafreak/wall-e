/**
 * EVE companion node — ESP32-S3 (N16R8)
 *
 * ONE source tree for both builds:
 *   - Arduino IDE: open this folder (EVE_Companion.ino).
 *   - PlatformIO : eve/platformio.ini points src_dir / include_dir at this same folder.
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX to WALL-E (see config.h, section 1).
 *
 * Libraries (Library Manager): ArduinoJson 6.x; lvgl 9.2.x; GFX Library for Arduino
 * (moononournation); Adafruit NeoPixel; Adafruit INA219 + Adafruit BusIO.
 * Optional ToF: Pololu VL53L1X. The shared lib/walle_i2s_audio folder must be installed as a
 * library (copy or symlink it into your Arduino libraries folder) for SD/I2S audio.
 * Place lv_conf.h where LVGL can find it when using the LVGL 9 face (see eve/arduino_ide/README.md).
 *
 * Board: ESP32S3 Dev Module (match your N16R8 module: 16 MB flash, OPI PSRAM in Tools menu).
 *
 * Serial Monitor @115200: type "help" for the console (status, stop, clearstop, diag, ...).
 */
#include <Arduino.h>

#include "audio_control.h"
#include "battery_monitor.h"
#include "config.h"
#include "eve_attachment_manager.h"
#include "eve_behavior_manager.h"
#include "eve_desktop_companion.h"
#include "eve_serial_console.h"
#include "eve_spatial_awareness.h"
#include "eve_status_manager.h"
#include "eve_web_server.h"
#include "eyes_control.h"
#include "mic_input.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "state_machine.h"
#include "system_status.h"
#include "tof_control.h"
#include "uart_link.h"

static void onUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
  stateMachineOnUartRx(type, payload, len, seq);
}

void setup() {
  Serial.begin(115200);
  randomSeed((uint32_t)micros());
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" EVE companion (ESP32-S3)"));
  Serial.println(F(" UART hand link: Serial1"));
  Serial.println(F("========================================"));

  systemStatusInit();
  // Safety + routine hooks and the GPIO configuration check come first, so every driver below
  // starts with the stop latch and fault register already in place.
  stateMachineEarlyInit();

  eveBatteryInit();
  uartLinkInit();
  uartLinkSetRxCallback(onUartRx);

  eyesInit();
  eveSpatialAwarenessInit();
  tofInit();
  servoInit();
  neopixelInit();
  audioInit();
  initMic();
  eveAttachmentManagerInit();

  stateMachineInit();

  eveBehaviorManagerInit();
  eveStatusManagerInit();
  eveWebServerInit();
  eveSerialConsoleInit();

  Serial.print(F("[EVE] Free heap: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // Commands first: a STOP from USB or WALL-E is handled before anything else this pass.
  eveSerialConsoleTick();
  uartLinkPoll();
  stateMachineTick();

  eveAttachmentManagerTick();
  updateMic();
  eveBatteryTick();
  eveBehaviorManagerTick();
  eveStatusManagerTick();
  systemStatusTick();
  eyesTick();
  tofTick();
  servoTick();
  neopixelTick();
  audioTick();
  eveWebServerTick(stateMachineIsDocked());
}
