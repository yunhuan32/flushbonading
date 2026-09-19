#pragma once

// 板载 WS2812 RGB 灯的开关控制（用核心自带的 rgbLedWrite 驱动）

void ledInit();        // 上电初始化，默认熄灭
void ledSet(bool on);  // 开灯(绿色) / 关灯(熄灭)
bool ledIsOn();        // 当前开关状态
void ledSelfTest();    // 开机自检：红-绿-蓝各闪一次，用来确认灯能正常驱动
