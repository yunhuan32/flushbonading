#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bluetooth_task.h"
#include "car_shared.h"
#include "heartbeat_task.h"
#include "motor_task.h"

static const size_t MOTOR_QUEUE_LENGTH = 4;

QueueHandle_t motorQueue = nullptr;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  motorDriverInit();
  heartbeatInit();

  motorQueue = xQueueCreate(MOTOR_QUEUE_LENGTH, sizeof(MotorCommand));
  if (motorQueue == nullptr) {
    Serial.println("Failed to create motor command queue");
    return;
  }

  bluetoothInit();

  BaseType_t bluetoothTaskCreated = xTaskCreatePinnedToCore(
    bluetoothTask,
    "Bluetooth RX",
    4096,
    nullptr,
    2,
    nullptr,
    0
  );
  BaseType_t motorTaskCreated = xTaskCreatePinnedToCore(
    motorTask,
    "Motor Control",
    3072,
    nullptr,
    3,
    nullptr,
    1
  );
  BaseType_t heartbeatTaskCreated = xTaskCreatePinnedToCore(
    heartbeatTask,
    "Heartbeat",
    2048,
    nullptr,
    1,
    nullptr,
    1
  );

  if (bluetoothTaskCreated != pdPASS ||
      motorTaskCreated != pdPASS ||
      heartbeatTaskCreated != pdPASS) {
    Serial.println("Failed to create one or more FreeRTOS tasks");
  } else {
    Serial.println("FreeRTOS tasks running");
  }
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
