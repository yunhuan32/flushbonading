#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "oled.h"
#include "oled_ssd1306.h"

QueueHandle_t oledQueue = nullptr;
QueueHandle_t isAddQueue = nullptr;
QueueHandle_t getDutyCycleQueue = nullptr;

static OledSsd1306 oled;

void oledInit() {
  if(!oled.begin(Wire, 0x3C, SDA, SCL))
  {
    Serial.println("OLED init failed");
  }
  else
  {
    Serial.println("OLED init success");
    oled.clear();
    oled.setCursor(0, 0);
    oled.print("OLED init success");
    oled.display();
  }
  oledQueue = xQueueCreate(4, sizeof(char[32]));
  isAddQueue = xQueueCreate(4, sizeof(int));
  getDutyCycleQueue = xQueueCreate(4, sizeof(int));
  if (oledQueue == nullptr) {
    Serial.println("Failed to create OLED queue");
  }
  if (isAddQueue == nullptr) {
    Serial.println("Failed to create isAdd queue");
  }
  if (getDutyCycleQueue == nullptr) {
    Serial.println("Failed to create getDutyCycle queue");
  }
}

void oledTask(void *parameter) {
  (void)parameter;
  char message[32] = {0};
    int getAdd = 0;
    int duty = 0;
    bool updated = false;

  for (;;) {
    if (oledQueue == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (xQueueReceive(oledQueue, message, 0) == pdPASS) {
        updated = true;
    }
    if (xQueueReceive(isAddQueue, &getAdd, 0) == pdPASS) {
        updated = true;
    }
    if (xQueueReceive(getDutyCycleQueue, &duty, 0) == pdPASS) {
        updated = true;
    }
    if (updated) {
      oled.clear();
      oled.setCursor(0, 0);
      oled.print("Message: ");
      oled.println(message);
      oled.println(getAdd == 1 ? "isAdding" : "isDeleting");
      oled.println("Duty Cycle: ");
      oled.println(duty);
      oled.display();
  }

  vTaskDelay(pdMS_TO_TICKS(10));
  }
}
