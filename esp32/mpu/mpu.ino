/*
 * ESP32 读取 MPU6050 的完整基础示例。
 *
 * 本文件只使用 Arduino 内置 Wire 库，不依赖第三方 MPU6050 库。
 * 完成的功能：
 *   1. 通过 I2C 检查 MPU6050 是否存在
 *   2. 唤醒 MPU6050 并配置加速度计和陀螺仪量程
 *   3. 采集静止样本，计算并消除陀螺仪零偏
 *   4. 连续读取 14 个数据字节并换算成 g、deg/s 和 pitch/roll
 *
 * 默认接线：
 *   ESP32 3V3 -> VCC
 *   ESP32 GND -> GND
 *   GPIO 21  -> SDA
 *   GPIO 22  -> SCL
 *   GND       -> AD0，此时 I2C 地址为 0x68
 */

#include <Wire.h>

// AD0 接 GND 时使用 0x68；接 3V3 时改成 0x69。
#define MPU6050_ADDR        0x68

// 标准 ESP32 DevKit 的默认 I2C 引脚。
#define MPU6050_SDA         22
#define MPU6050_SCL         23

// MPU6050 的常用寄存器地址。
#define REG_PWR_MGMT_1      0x6B  // 电源管理，写入 0x00 唤醒芯片
#define REG_SMPLRT_DIV      0x19  // 采样率分频器
#define REG_CONFIG          0x1A  // 数字低通滤波器配置
#define REG_GYRO_CONFIG     0x1B  // 陀螺仪量程配置
#define REG_ACCEL_CONFIG    0x1C  // 加速度计量程配置
#define REG_ACCEL_XOUT_H    0x3B  // 加速度 X 数据的高字节
#define REG_WHO_AM_I        0x75  // 芯片识别寄存器

// 量程为 ±2g 时，16384 个原始值代表 1g。
// 量程为 ±250 deg/s 时，131 个原始值代表 1 deg/s。
constexpr float ACCEL_SCALE = 16384.0f;
constexpr float GYRO_SCALE = 131.0f;

// 校准样本数和串口输出周期。
constexpr int CALIBRATION_SAMPLES = 500;
constexpr unsigned long PRINT_INTERVAL_MS = 50;

// 从 0x3B 开始一次读取的 14 个字节按此顺序解析。
struct SensorData {
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  int16_t temp;
  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

// 静止校准时得到的陀螺仪零偏，后续数据会减去这些值。
float gyroBiasX = 0.0f;
float gyroBiasY = 0.0f;
float gyroBiasZ = 0.0f;

// 写入一个寄存器。成功时 I2C endTransmission() 返回 0。
bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// 从某个寄存器开始连续读取 length 个字节。
// I2C 读操作分两步：先写寄存器地址，再请求读取数据。
bool readBlock(uint8_t reg, uint8_t *data, size_t length) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t received = Wire.requestFrom(static_cast<int>(MPU6050_ADDR), length);
  if (received < length) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t i = 0; i < length; ++i) {
    data[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

// 读取单个寄存器，读取失败时返回 0xFF。
uint8_t readRegister(uint8_t reg) {
  uint8_t value = 0;
  return readBlock(reg, &value, 1) ? value : 0xFF;
}

// MPU6050 寄存器采用大端格式，高字节在前、低字节在后。
int16_t combineBytes(uint8_t high, uint8_t low) {
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

// 从 0x3B 开始连续读取 14 字节，并解析为 7 个 16 位数据。
bool readSensor(SensorData &data) {
  uint8_t raw[14] = {0};
  if (!readBlock(REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
    return false;
  }

  data.accelX = combineBytes(raw[0], raw[1]);
  data.accelY = combineBytes(raw[2], raw[3]);
  data.accelZ = combineBytes(raw[4], raw[5]);
  data.temp = combineBytes(raw[6], raw[7]);
  data.gyroX = combineBytes(raw[8], raw[9]);
  data.gyroY = combineBytes(raw[10], raw[11]);
  data.gyroZ = combineBytes(raw[12], raw[13]);
  return true;
}

// 在传感器静止时采集多组陀螺仪数据并求平均零偏。
void calibrateGyro() {
  SensorData data;
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;
  int validSamples = 0;

  Serial.println("Calibrating gyro. Keep the sensor still...");
  for (int i = 0; i < CALIBRATION_SAMPLES; ++i) {
    if (readSensor(data)) {
      sumX += data.gyroX;
      sumY += data.gyroY;
      sumZ += data.gyroZ;
      ++validSamples;
    }
    delay(1);
  }

  if (validSamples > 0) {
    gyroBiasX = sumX / validSamples;
    gyroBiasY = sumY / validSamples;
    gyroBiasZ = sumZ / validSamples;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(MPU6050_SDA, MPU6050_SCL);
  Wire.setClock(400000);
  delay(100);

  Serial.println("MPU6050 demo");

  // WHO_AM_I 的有效位是 bit6..bit1，正常值应包含 0x68。
  uint8_t whoAmI = readRegister(REG_WHO_AM_I);
  if ((whoAmI & 0x7E) != 0x68) {
    Serial.printf("MPU6050 not found on I2C address 0x%02X. WHO_AM_I=0x%02X\n",
                  MPU6050_ADDR, whoAmI);
    while (true) {
      delay(1000);
    }
  }

  // PWR_MGMT_1 写 0x00 唤醒；
  // 其余寄存器这里分别选择默认采样率、低通滤波、±250 deg/s、±2g。
  if (!writeRegister(REG_PWR_MGMT_1, 0x00) ||
      !writeRegister(REG_SMPLRT_DIV, 0x00) ||
      !writeRegister(REG_CONFIG, 0x03) ||
      !writeRegister(REG_GYRO_CONFIG, 0x00) ||
      !writeRegister(REG_ACCEL_CONFIG, 0x00)) {
    Serial.println("Failed to configure MPU6050.");
    while (true) {
      delay(1000);
    }
  }

  delay(100);
  calibrateGyro();
  Serial.println("Calibration done.");
  Serial.println("ax/ay/az in g | gyro in deg/s | pitch/roll in degrees");
}

void loop() {
  static unsigned long lastPrint = 0;
  static unsigned long lastErrorPrint = 0;
  SensorData data;

  // 读取失败时不要连续刷屏，每秒只提示一次。
  if (!readSensor(data)) {
    unsigned long now = millis();
    if (now - lastErrorPrint >= 1000) {
      Serial.println("I2C read error. Check SDA/SCL and power wiring.");
      lastErrorPrint = now;
    }
    return;
  }

  // 将原始有符号 16 位整数换算为物理单位。
  float accelX = data.accelX / ACCEL_SCALE;
  float accelY = data.accelY / ACCEL_SCALE;
  float accelZ = data.accelZ / ACCEL_SCALE;

  float gyroX = (data.gyroX - gyroBiasX) / GYRO_SCALE;
  float gyroY = (data.gyroY - gyroBiasY) / GYRO_SCALE;
  float gyroZ = (data.gyroZ - gyroBiasZ) / GYRO_SCALE;

  // 用重力在三个轴上的分量计算倾斜角。
  float pitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0f / PI;
  float roll = atan2(accelY, accelZ) * 180.0f / PI;

  unsigned long now = millis();
  if (now - lastPrint >= PRINT_INTERVAL_MS) {
    Serial.printf(
        "ax=%7.2f ay=%7.2f az=%7.2f | "
        "gx=%7.2f gy=%7.2f gz=%7.2f | "
        "pitch=%7.1f roll=%7.1f\n",
        accelX, accelY, accelZ,
        gyroX, gyroY, gyroZ,
        pitch, roll);
    lastPrint = now;
  }
}
