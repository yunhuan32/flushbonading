#pragma once

// MPU6050 寄存器映射表
#define MPU6050_REG_SELF_TEST_X        0x0D
#define MPU6050_REG_SELF_TEST_Y        0x0E
#define MPU6050_REG_SELF_TEST_Z        0x0F
#define MPU6050_REG_SELF_TEST_A        0x10
#define MPU6050_REG_SMPLRT_DIV         0x19
#define MPU6050_REG_CONFIG             0x1A
#define MPU6050_REG_GYRO_CONFIG        0x1B
#define MPU6050_REG_ACCEL_CONFIG       0x1C
#define MPU6050_REG_FIFO_EN            0x23
#define MPU6050_REG_INT_PIN_CFG        0x37
#define MPU6050_REG_INT_ENABLE         0x38
#define MPU6050_REG_ACCEL_XOUT_H       0x3B
#define MPU6050_REG_TEMP_OUT_H         0x41
#define MPU6050_REG_GYRO_XOUT_H        0x43
#define MPU6050_REG_USER_CTRL          0x6A
#define MPU6050_REG_PWR_MGMT_1         0x6B
#define MPU6050_REG_PWR_MGMT_2         0x6C
#define MPU6050_REG_WHO_AM_I           0x75

// I2C 地址由 AD0 引脚决定。
#define MPU6050_ADDR_AD0_LOW           0x68
#define MPU6050_ADDR_AD0_HIGH          0x69
#define MPU6050_DEFAULT_ADDR           MPU6050_ADDR_AD0_LOW

// WHO_AM_I 返回 MPU6050 固定的芯片 ID。
#define MPU6050_WHO_AM_I_VALUE         0x68

// 量程选择值。写入配置寄存器前需要左移 3 位。
#define MPU6050_GYRO_FS_250_DPS        0
#define MPU6050_GYRO_FS_500_DPS        1
#define MPU6050_GYRO_FS_1000_DPS       2
#define MPU6050_GYRO_FS_2000_DPS       3

#define MPU6050_ACCEL_FS_2G            0
#define MPU6050_ACCEL_FS_4G            1
#define MPU6050_ACCEL_FS_8G            2
#define MPU6050_ACCEL_FS_16G           3

// CONFIG 寄存器常用的数字低通滤波器设置。
#define MPU6050_DLPF_CFG_260HZ         0
#define MPU6050_DLPF_CFG_184HZ         1
#define MPU6050_DLPF_CFG_94HZ          2
#define MPU6050_DLPF_CFG_44HZ          3
#define MPU6050_DLPF_CFG_21HZ          4
#define MPU6050_DLPF_CFG_10HZ          5
#define MPU6050_DLPF_CFG_5HZ           6

// 灵敏度，单位是 LSB 每物理量。
#define MPU6050_ACCEL_SENSITIVITY_2G   16384.0f
#define MPU6050_ACCEL_SENSITIVITY_4G   8192.0f
#define MPU6050_ACCEL_SENSITIVITY_8G   4096.0f
#define MPU6050_ACCEL_SENSITIVITY_16G  2048.0f

#define MPU6050_GYRO_SENSITIVITY_250   131.0f
#define MPU6050_GYRO_SENSITIVITY_500   65.5f
#define MPU6050_GYRO_SENSITIVITY_1000  32.8f
#define MPU6050_GYRO_SENSITIVITY_2000  16.4f

#define MPU6050_TEMP_SENSITIVITY       340.0f
#define MPU6050_TEMP_OFFSET_C          36.53f
