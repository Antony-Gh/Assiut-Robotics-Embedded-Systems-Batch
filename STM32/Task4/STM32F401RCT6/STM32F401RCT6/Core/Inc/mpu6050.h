/**
  ******************************************************************************
  * @file           : mpu6050.h
  * @brief          : High-Performance Driver for MPU-6050 (GY-521 & GY-87)
  *                   STM32F401 HAL Driver
  ******************************************************************************
  */

#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* Register Map */
#define MPU6050_REG_XG_OFFS_TC          0x00
#define MPU6050_REG_SMPLRT_DIV          0x19
#define MPU6050_REG_CONFIG              0x1A
#define MPU6050_REG_GYRO_CONFIG         0x1B
#define MPU6050_REG_ACCEL_CONFIG        0x1C
#define MPU6050_REG_INT_PIN_CFG         0x37
#define MPU6050_REG_INT_ENABLE          0x38
#define MPU6050_REG_INT_STATUS          0x3A
#define MPU6050_REG_ACCEL_XOUT_H        0x3B
#define MPU6050_REG_TEMP_OUT_H          0x41
#define MPU6050_REG_GYRO_XOUT_H         0x43
#define MPU6050_REG_USER_CTRL           0x6A
#define MPU6050_REG_PWR_MGMT_1          0x6B
#define MPU6050_REG_PWR_MGMT_2          0x6C
#define MPU6050_REG_WHO_AM_I            0x75

#define MPU6050_WHO_AM_I_VAL            0x68

/* Accel Full Scale */
typedef enum {
    MPU6050_ACCEL_FS_2G  = 0,
    MPU6050_ACCEL_FS_4G  = 1,
    MPU6050_ACCEL_FS_8G  = 2,
    MPU6050_ACCEL_FS_16G = 3
} MPU6050_AccelFS_t;

/* Gyro Full Scale */
typedef enum {
    MPU6050_GYRO_FS_250DPS  = 0,
    MPU6050_GYRO_FS_500DPS  = 1,
    MPU6050_GYRO_FS_1000DPS = 2,
    MPU6050_GYRO_FS_2000DPS = 3
} MPU6050_GyroFS_t;

/* Digital Low Pass Filter (DLPF) */
typedef enum {
    MPU6050_DLPF_260HZ = 0,
    MPU6050_DLPF_184HZ = 1,
    MPU6050_DLPF_94HZ  = 2,
    MPU6050_DLPF_44HZ  = 3,   /* Ideal for ROVs with motor thruster noise */
    MPU6050_DLPF_21HZ  = 4,
    MPU6050_DLPF_10HZ  = 5,
    MPU6050_DLPF_5HZ   = 6
} MPU6050_DLPF_t;

/* Sensor Data Structure */
typedef struct {
    /* Raw 16-bit register values */
    int16_t raw_ax, raw_ay, raw_az;
    int16_t raw_gx, raw_gy, raw_gz;
    int16_t raw_temp;

    /* Scaled Engineering Units */
    float ax_g, ay_g, az_g;         /* Acceleration in g */
    float ax_ms2, ay_ms2, az_ms2;   /* Acceleration in m/s^2 */
    float gx_dps, gy_dps, gz_dps;   /* Angular rate in deg/s */
    float temp_c;                   /* Temperature in Celsius */

    /* Calculated Attitude (Complementary / Trig) */
    float roll_deg;
    float pitch_deg;
} MPU6050_Data_t;

/* MPU-6050 Device Handle */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
    MPU6050_AccelFS_t accel_fs;
    MPU6050_GyroFS_t  gyro_fs;
    MPU6050_DLPF_t    dlpf;

    float accel_scale;  /* LSB/g */
    float gyro_scale;   /* LSB/(deg/s) */

    /* Calibration Offsets */
    int16_t offset_ax, offset_ay, offset_az;
    int16_t offset_gx, offset_gy, offset_gz;

    MPU6050_Data_t data;
    bool is_initialized;
} MPU6050_t;

/* Public API */
HAL_StatusTypeDef MPU6050_Init(MPU6050_t *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr,
                              MPU6050_AccelFS_t accel_fs, MPU6050_GyroFS_t gyro_fs, MPU6050_DLPF_t dlpf);
HAL_StatusTypeDef MPU6050_ReadSensors(MPU6050_t *dev);
HAL_StatusTypeDef MPU6050_SetBypass(MPU6050_t *dev, bool enable);
HAL_StatusTypeDef MPU6050_CalibrateOffsets(MPU6050_t *dev, uint16_t sample_count);
HAL_StatusTypeDef MPU6050_UpdateAttitude(MPU6050_t *dev, float dt_seconds);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
