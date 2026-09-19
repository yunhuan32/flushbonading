/* ============================================================
 *  ESP32-S3 蓝牙(BLE)控制
 *  · 手机/PC 发 ON / OFF  -> 控制板载 RGB 灯
 *  · 按下板载 BOOT 键     -> 蓝牙回传学号
 *
 *  引脚与灯驱动方式已对齐参考工程 ESP32S3_Serial_StudentID：
 *  板载 WS2812 灯在 GPIO48，用核心自带 rgbLedWrite() 驱动。
 * ============================================================ */

#include <Arduino.h>

#include "config.h"
#include "led_ctrl.h"
#include "ble_uart.h"

// ---------------- 按键消抖 ----------------
static bool     s_rawLast    = HIGH;  // 上一次读到的原始电平
static bool     s_stable     = HIGH;  // 消抖后确认的稳定电平（HIGH = 没按）
static uint32_t s_rawChanged = 0;     // 原始电平最后一次变化的时刻

static void handleButton() {
  bool     raw = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  if (raw != s_rawLast) {          // 电平刚变化，重新计时
    s_rawLast    = raw;
    s_rawChanged = now;
    return;
  }
  if (now - s_rawChanged < BUTTON_DEBOUNCE_MS) return;  // 还在抖动窗口内
  if (raw == s_stable) return;     // 没有新变化

  s_stable = raw;
  if (raw == LOW) {                // 确认按下
    Serial.printf("[KEY] BOOT 键按下 -> 上报学号 %s\n", STUDENT_ID);
    bleNotifyId();
  }
}

// ---------------- 初始化 ----------------
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("========== ESP32-S3 蓝牙控制 ==========");
  Serial.printf("学号     : %s\n", STUDENT_ID);
  Serial.printf("蓝牙名   : %s\n", BLE_DEVICE_NAME);
  Serial.printf("RGB 灯脚 : GPIO%d （核心 RGB_BUILTIN 宏）\n", RGB_BUILTIN);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ledInit();
  Serial.println("[LED] 自检：红-绿-蓝各闪一次……");
  ledSelfTest();

  bleInit();

  Serial.println("--------------------------------------");
  Serial.println("手机操作：");
  Serial.println("  1) 用 nRF Connect 连接上面的蓝牙名");
  Serial.println("  2) 订阅 TX 特征的通知，才能收到回传消息");
  Serial.println("  3) 向 RX 特征写入 ON / OFF 控制灯，写入 ID 查询学号");
  Serial.println("  4) 按下板上 BOOT 键 -> 手机会收到学号");
  Serial.println("======================================");
}

void loop() {
  handleButton();
  delay(10);
}
