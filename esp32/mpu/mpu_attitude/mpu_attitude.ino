/*
 * ESP32 + MPU6050 互补滤波姿态解算示例。
 *
 * 本示例在基础读取代码上增加姿态估计：
 *   加速度计提供长期稳定但容易受运动干扰的 pitch/roll
 *   陀螺仪提供短期快速、容易随时间漂移的角速度
 *   互补滤波器把两者融合，得到更稳定的姿态角
 *
 * 默认接线：
 *   ESP32 3V3 -> VCC
 *   ESP32 GND -> GND
 *   GPIO 21  -> SDA
 *   GPIO 22  -> SCL
 *   GND       -> AD0，此时 I2C 地址为 0x68
 */

#include <Wire.h>

// AD0 接 GND 时为 0x68；接 3V3 时为 0x69。
#define MPU6050_ADDR        0x68
#define MPU6050_SDA         21
#define MPU6050_SCL         22

// MPU6050 寄存器地址。
#define REG_PWR_MGMT_1      0x6B  // 电源管理
#define REG_SMPLRT_DIV      0x19  // 采样率分频
#define REG_CONFIG          0x1A  // 数字低通滤波器
#define REG_GYRO_CONFIG     0x1B  // 陀螺仪量程
#define REG_ACCEL_CONFIG    0x1C  // 加速度计量程
#define REG_ACCEL_XOUT_H    0x3B  // 传感器数据起始地址
#define REG_WHO_AM_I        0x75  // 芯片识别寄存器

// 当前量程对应的原始值换算系数。
constexpr float ACCEL_SCALE = 16384.0f;  // ±2g
constexpr float GYRO_SCALE = 131.0f;     // ±250 deg/s
constexpr int CALIBRATION_SAMPLES = 500;

// 姿态更新频率固定为 100Hz，串口每 50ms 输出一次。
constexpr unsigned long UPDATE_INTERVAL_US = 10000;
constexpr unsigned long PRINT_INTERVAL_MS = 50;

// 互补滤波系数：陀螺仪占 98%，加速度计占 2%。
constexpr float FILTER_ALPHA = 0.98f;

struct SensorData {
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  int16_t temp;
  int16_t gyroX;
  int16_t gyroY;
  int16_t gyroZ;
};

// 静止校准时测出的陀螺仪零偏。
float gyroBiasX = 0.0f;
float gyroBiasY = 0.0f;
float gyroBiasZ = 0.0f;

// accel* 是加速度计单独算出的角度；
// fused* 是互补滤波后的最终姿态角。
float accelPitch = 0.0f;
float accelRoll = 0.0f;
float fusedPitch = 0.0f;
float fusedRoll = 0.0f;

// 没有磁力计时，yaw 只能靠陀螺仪积分，因此会逐渐漂移。
float fusedYaw = 0.0f;
bool attitudeInitialized = false;
unsigned long lastUpdateUs = 0;

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

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

uint8_t readRegister(uint8_t reg) {
  uint8_t value = 0;
  return readBlock(reg, &value, 1) ? value : 0xFF;
}

int16_t combineBytes(uint8_t high, uint8_t low) {
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

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

// 用一次读取的数据更新加速度角度和滤波后的姿态。
void updateAttitude(const SensorData &data, float dt) {
  float accelX = data.accelX / ACCEL_SCALE;
  float accelY = data.accelY / ACCEL_SCALE;
  float accelZ = data.accelZ / ACCEL_SCALE;

  float gyroX = (data.gyroX - gyroBiasX) / GYRO_SCALE;
  float gyroY = (data.gyroY - gyroBiasY) / GYRO_SCALE;
  float gyroZ = (data.gyroZ - gyroBiasZ) / GYRO_SCALE;

  accelPitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) *
               180.0f / PI;
  accelRoll = atan2(accelY, accelZ) * 180.0f / PI;

  // 第一帧用加速度计初始化，避免姿态从 0 突然跳到真实角度。
  if (!attitudeInitialized) {
    fusedPitch = accelPitch;
    fusedRoll = accelRoll;
    fusedYaw = 0.0f;
    attitudeInitialized = true;
    return;
  }

  // 陀螺仪积分提供快速响应，加速度计提供缓慢但稳定的修正。
  fusedPitch = FILTER_ALPHA * (fusedPitch + gyroY * dt) +
               (1.0f - FILTER_ALPHA) * accelPitch;
  fusedRoll = FILTER_ALPHA * (fusedRoll + gyroX * dt) +
              (1.0f - FILTER_ALPHA) * accelRoll;

  // 仅靠 MPU6050 无法得到绝对 yaw，只能不断积分。
  fusedYaw += gyroZ * dt;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(MPU6050_SDA, MPU6050_SCL);
  Wire.setClock(400000);
  delay(100);

  Serial.println("MPU6050 complementary filter attitude demo");

  uint8_t whoAmI = readRegister(REG_WHO_AM_I);
  if ((whoAmI & 0x7E) != 0x68) {
    Serial.printf("MPU6050 not found on I2C address 0x%02X. WHO_AM_I=0x%02X\n",
                  MPU6050_ADDR, whoAmI);
    while (true) {
      delay(1000);
    }
  }

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
  lastUpdateUs = micros();
  Serial.println("Calibration done.");
  Serial.println("fusedPitch / fusedRoll / yaw in degrees");
}

void loop() {
  static unsigned long lastPrint = 0;
  static unsigned long lastErrorPrint = 0;

  unsigned long nowUs = micros();
  // 保持 100Hz 的固定更新频率，避免 dt 忽大忽小。
  if (nowUs - lastUpdateUs < UPDATE_INTERVAL_US) {
    return;
  }

  float dt = static_cast<float>(nowUs - lastUpdateUs) / 1000000.0f;
  lastUpdateUs = nowUs;

  SensorData data;
  if (!readSensor(data)) {
    unsigned long now = millis();
    if (now - lastErrorPrint >= 1000) {
      Serial.println("I2C read error. Check SDA/SCL and power wiring.");
      lastErrorPrint = now;
    }
    return;
  }

  // 保护异常间隔，防止偶然的 I2C 卡顿造成角度突变。
  if (dt > 0.0f && dt < 0.1f) {
    updateAttitude(data, dt);
  }

  unsigned long now = millis();
  if (now - lastPrint >= PRINT_INTERVAL_MS) {
    Serial.printf(
        "fusedPitch=%7.1f fusedRoll=%7.1f yaw=%7.1f | "
        "accelPitch=%7.1f accelRoll=%7.1f\n",
        fusedPitch, fusedRoll, fusedYaw,
        accelPitch, accelRoll);
    lastPrint = now;
  }
}
