#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <Arduino.h>

#define SDA 5
#define SCL 18

extern QueueHandle_t oledQueue;
extern QueueHandle_t isAddQueue;
extern QueueHandle_t getDutyCycleQueue;

void oledInit();
void oledTask(void *parameter);
