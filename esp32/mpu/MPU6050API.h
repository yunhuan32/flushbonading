#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "MPU6050Registers.h"

struct MPU6050SensorData {
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  int16_t temp;
  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

// 将两个大端格式字节拼接成一个有符号 16 位整数。
// 示例用法：
//   int16_t raw = mpu6050CombineBytes(high, low);
inline int16_t mpu6050CombineBytes(uint8_t high, uint8_t low) {
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

// 向寄存器写入一个字节。I2C 写入成功时返回 true。
// 示例用法：
//   bool ok = mpu6050WriteRegister(0x68, MPU6050_REG_PWR_MGMT_1, 0x00);
inline bool mpu6050WriteRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

inline bool mpu6050ReadBlock(uint8_t address, uint8_t reg, uint8_t* data,
                             size_t length);

// 从寄存器读取一个字节。读取失败时返回 0xFF。
// 示例用法：
//   uint8_t whoAmI = mpu6050ReadRegister(0x68, MPU6050_REG_WHO_AM_I);
inline uint8_t mpu6050ReadRegister(uint8_t address, uint8_t reg) {
  uint8_t value = 0;
  if (!mpu6050ReadBlock(address, reg, &value, 1)) {
    return 0xFF;
  }
  return value;
}

// 从指定寄存器开始连续读取多个字节。
// MPU6050 会自动递增寄存器指针，因此可以一次读回全部 14 个传感器字节。
// 示例用法：
//   uint8_t bytes[14];
//   bool ok = mpu6050ReadBlock(0x68, MPU6050_REG_ACCEL_XOUT_H, bytes, 14);
inline bool mpu6050ReadBlock(uint8_t address, uint8_t reg, uint8_t* data,
                             size_t length) {
  Wire.beginTransmission(address);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t received = Wire.requestFrom(static_cast<int>(address), length);
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

// 读取一个有符号 16 位寄存器对，高字节在前。
// 示例用法：
//   int16_t ax = mpu6050ReadInt16(0x68, MPU6050_REG_ACCEL_XOUT_H);
inline int16_t mpu6050ReadInt16(uint8_t address, uint8_t reg) {
  uint8_t bytes[2] = {0, 0};
  if (!mpu6050ReadBlock(address, reg, bytes, 2)) {
    return 0;
  }
  return mpu6050CombineBytes(bytes[0], bytes[1]);
}

// 从 0x3B 到 0x48 读取完整的传感器寄存器块。
// 示例用法：
//   MPU6050SensorData data;
//   if (mpu6050ReadSensor(0x68, data)) {
//     Serial.println(data.accelX);
//   }
inline bool mpu6050ReadSensor(uint8_t address, MPU6050SensorData& data) {
  uint8_t raw[14] = {0};
  if (!mpu6050ReadBlock(address, MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
    return false;
  }

  data.accelX = mpu6050CombineBytes(raw[0], raw[1]);
  data.accelY = mpu6050CombineBytes(raw[2], raw[3]);
  data.accelZ = mpu6050CombineBytes(raw[4], raw[5]);
  data.temp = mpu6050CombineBytes(raw[6], raw[7]);
  data.gyroX = mpu6050CombineBytes(raw[8], raw[9]);
  data.gyroY = mpu6050CombineBytes(raw[10], raw[11]);
  data.gyroZ = mpu6050CombineBytes(raw[12], raw[13]);
  return true;
}

// 将 MPU6050 从默认睡眠状态唤醒。
// 示例用法：
//   mpu6050WakeUp(MPU6050_DEFAULT_ADDR);
inline bool mpu6050WakeUp(uint8_t address) {
  return mpu6050WriteRegister(address, MPU6050_REG_PWR_MGMT_1, 0x00);
}

// 配置陀螺仪量程、加速度计量程和数字低通滤波器。
// 示例用法：
//   mpu6050Configure(
//     MPU6050_DEFAULT_ADDR,
//     MPU6050_GYRO_FS_250_DPS,
//     MPU6050_ACCEL_FS_2G,
//     MPU6050_DLPF_CFG_44HZ);
inline bool mpu6050Configure(uint8_t address,
                             uint8_t gyroRange = MPU6050_GYRO_FS_250_DPS,
                             uint8_t accelRange = MPU6050_ACCEL_FS_2G,
                             uint8_t dlpfConfig = MPU6050_DLPF_CFG_44HZ) {
  return mpu6050WriteRegister(address, MPU6050_REG_PWR_MGMT_1, 0x00) &&
         mpu6050WriteRegister(address, MPU6050_REG_SMPLRT_DIV, 0x00) &&
         mpu6050WriteRegister(address, MPU6050_REG_CONFIG, dlpfConfig) &&
         mpu6050WriteRegister(address, MPU6050_REG_GYRO_CONFIG,
                              gyroRange << 3) &&
         mpu6050WriteRegister(address, MPU6050_REG_ACCEL_CONFIG,
                              accelRange << 3);
}

// 返回指定量程对应的加速度计灵敏度。
// 示例用法：
//   float scale = mpu6050AccelScale(MPU6050_ACCEL_FS_2G);
inline float mpu6050AccelScale(uint8_t range) {
  switch (range) {
    case MPU6050_ACCEL_FS_4G:
      return MPU6050_ACCEL_SENSITIVITY_4G;
    case MPU6050_ACCEL_FS_8G:
      return MPU6050_ACCEL_SENSITIVITY_8G;
    case MPU6050_ACCEL_FS_16G:
      return MPU6050_ACCEL_SENSITIVITY_16G;
    default:
      return MPU6050_ACCEL_SENSITIVITY_2G;
  }
}

// 返回指定量程对应的陀螺仪灵敏度。
// 示例用法：
//   float scale = mpu6050GyroScale(MPU6050_GYRO_FS_250_DPS);
inline float mpu6050GyroScale(uint8_t range) {
  switch (range) {
    case MPU6050_GYRO_FS_500_DPS:
      return MPU6050_GYRO_SENSITIVITY_500;
    case MPU6050_GYRO_FS_1000_DPS:
      return MPU6050_GYRO_SENSITIVITY_1000;
    case MPU6050_GYRO_FS_2000_DPS:
      return MPU6050_GYRO_SENSITIVITY_2000;
    default:
      return MPU6050_GYRO_SENSITIVITY_250;
  }
}

// 采集多组静止时的陀螺仪数据，取平均值得到零偏。
// 执行此函数期间必须保持传感器静止。
// 示例用法：
//   float biasX, biasY, biasZ;
//   mpu6050CalibrateGyro(0x68, biasX, biasY, biasZ, 500);
inline bool mpu6050CalibrateGyro(uint8_t address,
                                 float& biasX,
                                 float& biasY,
                                 float& biasZ,
                                 uint16_t samples = 500) {
  MPU6050SensorData data;
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;
  uint16_t valid = 0;

  for (uint16_t i = 0; i < samples; ++i) {
    if (mpu6050ReadSensor(address, data)) {
      sumX += data.gyroX;
      sumY += data.gyroY;
      sumZ += data.gyroZ;
      ++valid;
    }
    delay(1);
  }

  if (valid == 0) {
    return false;
  }

  biasX = sumX / valid;
  biasY = sumY / valid;
  biasZ = sumZ / valid;
  return true;
}

// 将加速度计原始值按指定量程换算成 g。
// 示例用法：
//   float ax = mpu6050RawToAccelG(rawX, MPU6050_ACCEL_FS_2G);
inline float mpu6050RawToAccelG(int16_t raw, uint8_t range) {
  return raw / mpu6050AccelScale(range);
}

// 将陀螺仪原始值按指定量程换算成 deg/s，并减去校准零偏。
// 示例用法：
//   float gx = mpu6050RawToGyroDps(rawX - biasX, MPU6050_GYRO_FS_250_DPS);
inline float mpu6050RawToGyroDps(int16_t raw, uint8_t range) {
  return raw / mpu6050GyroScale(range);
}

// 将温度寄存器原始值换算成摄氏度。
// 示例用法：
//   float tempC = mpu6050RawToTempC(data.temp);
inline float mpu6050RawToTempC(int16_t raw) {
  return raw / MPU6050_TEMP_SENSITIVITY + MPU6050_TEMP_OFFSET_C;
}

// 根据以 g 为单位的加速度数据计算静态 pitch 和 roll。
// 示例用法：
//   float pitch, roll;
//   mpu6050ComputePitchRoll(ax, ay, az, pitch, roll);
inline void mpu6050ComputePitchRoll(float ax,
                                    float ay,
                                    float az,
                                    float& pitch,
                                    float& roll) {
  pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;
  roll = atan2(ay, az) * 180.0f / PI;
}
