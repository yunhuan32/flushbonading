#include <Arduino.h>
#include <BluetoothSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "getNews.h"
#include "oled.h"
#include "led.h"

BluetoothSerial bluetoothSerial;

void blueToothInit() {  
    
    if(!bluetoothSerial.begin(BLUETOOTH_NAME,false, true)) {
        Serial.println("An error occurred initializing Bluetooth");
    } else {
        Serial.println("Bluetooth initialized");
        Serial.print("Bluetooth MAC: ");
        Serial.println(bluetoothSerial.getBtAddressString());
    }
}    

void blueToothTask(void *parameter) {
    for (;;) {
        String message;
        if (bluetoothSerial.available()) {
            message = bluetoothSerial.readStringUntil('\n');
            message.trim(); // Remove any leading/trailing whitespace
            if (message.length() > 0) {
                char msg[32];
                message.toCharArray(msg, sizeof(msg));
                xQueueSend(oledQueue, msg, portMAX_DELAY);
                xQueueSend(ledQueue, msg, portMAX_DELAY);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
