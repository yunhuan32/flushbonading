#include <Wire.h>

#define MPU6050_SDA         22
#define MPU6050_SCL         23

#define MPU6050_ADDR        0x68  // I2C 地址，AD0 接地时为 0x68，接 VCC 时为 0x69

#define REG_PWR_MGMT_1      0x6B  // 电源管理寄存器

uint8_t getByte(uint8_t reg ,int length) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(static_cast<int>(MPU6050_ADDR), length);
  return static_cast<uint8_t>(Wire.read());
}

int16_t combine(uint8_t high, uint8_t low) {
  return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
}

void awakeMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00); // 写入 0x00 唤醒 MPU6050
  Wire.endTransmission();
}

void setup() {
  Wire.begin(MPU6050_SDA, MPU6050_SCL);
  Serial.begin(115200);
  Serial.println("MPU6050 Test");
  awakeMPU6050();
}

void loop() {
    
    
    
    
    delay(1000);
}
