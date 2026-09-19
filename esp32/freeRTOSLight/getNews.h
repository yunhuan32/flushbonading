#pragma once

#include <BluetoothSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define BLUETOOTH_NAME "light"
#define BLUETOOTH_MESSAGE_LEN 64




void blueToothInit();
void blueToothTask(void *parameter);


