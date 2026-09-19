#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "car_shared.h"
#include "motor_task.h"

namespace {

const TickType_t MOTOR_QUEUE_TIMEOUT = pdMS_TO_TICKS(250);
const TickType_t MOTOR_COMMAND_TIMEOUT = pdMS_TO_TICKS(1000);

const int8_t FULL_SPEED_PERCENT = 30;
const int8_t MOTOR_DEADBAND_PERCENT = 5;

const uint8_t RIGHT_MOTOR_PWM_PIN = 25;
const uint8_t RIGHT_MOTOR_IN1_PIN = 26;
const uint8_t RIGHT_MOTOR_IN2_PIN = 27;
const uint8_t LEFT_MOTOR_PWM_PIN = 14;
const uint8_t LEFT_MOTOR_IN1_PIN = 12;
const uint8_t LEFT_MOTOR_IN2_PIN = 13;
const uint8_t MOTOR_STANDBY_PIN = 32;

const uint8_t RIGHT_PWM_CHANNEL = 0;
const uint8_t LEFT_PWM_CHANNEL = 1;
const uint32_t PWM_FREQUENCY_HZ = 10000;
const uint8_t PWM_RESOLUTION_BITS = 8;

// Flip one of these when a wheel turns the wrong way on the car.
const bool LEFT_MOTOR_INVERTED = false;
const bool RIGHT_MOTOR_INVERTED = false;

void driveMotor(uint8_t pwmChannel,
                uint8_t in1Pin,
                uint8_t in2Pin,
                int8_t percent,
                bool inverted) {
  int8_t actualPercent = inverted ? static_cast<int8_t>(-percent) : percent;

  if (actualPercent >= -MOTOR_DEADBAND_PERCENT &&
      actualPercent <= MOTOR_DEADBAND_PERCENT) {
    ledcWriteChannel(pwmChannel, 0);
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, HIGH);
    return;
  }

  bool forward = actualPercent > 0;
  int absolutePercent = forward ? actualPercent : -actualPercent;
  uint32_t dutyCycle = map(
    absolutePercent,
    0,
    FULL_SPEED_PERCENT,
    0,
    (1 << PWM_RESOLUTION_BITS) - 1
  );

  digitalWrite(in1Pin, forward ? HIGH : LOW);
  digitalWrite(in2Pin, forward ? LOW : HIGH);
  ledcWriteChannel(pwmChannel, dutyCycle);
}

void setMotorSpeed(int8_t leftPercent, int8_t rightPercent) {
  driveMotor(
    LEFT_PWM_CHANNEL,
    LEFT_MOTOR_IN1_PIN,
    LEFT_MOTOR_IN2_PIN,
    leftPercent,
    LEFT_MOTOR_INVERTED
  );
  driveMotor(
    RIGHT_PWM_CHANNEL,
    RIGHT_MOTOR_IN1_PIN,
    RIGHT_MOTOR_IN2_PIN,
    rightPercent,
    RIGHT_MOTOR_INVERTED
  );
}

}  // namespace

void motorDriverInit() {
  pinMode(RIGHT_MOTOR_PWM_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_PWM_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(MOTOR_STANDBY_PIN, OUTPUT);

  digitalWrite(MOTOR_STANDBY_PIN, HIGH);

  ledcAttachChannel(
    RIGHT_MOTOR_PWM_PIN,
    PWM_FREQUENCY_HZ,
    PWM_RESOLUTION_BITS,
    RIGHT_PWM_CHANNEL
  );
  ledcAttachChannel(
    LEFT_MOTOR_PWM_PIN,
    PWM_FREQUENCY_HZ,
    PWM_RESOLUTION_BITS,
    LEFT_PWM_CHANNEL
  );

  setMotorSpeed(0, 0);
}

void motorTask(void *parameter) {
  (void)parameter;

  MotorCommand command;
  TickType_t lastCommandTick = xTaskGetTickCount();

  for (;;) {
    if (xQueueReceive(motorQueue, &command, MOTOR_QUEUE_TIMEOUT) == pdPASS) {
      lastCommandTick = command.receivedAt;
      setMotorSpeed(command.leftPercent, command.rightPercent);
      continue;
    }

   
  }
}
