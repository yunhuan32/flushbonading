#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define led 23
#define freq 5000
#define resolution 8

extern QueueHandle_t ledQueue;

void ledInit();
void ledTask(void *parameter);
void PWMoutput();
