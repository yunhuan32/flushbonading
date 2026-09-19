#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "oled_ssd1306.h"

namespace {

constexpr int LED_PIN = 23;
constexpr int OLED_SDA_PIN = 5;
constexpr int OLED_SCL_PIN = 18;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint32_t OLED_CLOCK_HZ = 400000;

constexpr uint16_t PWM_MAX = 4095;
constexpr uint32_t BREATH_PERIOD_MS = 4000;
constexpr uint32_t BREATH_STEP_MS = 20;
constexpr uint32_t PWM_FREQUENCY_HZ = 5000;

constexpr uint16_t DISPLAY_SPEED_PERCENT = 100;

enum LedCommand {
  LED_CMD_ON,
  LED_CMD_OFF,
};

struct DisplayState {
  bool bluetoothConnected;
  bool breathingEnabled;
  uint16_t brightness;
  uint16_t speedPercent;
};

QueueHandle_t ledQueue = nullptr;
QueueHandle_t displayQueue = nullptr;
SemaphoreHandle_t stateMutex = nullptr;
DisplayState currentState = {false, false, PWM_MAX, DISPLAY_SPEED_PERCENT};

BluetoothSerial SerialBT;
OledSsd1306 oled;

bool equalsIgnoreCase(const char *text, const char *expected) {
  while (*text != '\0' && *expected != '\0') {
    char left = *text;
    char right = *expected;

    if (left >= 'a' && left <= 'z') {
      left = static_cast<char>(left - 'a' + 'A');
    }
    if (right >= 'a' && right <= 'z') {
      right = static_cast<char>(right - 'a' + 'A');
    }

    if (left != right) {
      return false;
    }

    ++text;
    ++expected;
  }

  return *text == '\0' && *expected == '\0';
}

void trimWhitespace(char *text) {
  char *start = text;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
    ++start;
  }

  if (*start == '\0') {
    text[0] = '\0';
    return;
  }

  char *end = start;
  while (*end != '\0') {
    ++end;
  }
  --end;

  while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
    *end = '\0';
    --end;
  }

  if (start != text) {
    size_t index = 0;
    while (start[index] != '\0') {
      text[index] = start[index];
      ++index;
    }
    text[index] = '\0';
  }
}

bool parseCommand(const char *line, LedCommand &command) {
  if (equalsIgnoreCase(line, "ON")) {
    command = LED_CMD_ON;
    return true;
  }
  if (equalsIgnoreCase(line, "OFF")) {
    command = LED_CMD_OFF;
    return true;
  }
  return false;
}

void publishDisplayState() {
  if (displayQueue == nullptr) {
    return;
  }

  DisplayState snapshot;
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  snapshot = currentState;
  xSemaphoreGive(stateMutex);

  xQueueOverwrite(displayQueue, &snapshot);
}

void setBluetoothConnected(bool connected) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState.bluetoothConnected = connected;
  xSemaphoreGive(stateMutex);
  publishDisplayState();
}

void setBreathingEnabled(bool enabled) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentState.breathingEnabled = enabled;
  xSemaphoreGive(stateMutex);
  publishDisplayState();
}

uint16_t breathDuty(uint32_t elapsedMs) {
  const float phase = (TWO_PI * static_cast<float>(elapsedMs) /
                       static_cast<float>(BREATH_PERIOD_MS)) -
                      (PI / 2.0F);
  const float value = (sinf(phase) + 1.0F) / 2.0F;
  return static_cast<uint16_t>(lroundf(value * static_cast<float>(PWM_MAX)));
}

void bluetoothTask(void *parameter) {
  (void)parameter;

  bool lastConnected = false;
  char lineBuffer[32] = {0};
  size_t lineLength = 0;

  while (!SerialBT.begin("ESP32_BreathLED")) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  for (;;) {
    const bool connected = SerialBT.connected();
    if (connected != lastConnected) {
      lastConnected = connected;
      setBluetoothConnected(connected);
    }

    if (!connected) {
      lineLength = 0;
      lineBuffer[0] = '\0';
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (SerialBT.available() == 0) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    const char incoming = static_cast<char>(SerialBT.read());
    if (incoming == '\n' || incoming == '\r') {
      if (lineLength > 0) {
        lineBuffer[lineLength] = '\0';
        trimWhitespace(lineBuffer);

        LedCommand command;
        if (parseCommand(lineBuffer, command)) {
          xQueueSend(ledQueue, &command, pdMS_TO_TICKS(100));
          if (command == LED_CMD_ON) {
            SerialBT.println("OK:ON");
          } else {
            SerialBT.println("OK:OFF");
          }
        } else {
          SerialBT.println("ERR");
        }
      }

      lineLength = 0;
      lineBuffer[0] = '\0';
      continue;
    }

    if (lineLength + 1 < sizeof(lineBuffer)) {
      lineBuffer[lineLength++] = incoming;
      lineBuffer[lineLength] = '\0';
    }
  }
}

void breathingLedTask(void *parameter) {
  (void)parameter;

  ledcAttach(LED_PIN, PWM_FREQUENCY_HZ, 12);
  ledcWrite(LED_PIN, 0);

  bool running = false;
  uint32_t elapsedMs = 0;

  for (;;) {
    LedCommand latestCommand;
    bool receivedCommand = false;

    while (xQueueReceive(ledQueue, &latestCommand, 0) == pdTRUE) {
      receivedCommand = true;
    }

    if (receivedCommand) {
      if (latestCommand == LED_CMD_ON && !running) {
        running = true;
        elapsedMs = 0;
        setBreathingEnabled(true);
      } else if (latestCommand == LED_CMD_OFF && running) {
        running = false;
        ledcWrite(LED_PIN, 0);
        setBreathingEnabled(false);
      }
    }

    if (running) {
      ledcWrite(LED_PIN, breathDuty(elapsedMs));
      elapsedMs += BREATH_STEP_MS;
      if (elapsedMs >= BREATH_PERIOD_MS) {
        elapsedMs -= BREATH_PERIOD_MS;
      }
      vTaskDelay(pdMS_TO_TICKS(BREATH_STEP_MS));
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

void oledTask(void *parameter) {
  (void)parameter;

  if (!oled.begin(Wire, OLED_ADDRESS, OLED_SDA_PIN, OLED_SCL_PIN, OLED_CLOCK_HZ)) {
    for (;;) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (oled.begin(Wire, OLED_ADDRESS, OLED_SDA_PIN, OLED_SCL_PIN, OLED_CLOCK_HZ)) {
        break;
      }
    }
  }

  oled.setTextWrap(false);

  for (;;) {
    DisplayState state;
    if (xQueueReceive(displayQueue, &state, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    oled.clear();
    oled.drawString(0, 0, state.bluetoothConnected ? "BT: Connected" : "BT: Disconnected");
    oled.drawString(0, 16, state.breathingEnabled ? "LED: ON" : "LED: OFF");
    oled.drawString(0, 32, "Speed:4s Bright:100%");
    oled.display();
  }
}

}  // namespace

void setup() {
  ledQueue = xQueueCreate(4, sizeof(LedCommand));
  displayQueue = xQueueCreate(1, sizeof(DisplayState));
  stateMutex = xSemaphoreCreateMutex();

  currentState.bluetoothConnected = false;
  currentState.breathingEnabled = false;
  currentState.brightness = PWM_MAX;
  currentState.speedPercent = DISPLAY_SPEED_PERCENT;

  if (displayQueue != nullptr) {
    xQueueOverwrite(displayQueue, &currentState);
  }

  xTaskCreatePinnedToCore(
      bluetoothTask, "bluetooth", 4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(
      breathingLedTask, "breathing", 3072, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(
      oledTask, "oled", 4096, nullptr, 1, nullptr, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
