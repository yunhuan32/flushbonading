#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "heartbeat_task.h"

namespace {

const TickType_t HEARTBEAT_DELAY = pdMS_TO_TICKS(250);
const uint8_t HEARTBEAT_LED_PIN = 2;

}  // namespace

void heartbeatInit() {
  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  digitalWrite(HEARTBEAT_LED_PIN, LOW);
}

void heartbeatTask(void *parameter) {
  (void)parameter;

  bool ledState = false;

  for (;;) {
    ledState = !ledState;
    digitalWrite(HEARTBEAT_LED_PIN, ledState ? HIGH : LOW);
    vTaskDelay(HEARTBEAT_DELAY);
  }
}
