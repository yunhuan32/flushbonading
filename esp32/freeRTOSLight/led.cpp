#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>
#include <string.h>
#include "led.h"
#include "oled.h"

QueueHandle_t ledQueue = nullptr;




void ledInit() {
  ledcAttach(led, freq, resolution);

  ledQueue = xQueueCreate(4, sizeof(char[32]));
  if (ledQueue == nullptr) {
    Serial.println("Failed to create LED queue");
  }
}

void ledTask(void *parameter) {
  (void)parameter;
  char message[32];
  bool ledOn = false;
  int dutyCycle = 0;
  int step = 1;
  for (;;) {
    
    xQueueSend(getDutyCycleQueue, &dutyCycle, portMAX_DELAY);
    xQueueSend(isAddQueue, &step, portMAX_DELAY);
    if (ledQueue == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    
   if (xQueueReceive(ledQueue, message, pdMS_TO_TICKS(10)) == pdPASS) {
      if (strcmp(message, "on") == 0) {
        ledOn = true;
        step = 1;
      } else if (strcmp(message, "off") == 0) {
        ledOn = false;
        dutyCycle = 0;
        ledcWrite(led, 0);
      }
    }

    if (ledOn) {
      ledcWrite(led, dutyCycle);

      dutyCycle += step;

      if (dutyCycle >= 255) {
        dutyCycle = 255;
        step = -1;
      } else if (dutyCycle <= 0) {
        dutyCycle = 0;
        step = 1;
      }
    }
  }
}

