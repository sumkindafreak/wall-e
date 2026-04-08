/**
 * EVE companion node — Arduino IDE sketch (ESP32-S3, same logic as ../ PlatformIO eve/)
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX to WALL-E (see config.h).
 *
 * Required library: ArduinoJson 6.x (Library Manager: "ArduinoJson" by Benoit Blanchon)
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

static void onUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq) {
  stateMachineOnUartRx(type, payload, len, seq);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" EVE companion (Arduino IDE build)"));
  Serial.println(F(" UART hand link: Serial1"));
  Serial.println(F("========================================"));

  systemStatusInit();
  uartLinkInit();
  uartLinkSetRxCallback(onUartRx);

  eyesInit();
  tofInit();
  servoInit();
  neopixelInit();
  audioInit();

#if EVE_PRESENT_PIN >= 0
  pinMode(EVE_PRESENT_PIN, INPUT_PULLUP);
#endif

  stateMachineInit();

  Serial.print(F("[EVE] Free heap: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  uartLinkPoll();
  stateMachineTick();
  systemStatusTick();
  eyesTick();
  tofTick();
  servoTick();
  neopixelTick();
  audioTick();
}
