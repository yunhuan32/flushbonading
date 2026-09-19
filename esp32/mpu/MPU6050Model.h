#pragma once

#include <Arduino.h>
#include <Wire.h>

// Range selectors for GYRO_CONFIG and ACCEL_CONFIG.
enum class MPU6050GyroRange : uint8_t {
  DPS_250 = 0,
  DPS_500 = 1,
  DPS_1000 = 2,
  DPS_2000 = 3
};

enum class MPU6050AccelRange : uint8_t {
  G_2 = 0,
  G_4 = 1,
  G_8 = 2,
  G_16 = 3
};

struct MPU6050Config {
  uint8_t address = 0x68;
  int sda = 21;
  int scl = 22;
  uint32_t clockHz = 400000;
  MPU6050GyroRange gyroRange = MPU6050GyroRange::DPS_250;
  MPU6050AccelRange accelRange = MPU6050AccelRange::G_2;
  uint8_t dlpfConfig = 3;
  uint16_t calibrationSamples = 500;
  float filterAlpha = 0.98f;
};

struct MPU6050RawData {
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  int16_t temp;
  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

struct MPU6050Reading {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
  float temperatureC;
};

class MPU6050Model {
 public:
  explicit MPU6050Model(const MPU6050Config& config = MPU6050Config())
      : config_(config) {}

  bool begin() {
    Wire.begin(config_.sda, config_.scl);
    Wire.setClock(config_.clockHz);
    delay(100);

    connected_ = detect();
    if (!connected_) {
      return false;
    }

    return configure();
  }

  bool isConnected() const {
    return connected_;
  }

  bool readRaw(MPU6050RawData& raw) {
    uint8_t bytes[14] = {0};
    if (!readBlock(REG_ACCEL_XOUT_H, bytes, sizeof(bytes))) {
      return false;
    }

    raw.accelX = combineBytes(bytes[0], bytes[1]);
    raw.accelY = combineBytes(bytes[2], bytes[3]);
    raw.accelZ = combineBytes(bytes[4], bytes[5]);
    raw.temp = combineBytes(bytes[6], bytes[7]);
    raw.gyroX = combineBytes(bytes[8], bytes[9]);
    raw.gyroY = combineBytes(bytes[10], bytes[11]);
    raw.gyroZ = combineBytes(bytes[12], bytes[13]);
    return true;
  }

  bool read(MPU6050Reading& reading) {
    MPU6050RawData raw;
    if (!readRaw(raw)) {
      return false;
    }

    reading.accelX = raw.accelX / accelScale();
    reading.accelY = raw.accelY / accelScale();
    reading.accelZ = raw.accelZ / accelScale();
    reading.gyroX = (raw.gyroX - gyroBiasX_) / gyroScale();
    reading.gyroY = (raw.gyroY - gyroBiasY_) / gyroScale();
    reading.gyroZ = (raw.gyroZ - gyroBiasZ_) / gyroScale();
    reading.temperatureC = raw.temp / 340.0f + 36.53f;
    return true;
  }

  void calibrateGyro() {
    MPU6050RawData raw;
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;
    uint16_t validSamples = 0;

    for (uint16_t i = 0; i < config_.calibrationSamples; ++i) {
      if (readRaw(raw)) {
        sumX += raw.gyroX;
        sumY += raw.gyroY;
        sumZ += raw.gyroZ;
        ++validSamples;
      }
      delay(1);
    }

    if (validSamples > 0) {
      gyroBiasX_ = sumX / validSamples;
      gyroBiasY_ = sumY / validSamples;
      gyroBiasZ_ = sumZ / validSamples;
    }
  }

  bool update(float dt) {
    if (dt <= 0.0f || dt > 0.1f) {
      return false;
    }

    if (!read(reading_)) {
      return false;
    }

    accelPitch_ =
        atan2(-reading_.accelX,
              sqrt(reading_.accelY * reading_.accelY +
                   reading_.accelZ * reading_.accelZ)) *
        180.0f / PI;
    accelRoll_ = atan2(reading_.accelY, reading_.accelZ) * 180.0f / PI;

    if (!attitudeInitialized_) {
      fusedPitch_ = accelPitch_;
      fusedRoll_ = accelRoll_;
      fusedYaw_ = 0.0f;
      attitudeInitialized_ = true;
      return true;
    }

    const float alpha = config_.filterAlpha;
    fusedPitch_ = alpha * (fusedPitch_ + reading_.gyroY * dt) +
                  (1.0f - alpha) * accelPitch_;
    fusedRoll_ = alpha * (fusedRoll_ + reading_.gyroX * dt) +
                 (1.0f - alpha) * accelRoll_;
    fusedYaw_ += reading_.gyroZ * dt;
    return true;
  }

  const MPU6050Reading& reading() const {
    return reading_;
  }

  float accelPitch() const {
    return accelPitch_;
  }

  float accelRoll() const {
    return accelRoll_;
  }

  float pitch() const {
    return fusedPitch_;
  }

  float roll() const {
    return fusedRoll_;
  }

  float yaw() const {
    return fusedYaw_;
  }

 private:
  static constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
  static constexpr uint8_t REG_SMPLRT_DIV = 0x19;
  static constexpr uint8_t REG_CONFIG = 0x1A;
  static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
  static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
  static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
  static constexpr uint8_t REG_WHO_AM_I = 0x75;

  bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(config_.address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  bool readBlock(uint8_t reg, uint8_t* data, size_t length) {
    Wire.beginTransmission(config_.address);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0) {
      return false;
    }

    size_t received =
        Wire.requestFrom(static_cast<int>(config_.address), length);
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

  bool detect() {
    uint8_t whoAmI = 0;
    if (!readBlock(REG_WHO_AM_I, &whoAmI, 1)) {
      return false;
    }
    return (whoAmI & 0x7E) == 0x68;
  }

  bool configure() {
    return writeRegister(REG_PWR_MGMT_1, 0x00) &&
           writeRegister(REG_SMPLRT_DIV, 0x00) &&
           writeRegister(REG_CONFIG, config_.dlpfConfig) &&
           writeRegister(REG_GYRO_CONFIG,
                         static_cast<uint8_t>(config_.gyroRange) << 3) &&
           writeRegister(REG_ACCEL_CONFIG,
                         static_cast<uint8_t>(config_.accelRange) << 3);
  }

  static int16_t combineBytes(uint8_t high, uint8_t low) {
    return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
  }

  float accelScale() const {
    static constexpr float scales[] = {16384.0f, 8192.0f, 4096.0f, 2048.0f};
    return scales[static_cast<uint8_t>(config_.accelRange)];
  }

  float gyroScale() const {
    static constexpr float scales[] = {131.0f, 65.5f, 32.8f, 16.4f};
    return scales[static_cast<uint8_t>(config_.gyroRange)];
  }

  MPU6050Config config_;
  MPU6050Reading reading_ = {};
  bool connected_ = false;
  bool attitudeInitialized_ = false;
  float gyroBiasX_ = 0.0f;
  float gyroBiasY_ = 0.0f;
  float gyroBiasZ_ = 0.0f;
  float accelPitch_ = 0.0f;
  float accelRoll_ = 0.0f;
  float fusedPitch_ = 0.0f;
  float fusedRoll_ = 0.0f;
  float fusedYaw_ = 0.0f;
};
