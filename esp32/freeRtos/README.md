# ESP32 蓝牙小车 FreeRTOS 教学示例

这是一个基于 ESP32、TB6612FNG 和 2WD 差速小车的 Arduino 工程。项目把三个 FreeRTOS 任务拆分到不同的 `.cpp` 文件中，主文件通过头文件引用它们。

## 文件结构

```text
freeRtos/
├── freeRtos.ino       主文件：初始化、创建队列、创建三个任务
├── car_shared.h       多个文件共享的 MotorCommand 和队列声明
├── bluetooth_task.h   蓝牙任务接口
├── bluetooth_task.cpp 蓝牙任务实现
├── motor_task.h       电机任务和电机驱动接口
├── motor_task.cpp     电机任务和 TB6612FNG 驱动实现
├── heartbeat_task.h   心跳任务接口
├── heartbeat_task.cpp 心跳任务实现
└── README.md
```

### 主文件如何引用其他文件

`freeRtos.ino` 中使用引号形式的 `#include` 引用同目录下的头文件：

```cpp
#include "bluetooth_task.h"
#include "car_shared.h"
#include "heartbeat_task.h"
#include "motor_task.h"
```

规则如下：

- `#include "xxx.h"`：先查找当前 Arduino 工程目录，适合引用自己的头文件。
- `#include <xxx.h>`：查找编译器和库的安装目录，适合引用 Arduino、FreeRTOS、BluetoothSerial 等库。
- `.h` 文件通常只放函数声明和类型定义，`.cpp` 文件放函数实现。
- 主文件调用其他文件中的函数前，必须包含对应的头文件。
- 所有文件必须和 `freeRtos.ino` 放在同一个文件夹中，Arduino IDE 会自动显示为多个标签页。

`car_shared.h` 中的 `extern QueueHandle_t motorQueue;` 表示“这个队列在其他文件中定义”。它实际上由 `freeRtos.ino` 创建，蓝牙任务和电机任务通过同一个 `extern` 声明使用这个队列。

## 硬件

- ESP32 Dev Module 或兼容开发板
- TB6612FNG 电机驱动模块
- 两个直流减速电机，组成 2WD 差速底盘
- 小车电源和杜邦线

ESP32 和 TB6612FNG 必须共地。电机建议使用独立电池供电。

## 接线

| ESP32 引脚 | TB6612FNG | 说明 |
| --- | --- | --- |
| GPIO25 | PWMA | 右电机 PWM |
| GPIO26 | AIN1 | 右电机方向 1 |
| GPIO27 | AIN2 | 右电机方向 2 |
| GPIO14 | PWMB | 左电机 PWM |
| GPIO12 | BIN1 | 左电机方向 1 |
| GPIO13 | BIN2 | 左电机方向 2 |
| GPIO32 | STBY | 驱动使能 |
| GND | GND | 共地 |

TB6612FNG 的 AO1、AO2 接右电机，BO1、BO2 接左电机。VM 接电机电源，VCC 接逻辑电源。

## 蓝牙命令

连接蓝牙设备 `FreeRTOS-Car` 后发送单个字符，大小写均可：

| 命令 | 动作 |
| --- | --- |
| `F` | 前进 |
| `B` | 后退 |
| `L` | 原地左转 |
| `R` | 原地右转 |
| `S` | 停止 |

回车、换行和空格会被忽略，其他字符不会改变电机状态。

## 编译和上传

1. 在 Arduino IDE 中安装 ESP32 开发板支持。
2. 开发板选择 `ESP32 Dev Module`。
3. 打开 `freeRtos.ino`，Arduino IDE 会自动加载同目录下的 `.h` 和 `.cpp` 文件。
4. 选择正确的串口后点击上传。
5. 串口监视器波特率设置为 `115200`。
6. 使用支持 Classic Bluetooth SPP 的蓝牙串口 App 连接 `FreeRTOS-Car`。

建议第一次测试时抬起车轮，避免连接失败或方向错误时小车直接冲出桌面。

## FreeRTOS 结构

| 任务 | 文件 | 核心 | 优先级 | 栈大小 | 作用 |
| --- | --- | ---: | ---: | ---: | --- |
| Bluetooth RX | `bluetooth_task.cpp` | Core 0 | 2 | 4096 | 接收蓝牙命令并写入队列 |
| Motor Control | `motor_task.cpp` | Core 1 | 3 | 3072 | 从队列取命令并驱动电机 |
| Heartbeat | `heartbeat_task.cpp` | Core 1 | 1 | 2048 | 每 250 ms 翻转 GPIO2 上的 LED |

主要 FreeRTOS API：

- `xTaskCreatePinnedToCore`：创建任务并指定运行核心。
- `xQueueCreate`：创建电机命令队列。
- `xQueueSend`：蓝牙任务向队列发送 `MotorCommand`。
- `xQueueReceive`：电机任务从队列接收 `MotorCommand`。
- `vTaskDelay`：让任务阻塞一段时间，使其他任务获得 CPU。

Arduino 的 `loop()` 只执行 `vTaskDelay(portMAX_DELAY)`，电机和蓝牙逻辑全部由 FreeRTOS 任务完成。

## 安全机制

- 蓝牙断开时立即发送停止命令。
- 连续 1000 ms 没有收到有效命令时自动停车。
- 速度为 0 时使用 TB6612FNG 的短制动模式。
- 启动时先初始化 `STBY`，并让电机保持停止状态。

## 方向校准

如果某个轮子方向相反，修改 `motor_task.cpp` 中的常量：

```cpp
const bool LEFT_MOTOR_INVERTED = false;
const bool RIGHT_MOTOR_INVERTED = false;
```

需要反转时把对应值改为 `true`。

心跳任务默认使用 GPIO2 作为 LED 引脚。如果开发板的内置 LED 不在 GPIO2，修改 `heartbeat_task.cpp` 中的 `HEARTBEAT_LED_PIN`。

## 快速测试

1. 抬起小车，连接蓝牙。
2. 发送 `F`，两个轮子应向前转。
3. 发送 `S`，两个轮子应立即停止。
4. 发送 `L` 和 `R`，检查原地转向。
5. 断开蓝牙，确认小车在 1000 ms 内停止。
6. 观察 ESP32 内置 LED 是否持续闪烁，说明 Heartbeat 任务在运行。
