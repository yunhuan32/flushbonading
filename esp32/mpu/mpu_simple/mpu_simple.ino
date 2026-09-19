/*
 * MPU6050 最简使用示例
 *
 * 功能：
 *   1. 检查 MPU6050 是否连接成功
 *   2. 唤醒 MPU6050
 *   3. 读取三轴加速度原始数据
 *   4. 换算成 g 并通过串口打印
 *
 * 适合第一次接线时快速判断模块是否能正常通信。
 *
 * 接线：
 *   ESP32 3V3 -> VCC
 *   ESP32 GND -> GND
 *   GPIO 21  -> SDA
 *   GPIO 22  -> SCL
 *   GND       -> AD0
 */

#include <Wire.h>  // Arduino 内置的 I2C 通信库

// AD0 接 GND 时，MPU6050 的 I2C 地址是 0x68。
constexpr uint8_t MPU6050_ADDR = 0x68;

// 从指定寄存器读取 1 个字节。
// I2C 读操作需要先写寄存器地址，然后再请求读取。
uint8_t readByte(uint8_t reg) {
  // 开始向 MPU6050 发送
  Wire.beginTransmission(MPU6050_ADDR);

  // 告诉 MPU6050 接下来要读哪个寄存器
  Wire.write(reg);

  // false 表示先不发送停止位，后面还要继续读取
  Wire.endTransmission(false);

  // 请求 MPU6050 返回 1 个字节
  Wire.requestFrom(static_cast<int>(MPU6050_ADDR), 1);

  // 读取返回的字节
  return static_cast<uint8_t>(Wire.read());
}

// 从指定寄存器连续读取 2 个字节，并合并成有符号 16 位整数。
int16_t readInt16(uint8_t reg) {
  // 先发送要读取的寄存器地址
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  // 请求连续返回 2 个字节
  Wire.requestFrom(static_cast<int>(MPU6050_ADDR), 2);

  // MPU6050 数据是大端格式：
  // 第一个字节是高位，第二个字节是低位
  uint8_t high = Wire.read();
  uint8_t low = Wire.read();

  // 先把高位移动到高 8 位，再与低位组合
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

void setup() {
  // 初始化串口，监视器要设置成 115200
  Serial.begin(115200);

  // ESP32 使用 GPIO21 作为 SDA，GPIO22 作为 SCL
  Wire.begin(21, 22);

  // I2C 时钟设置为 400kHz
  Wire.setClock(400000);

  // 等待 I2C 稳定
  delay(100);

  // 0x75 是 WHO_AM_I 识别寄存器。
  // MPU6050 返回值的有效位应包含 0x68。
  if ((readByte(0x75) & 0x7E) != 0x68) {
    Serial.println("MPU6050 not found");

    // 找不到设备时停在原地，避免继续读取无效数据
    while (true) {
      delay(1000);
    }
  }

  // 0x6B 是电源管理寄存器。
  // 写入 0x00 会清除睡眠位，唤醒 MPU6050。
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  // 等待 MPU6050 完成唤醒
  delay(100);

  Serial.println("MPU6050 simple test");
  Serial.println("accelX accelY accelZ in g");
}

void loop() {
  // 0x3B 是加速度 X 轴高字节地址。
  // 每次读取 2 字节，得到对应轴的原始数据。
  int16_t rawX = readInt16(0x3B);

  // 0x3D 是加速度 Y 轴高字节地址
  int16_t rawY = readInt16(0x3D);

  // 0x3F 是加速度 Z 轴高字节地址
  int16_t rawZ = readInt16(0x3F);

  // MPU6050 默认加速度量程是 ±2g。
  // 此时 16384 个原始值代表 1g。
  float accelX = rawX / 16384.0f;
  float accelY = rawY / 16384.0f;
  float accelZ = rawZ / 16384.0f;

  // 每行打印 X、Y、Z 三个轴的加速度，单位为 g。
  // 模块平放时，accelZ 通常接近 1.00。
  Serial.printf("%6.2f %6.2f %6.2f\n", accelX, accelY, accelZ);

  // 每 100ms 读取并输出一次
  delay(100);
}
