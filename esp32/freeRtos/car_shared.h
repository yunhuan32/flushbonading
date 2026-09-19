#pragma once

#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct MotorCommand {
  int8_t leftPercent;
  int8_t rightPercent;
  TickType_t receivedAt;
};

// The queue is created in freeRtos.ino and used by the Bluetooth and motor tasks.
extern QueueHandle_t motorQueue;
