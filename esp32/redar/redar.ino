#include <esp32-hal-timer.h>

#include "oled_ssd1306.h"

// HC-SR04 超声波测距 + SSD1306 OLED 显示。
// 硬件定时器每 60ms 请求一次测量。中断里只设置标志，
// 阻塞的超声波测距和 I2C 显示都放在 loop() 中执行。
namespace {

// HC-SR04 的 Trig 和 Echo 引脚。
constexpr uint8_t TRIG_PIN = 15;
constexpr uint8_t ECHO_PIN = 2;

// 距离提醒输出。LED 高电平点亮，蜂鸣器模块低电平触发。
constexpr uint8_t  light1 =  4;
constexpr uint8_t  light2 =  5;
constexpr uint8_t  light3 =  18;
constexpr uint8_t  light4 =  19;
constexpr uint8_t buzzer =  21;
constexpr uint8_t LED_PINS[4] = {light1, light2, light3, light4};
                          AZ
// SSD1306 的 I2C 连接。
constexpr uint8_t OLED_SDA = 22;
constexpr uint8_t OLED_SCL = 23;
constexpr uint8_t OLED_ADDRESS = 0x3C;

// 超声波和距离提醒相关的时间参数。
constexpr unsigned long TRIG_PULSE_US = 10;
constexpr unsigned long ECHO_TIMEOUT_US = 30000;
constexpr unsigned long ALARM_BLINK_PERIOD_MS = 160;
constexpr uint32_t MEASURE_TIMER_FREQUENCY_HZ = 1000000;
constexpr uint64_t MEASURE_INTERVAL_US = 60000;

OledSsd1306 oled;
hw_timer_t *measureTimer = nullptr;

// 定时器中断和主循环共用的标志。
portMUX_TYPE measureTimerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool measureRequested = false;

// 当前距离提醒状态。
uint8_t activeLedCount = 0;
bool blinkAlarm = false;
bool alarmOutputOn = true;
unsigned long lastAlarmToggleMs = 0;

void applyAlarmOutputs() {
  // 只点亮前面 activeLedCount 个 LED。
  for (uint8_t index = 0; index < 4; ++index) {
    const bool ledOn = alarmOutputOn && index < activeLedCount;
    digitalWrite(LED_PINS[index], ledOn ? HIGH : LOW);
  }

  digitalWrite(buzzer, blinkAlarm && alarmOutputOn ? LOW : HIGH);
}

void setDistanceAlarm(uint8_t ledCount, bool blink) {
  // 距离区间不变时保留当前闪烁相位，避免每次测量都重新计时。
  if (activeLedCount == ledCount && blinkAlarm == blink) {
    return;
  }

  activeLedCount = ledCount;
  blinkAlarm = blink;
  alarmOutputOn = true;
  lastAlarmToggleMs = millis();
  applyAlarmOutputs();
}

void updateDistanceAlarm() {
  // 最近距离报警使用非阻塞闪烁，不会影响主循环和定时器。
  if (!blinkAlarm) {
    return;
  }

  if (millis() - lastAlarmToggleMs >= ALARM_BLINK_PERIOD_MS) {
    lastAlarmToggleMs = millis();
    alarmOutputOn = !alarmOutputOn;
    applyAlarmOutputs();
  }
}

void lightNum(float distanceCm) {
  // 将厘米距离映射为对应的 LED 报警等级。
  if (distanceCm > 150.0f) {
    setDistanceAlarm(4, false);
  } else if (distanceCm >= 100.0f) {
    setDistanceAlarm(3, false);
  } else if (distanceCm >= 50.0f) {
    setDistanceAlarm(2, false);
  } else {
    setDistanceAlarm(1, true);
  }
}

void IRAM_ATTR onMeasureTimer() {
  // 中断里只设置测量请求标志，保持 ISR 足够短。
  portENTER_CRITICAL_ISR(&measureTimerMux);
  measureRequested = true;
  portEXIT_CRITICAL_ISR(&measureTimerMux);
}

void pulseTrigAndMeasure() {
  // 发送 HC-SR04 的 10us Trig 脉冲。
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(TRIG_PULSE_US);
  digitalWrite(TRIG_PIN, LOW);

  const unsigned long durationUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  // Echo 持续时间为 0 表示超时前没有收到回波。
  oled.clear();
  oled.setCursor(0, 0);

  if (durationUs == 0) {
    lightNum(999.0f);
    oled.print("No echo");
    oled.setCursor(0, 12);
    oled.print("Timeout");
  } else {
    const float distanceCm = durationUs / 58.0f;

    lightNum(distanceCm);
    oled.print("Pulse: ");
    oled.print(durationUs);
    oled.print(" us");
    oled.setCursor(0, 12);
    oled.print("Dist: ");
    oled.print(distanceCm, 1);
    oled.print(" cm");
  }

  oled.display();
}

}  // namespace

void setup() {
  // 初始化超声波引脚。
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);
  pinMode(light3, OUTPUT);
  pinMode(light4, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(light1, LOW);
  digitalWrite(light2, LOW); 
  digitalWrite(light3, LOW);
  digitalWrite(light4, LOW);
  digitalWrite(buzzer, LOW);

  digitalWrite(TRIG_PIN, LOW);

  pinMode(ECHO_PIN, INPUT);

  // 初始化 OLED 并显示启动状态。
  oled.begin(Wire, OLED_ADDRESS, OLED_SDA, OLED_SCL, 400000);
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("Ranging...");
  oled.display();

  // 新版 ESP32 定时器 API：1MHz 分辨率，60000 个计数等于 60ms。
  measureTimer = timerBegin(MEASURE_TIMER_FREQUENCY_HZ);
  timerAttachInterrupt(measureTimer, &onMeasureTimer);
  timerAlarm(measureTimer, MEASURE_INTERVAL_US, true, 0);
  timerRestart(measureTimer);
}

void loop() {
  bool shouldMeasure = false;

  // 原子地读取并清除定时器标志。
  portENTER_CRITICAL(&measureTimerMux);
  if (measureRequested) {
    measureRequested = false;
    shouldMeasure = true;
  }
  portEXIT_CRITICAL(&measureTimerMux);

  if (shouldMeasure) {
    pulseTrigAndMeasure();
  }

  // 在两次测量之间继续刷新最近距离的 LED 和蜂鸣器闪烁。
  updateDistanceAlarm();
}
