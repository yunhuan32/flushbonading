# ESP32 经典蓝牙 SPP 使用教学

## 1. 这是什么

SPP 是 Serial Port Profile，也就是“蓝牙串口”。ESP32 开启 SPP 后，手机会把它当作一个蓝牙串口设备，通过串口 App 发送文字，ESP32 就能收到。

本项目里的配置如下：

| 项目 | 值 |
| --- | --- |
| 蓝牙名称 | `light` |
| 蓝牙模式 | 经典蓝牙 SPP |
| 串口波特率 | `115200` |
| 示例控制命令 | 发送 `on` 后 LED 呼吸，发送其他内容 LED 关闭 |

相关文件：

- `getNews.h`：定义蓝牙名称和消息长度。
- `getNews.cpp`：初始化蓝牙，并在任务中读取手机发来的消息。
- `freeRTOSLight.ino`：创建蓝牙任务、OLED 任务和 LED 任务。

## 2. 硬件要求

SPP 经典蓝牙只支持带经典蓝牙模块的 ESP32。

推荐：

- ESP32-WROOM-32
- ESP32-DevKitC
- NodeMCU-32S

这些芯片不支持本项目使用的 `BluetoothSerial` SPP：

- ESP32-S2
- ESP32-S3
- ESP32-C3
- ESP32-C6
- ESP32-H2

如果开发板芯片不是原版 ESP32，手机蓝牙列表中不会出现 `light`。

## 3. 软件准备

1. 安装 Arduino IDE 或使用项目现有的 ESP32 Arduino 环境。
2. 安装 `esp32` 开发板包。
3. `BluetoothSerial` 是 ESP32 Arduino core 自带库，不需要额外安装。
4. 选择正确开发板，例如：

```text
ESP32 Dev Module
```

## 4. 上传并观察串口

1. 用 USB 数据线连接 ESP32。
2. 选择对应 COM 口。
3. 点击上传。
4. 打开串口监视器，波特率设置为 `115200`。

上传成功后，串口应显示：

```text
Bluetooth initialized
```

如果显示：

```text
An error occurred initializing Bluetooth
```

说明蓝牙初始化失败，通常需要检查：

- 是否选错开发板。
- 是否使用了不支持经典蓝牙的芯片。
- 是否刷入了旧的、编译失败前的程序。

## 5. 手机连接 ESP32

### 安卓手机

1. 打开手机系统蓝牙。
2. 进入“蓝牙设备”或“可用设备”列表。
3. 下拉刷新，等待出现 `light`。
4. 点击 `light` 配对。
5. 如果要求 PIN，通常输入：

```text
1234
```

有些 App 会在连接时提示，按提示确认即可。

### 推荐串口 App

安卓可用：

- Serial Bluetooth Terminal
- Bluetooth Terminal HC-05
- Serial Bluetooth

连接步骤：

1. 打开 App。
2. 选择“设备”。
3. 选择 `light`。
4. 连接成功。
5. 输入框发送消息，并确保发送内容带换行。

发送：

```text
on
```

预期结果：

- LED 引脚 `23` 开始 PWM 呼吸。
- OLED 显示 `on`。

发送：

```text
off
```

或发送其他任意内容：

- LED 关闭。
- OLED 显示对应文本。

## 6. 苹果手机说明

iPhone 的系统蓝牙设置通常不会显示 ESP32 的经典蓝牙 SPP 串口设备。

原因是 iOS 对通用 SPP 串口设备的支持有限，很多时候只能配合专用 App 使用。

如果你主要用 iPhone，建议：

- 改用安卓手机测试 SPP。
- 或改用支持 BLE 的方案，而不是当前 `BluetoothSerial` 的 SPP 模式。

## 7. 常见问题

### 7.1 手机搜不到 `light`

按顺序检查：

1. 串口是否显示 `Bluetooth initialized`。
2. 芯片是否为原版 ESP32，而不是 S2/S3/C3 等。
3. 手机是否打开蓝牙并手动刷新。
4. 设备是否已经配对过，是否出现在“已配对”列表中。
5. 手机与开发板距离是否太远。
6. 是否用 iPhone 搜索经典蓝牙 SPP 设备。

### 7.2 能看到设备，但连接失败

尝试：

- 删除手机里旧的 `light` 配对。
- 关闭再打开手机蓝牙。
- 重启 ESP32。
- 换一个串口 App 再试。

### 7.3 连接成功，但发送 `on` 没反应

检查：

1. App 是否已经连接到 `light`。
2. 是否发送的是小写 `on`。
3. App 是否设置为发送 `LF` 或 `NL` 换行符。
4. 串口监视器是否仍然显示蓝牙任务正常。
5. LED 引脚和 OLED 接线是否正确。

### 7.4 串口输出乱码

把串口监视器和蓝牙 App 的波特率都设置为 `115200`。

## 8. 快速连接检查清单

- [ ] 使用原版 ESP32 开发板。
- [ ] 上传成功，没有编译错误。
- [ ] 串口监视器为 `115200`。
- [ ] 串口显示 `Bluetooth initialized`。
- [ ] 安卓手机蓝牙扫描到 `light`。
- [ ] App 已连接到 `light`。
- [ ] 发送 `on` 后 LED 和 OLED 有反应。

## 9. 总结

ESP32 的经典蓝牙 SPP 适合做手机到开发板的短距离串口通信。使用要点是：

1. 必须选择支持经典蓝牙的原版 ESP32。
2. 初始化成功后会以 `light` 名称出现在安卓蓝牙列表中。
3. iPhone 通常不适合测试经典蓝牙 SPP。
4. 连接后通过串口 App 发送 `on` 即可控制本项目的 LED 和 OLED。
