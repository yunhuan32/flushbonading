#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ctype.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bluetooth_task.h"
#include "car_shared.h"

namespace {

const char BLUETOOTH_NAME[] = "FreeRTOS-Car";
const TickType_t BLUETOOTH_POLL_DELAY = pdMS_TO_TICKS(20);
const int8_t FULL_SPEED_PERCENT = 100;

BluetoothSerial bluetoothSerial;
bool bluetoothWasConnected = false;

void sendMotorCommand(int8_t leftPercent, int8_t rightPercent) {
  MotorCommand command = {
    leftPercent,
    rightPercent,
    xTaskGetTickCount()
  };

  if (xQueueSend(motorQueue, &command, 0) != pdPASS) {
    Serial.println("Motor queue full; command dropped");
  }
}

}  // namespace

void bluetoothInit() {
  bluetoothSerial.begin(BLUETOOTH_NAME);
  Serial.printf("Bluetooth ready. Connect to %s\n", BLUETOOTH_NAME);
}

void bluetoothTask(void *parameter) {
  (void)parameter;

  for (;;) {
    if (!bluetoothSerial.connected()) {
      if (bluetoothWasConnected) {
        bluetoothWasConnected = false;
        Serial.println("Bluetooth disconnected; stopping car");
        sendMotorCommand(0, 0);
      }
      vTaskDelay(BLUETOOTH_POLL_DELAY);
      continue;
    }

    if (!bluetoothWasConnected) {
      bluetoothWasConnected = true;
      Serial.println("Bluetooth connected");
    }

    while (bluetoothSerial.available() > 0) {
      char input = static_cast<char>(bluetoothSerial.read());

      if (input == '\r' || input == '\n' || input == ' ') {
        continue;
      }

      switch (toupper(static_cast<unsigned char>(input))) {
        case 'F':
          sendMotorCommand(FULL_SPEED_PERCENT, FULL_SPEED_PERCENT);
          break;
        case 'B':
          sendMotorCommand(-FULL_SPEED_PERCENT, -FULL_SPEED_PERCENT);
          break;
        case 'L':
          sendMotorCommand(-FULL_SPEED_PERCENT, FULL_SPEED_PERCENT);
          break;
        case 'R':
          sendMotorCommand(FULL_SPEED_PERCENT, -FULL_SPEED_PERCENT);
          break;
        case 'S':
          sendMotorCommand(0, 0);
          break;
        default:
          Serial.printf("Ignored command: %c\n", input);
          break;
      }
    }

    vTaskDelay(BLUETOOTH_POLL_DELAY);
  }
}
