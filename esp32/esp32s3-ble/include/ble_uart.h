#pragma once

#include <Arduino.h>

// 基于 Nordic UART Service (NUS) 的蓝牙"串口"
// 手机端用 nRF Connect / BLE 调试助手 等 APP 连接后：
//   向 RX 特征写入 ON / OFF / ID  -> 控制灯、查询学号
//   订阅 TX 特征的 notify         -> 接收 ESP32 回传的消息（按键上报的学号）

void bleInit();                    // 初始化并开始广播
bool bleConnected();               // 当前是否有设备连着
void bleNotifyText(const String& s); // 通过 TX 特征发一条消息给已连接的手机
void bleNotifyId();                // 把学号发给手机
