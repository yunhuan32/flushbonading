# MPU6050 API 速查与详解

本教程中的示例没有依赖第三方 MPU6050 库，而是直接使用 ESP32 的 I2C API 和 MPU6050 的寄存器地址。因此“MPU6050 的 API”需要分成两层理解：

1. **硬件寄存器 API**：直接读写 MPU6050 内部寄存器。
2. **Arduino 库 API**：使用 Adafruit、MPU6050_light 等库时调用的函数。

对应示例：

- 基础寄存器读取：[mpu.ino](D:/32_practice/esp32/mpu/mpu.ino)
- 互补滤波姿态解算：[mpu_attitude.ino](D:/32_practice/esp32/mpu/mpu_attitude/mpu_attitude.ino)

## 1. 本项目使用的 I2C API

这些函数来自 Arduino/ESP32 自带的 `Wire` 库。

| 函数 | 作用 |
|---|---|
| `Wire.begin(sda, scl)` | 初始化 I2C，并指定 SDA、SCL 引脚 |
| `Wire.setClock(frequency)` | 设置 I2C 时钟频率，例如 100000 或 400000 |
| `Wire.beginTransmission(address)` | 开始向某个 I2C 地址发送数据 |
| `Wire.write(byte)` | 写入一个字节 |
| `Wire.endTransmission(stop)` | 结束发送；`false` 表示暂不发送停止位 |
| `Wire.requestFrom(address, quantity)` | 从设备请求指定数量的字节 |
| `Wire.available()` | 当前可读取的字节数 |
| `Wire.read()` | 读取一个字节 |

MPU6050 的读操作分为两步：

```text
第一步：写寄存器起始地址
第二步：连续读取 N 个字节
```

本项目把这两步封装成了：

```cpp
bool writeRegister(uint8_t reg, uint8_t value);
bool readBlock(uint8_t reg, uint8_t *data, size_t length);
uint8_t readRegister(uint8_t reg);
bool readSensor(SensorData &data);
void calibrateGyro();
```

姿态解算示例还额外使用：

```cpp
void updateAttitude(const SensorData &data, float dt);
```

## 2. MPU6050 常用寄存器 API

### 2.1 核心寄存器表

| 寄存器名称 | 地址 | 方向 | 作用 |
|---|---:|---|---|
| `SELF_TEST_X` | 0x0D | 读写 | X 轴自检 |
| `SELF_TEST_Y` | 0x0E | 读写 | Y 轴自检 |
| `SELF_TEST_Z` | 0x0F | 读写 | Z 轴自检 |
| `SELF_TEST_A` | 0x10 | 读写 | 加速度计自检 |
| `SMPLRT_DIV` | 0x19 | 读写 | 采样率分频 |
| `CONFIG` | 0x1A | 读写 | 数字低通滤波器配置 |
| `GYRO_CONFIG` | 0x1B | 读写 | 陀螺仪量程 |
| `ACCEL_CONFIG` | 0x1C | 读写 | 加速度计量程 |
| `FIFO_EN` | 0x23 | 读写 | FIFO 使能 |
| `INT_PIN_CFG` | 0x37 | 读写 | 中断引脚配置 |
| `INT_ENABLE` | 0x38 | 读写 | 中断源使能 |
| `ACCEL_XOUT_H` | 0x3B | 只读 | 加速度 X 高字节 |
| `TEMP_OUT_H` | 0x41 | 只读 | 温度高字节 |
| `GYRO_XOUT_H` | 0x43 | 只读 | 陀螺仪 X 高字节 |
| `SIGNAL_PATH_RESET` | 0x68 | 只写 | 复位信号路径 |
| `PWR_MGMT_1` | 0x6B | 读写 | 电源管理 1 |
| `PWR_MGMT_2` | 0x6C | 读写 | 电源管理 2 |
| `WHO_AM_I` | 0x75 | 只读 | 设备识别 |

### 2.2 电源管理寄存器 `PWR_MGMT_1`

地址：`0x6B`

| 位 | 名称 | 说明 |
|---:|---|---|
| bit7 | `DEVICE_RESET` | 写 1 时复位整个设备 |
| bit6 | `SLEEP` | 写 1 进入睡眠，写 0 唤醒 |
| bit5 | `CYCLE` | 低功耗循环模式 |
| bit3 | `TEMP_DIS` | 写 1 禁用温度传感器 |
| bit2..0 | `CLKSEL` | 时钟源选择 |

唤醒 MPU6050 的标准写法：

```cpp
writeRegister(0x6B, 0x00);
```

### 2.3 陀螺仪量程寄存器 `GYRO_CONFIG`

地址：`0x1B`

`FS_SEL` 位于 bit4..bit3：

| FS_SEL | 量程 | 灵敏度 |
|---|---:|---:|
| 0 | ±250 deg/s | 131 LSB/(deg/s) |
| 1 | ±500 deg/s | 65.5 LSB/(deg/s) |
| 2 | ±1000 deg/s | 32.8 LSB/(deg/s) |
| 3 | ±2000 deg/s | 16.4 LSB/(deg/s) |

本示例使用：

```cpp
writeRegister(0x1B, 0x00);  // ±250 deg/s
```

### 2.4 加速度计量程寄存器 `ACCEL_CONFIG`

地址：`0x1C`

`AFS_SEL` 位于 bit4..bit3：

| AFS_SEL | 量程 | 灵敏度 |
|---|---:|---:|
| 0 | ±2g | 16384 LSB/g |
| 1 | ±4g | 8192 LSB/g |
| 2 | ±8g | 4096 LSB/g |
| 3 | ±16g | 2048 LSB/g |

本示例使用：

```cpp
writeRegister(0x1C, 0x00);  // ±2g
```

### 2.5 数字低通滤波器 `CONFIG`

地址：`0x1A`

低通滤波器可以降低高频噪声。常用配置如下：

| DLPF_CFG | 加速度计带宽 | 陀螺仪带宽 |
|---:|---:|---:|
| 0 | 260 Hz | 256 Hz |
| 1 | 184 Hz | 188 Hz |
| 2 | 94 Hz | 98 Hz |
| 3 | 44 Hz | 42 Hz |
| 4 | 21 Hz | 20 Hz |
| 5 | 10 Hz | 10 Hz |
| 6 | 5 Hz | 5 Hz |

本示例写入 `0x03`：

```cpp
writeRegister(0x1A, 0x03);
```

### 2.6 采样率

MPU6050 的采样率由 `SMPLRT_DIV` 决定：

```text
采样率 = 陀螺仪输出频率 / (1 + SMPLRT_DIV)
```

陀螺仪输出频率通常是：

- 低通滤波器关闭时：8 kHz
- 低通滤波器启用时：1 kHz

例如本示例：

```text
SMPLRT_DIV = 0
低通滤波器 = 3
采样率 = 1000 / (1 + 0) = 1000 Hz
```

### 2.7 传感器数据寄存器

从 `0x3B` 开始可以连续读取 14 个字节：

| 地址 | 名称 | 内容 |
|---|---:|---|
| 0x3B | `ACCEL_XOUT_H` | 加速度 X 高字节 |
| 0x3C | `ACCEL_XOUT_L` | 加速度 X 低字节 |
| 0x3D | `ACCEL_YOUT_H` | 加速度 Y 高字节 |
| 0x3E | `ACCEL_YOUT_L` | 加速度 Y 低字节 |
| 0x3F | `ACCEL_ZOUT_H` | 加速度 Z 高字节 |
| 0x40 | `ACCEL_ZOUT_L` | 加速度 Z 低字节 |
| 0x41 | `TEMP_OUT_H` | 温度高字节 |
| 0x42 | `TEMP_OUT_L` | 温度低字节 |
| 0x43 | `GYRO_XOUT_H` | 陀螺仪 X 高字节 |
| 0x44 | `GYRO_XOUT_L` | 陀螺仪 X 低字节 |
| 0x45 | `GYRO_YOUT_H` | 陀螺仪 Y 高字节 |
| 0x46 | `GYRO_YOUT_L` | 陀螺仪 Y 低字节 |
| 0x47 | `GYRO_ZOUT_H` | 陀螺仪 Z 高字节 |
| 0x48 | `GYRO_ZOUT_L` | 陀螺仪 Z 低字节 |

每个数值都是高位在前的有符号 16 位整数：

```cpp
int16_t raw = (int16_t)((uint16_t)high << 8 | low);
```

### 2.8 原始值换算

本示例配置为 ±2g、±250 deg/s：

```text
accel_g = accel_raw / 16384
gyro_dps = (gyro_raw - gyro_bias) / 131
temperature_c = temp_raw / 340 + 36.53
```

## 3. 常用 Arduino 库 API

如果不想自己读写寄存器，可以使用以下库。

### 3.1 Adafruit MPU6050

需要安装：

```text
Adafruit MPU6050
Adafruit Unified Sensor
```

基本用法：

```cpp
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

void setup() {
  Serial.begin(115200);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    while (true);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  mpu.getEvent(&a, &g, &temp);

  Serial.printf(
      "accel=%.2f %.2f %.2f m/s^2 | gyro=%.2f %.2f %.2f rad/s\n",
      a.acceleration.x, a.acceleration.y, a.acceleration.z,
      g.gyro.x, g.gyro.y, g.gyro.z);

  delay(50);
}
```

常用方法：

| 方法 | 作用 |
|---|---|
| `mpu.begin()` | 初始化，成功返回 `true` |
| `mpu.setAccelerometerRange(range)` | 设置加速度量程 |
| `mpu.setGyroRange(range)` | 设置陀螺仪量程 |
| `mpu.setFilterBandwidth(bandwidth)` | 设置数字低通滤波带宽 |
| `mpu.getEvent(&a, &g, &temp)` | 一次读取全部事件 |
| `mpu.getAccelerometerSensor()` | 获取加速度传感器对象 |
| `mpu.getGyroSensor()` | 获取陀螺仪传感器对象 |
| `mpu.getTemperatureSensor()` | 获取温度传感器对象 |

Adafruit 库的加速度单位是 `m/s^2`，陀螺仪单位是 `rad/s`。

### 3.2 MPU6050_light

需要安装：

```text
MPU6050_light
```

它更适合直接获取姿态角：

```cpp
#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (mpu.begin() == 0) {
    Serial.println("MPU6050 not found");
    while (true);
  }

  mpu.calcOffsets();
}

void loop() {
  mpu.update();

  float pitch = mpu.getAngleX();
  float roll = mpu.getAngleY();
  float yaw = mpu.getAngleZ();

  Serial.printf("pitch=%.2f roll=%.2f yaw=%.2f\n", pitch, roll, yaw);
  delay(20);
}
```

常用方法：

| 方法 | 作用 |
|---|---|
| `mpu.begin()` | 初始化 |
| `mpu.calcOffsets()` | 自动计算陀螺仪零偏 |
| `mpu.update()` | 更新姿态，需要在循环中频繁调用 |
| `mpu.getAngleX()` | 获取 pitch |
| `mpu.getAngleY()` | 获取 roll |
| `mpu.getAngleZ()` | 获取 yaw |
| `mpu.getAccX/Y/Z()` | 获取加速度 |
| `mpu.getGyroX/Y/Z()` | 获取角速度 |
| `mpu.getTemp()` | 获取温度 |

注意：没有磁力计时，`getAngleZ()` 仍然会漂移。

## 4. 从寄存器到姿态角的完整调用流程

```text
Wire.begin()
  -> 写 0x6B = 0x00
  -> 写 0x1A = 0x03
  -> 写 0x1B = 0x00
  -> 写 0x1C = 0x00
  -> 静止采集 500 次，计算 gyroBias
  -> 循环读取 0x3B 开始的 14 字节
  -> 将原始值换算成物理量
  -> 加速度计计算 accelPitch/accelRoll
  -> 陀螺仪按 dt 积分
  -> 互补滤波融合
  -> 串口输出
```

## 5. 快速故障对照

| 现象 | 优先检查 |
|---|---|
| 读不到 `WHO_AM_I` | I2C 地址、SDA/SCL、VCC/GND |
| `WHO_AM_I` 不是 0x68 | 可能是模块差异或 I2C 接线问题 |
| 数据始终为 0 | 芯片是否仍处于睡眠模式 |
| 陀螺仪静止时漂移 | 是否在运动时完成校准 |
| 姿态运动时抖动 | 加速度计角度受运动加速度干扰 |
| yaw 不停漂移 | MPU6050 没有磁力计，需要增加磁力计 |

## 6. 推荐记忆的核心寄存器

```text
0x6B = PWR_MGMT_1
0x1A = CONFIG
0x1B = GYRO_CONFIG
0x1C = ACCEL_CONFIG
0x3B = ACCEL_XOUT_H
0x41 = TEMP_OUT_H
0x43 = GYRO_XOUT_H
0x75 = WHO_AM_I
```

先掌握这些寄存器，再使用 Adafruit 或 MPU6050_light 库，就能理解库函数内部实际做了什么。
