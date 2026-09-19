#include <Arduino.h>
#include <BluetoothSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "oled_ssd1306.h"
#include "oled.h"
#include "led.h"
#include "getNews.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ledInit();
  blueToothInit();
  oledInit();

  xTaskCreate(blueToothTask, "Bluetooth Task", 4096, NULL, 1, NULL);
  xTaskCreate(oledTask, "OLED Task", 4096, NULL, 1, NULL);
  xTaskCreate(ledTask, "LED Task", 4096, NULL, 1, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelay(pdMS_TO_TICKS(1000));
}
