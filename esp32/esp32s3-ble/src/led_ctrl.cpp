#include "led_ctrl.h"
#include "config.h"

// 板载 WS2812 是单总线彩灯，只能用它专用的时序驱动，
// 核心自带 rgbLedWrite() 就是干这个的，不需要额外装库。
// （驱动方式与参考工程 ESP32S3_Serial_StudentID 一致）
static void ledWrite(bool on) {
  if (on) {
    rgbLedWrite(RGB_BUILTIN, LED_ON_R, LED_ON_G, LED_ON_B);
  } else {
    rgbLedWrite(RGB_BUILTIN, 0, 0, 0);  // 三通道全 0 = 熄灭
  }
}

static bool s_on = false;

void ledInit() {
  ledWrite(false);  // 上电默认熄灭
}

void ledSet(bool on) {
  s_on = on;
  ledWrite(on);
}

bool ledIsOn() {
  return s_on;
}

void ledSelfTest() {
  const uint8_t seq[3][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};  // 红 绿 蓝
  for (int i = 0; i < 3; i++) {
    rgbLedWrite(RGB_BUILTIN, seq[i][0], seq[i][1], seq[i][2]);
    delay(250);
  }
  ledWrite(false);
}
