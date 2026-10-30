/**
  ******************************************************************************
  * @file           : gy87.h
  * @brief          : 10-DOF Composite Sensor Driver for GY-87 Breakout Board
  *                   (MPU-6050 + HMC5883L/QMC5883L + BMP180)
  *                   STM32F401 HAL Driver
  ******************************************************************************
  */

#ifndef __GY87_H
#define __GY87_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpu6050.h"
#include "hmc5883l.h"
#include "bmp180.h"

/* Complete 10-DOF Unified Data Structure */
typedef struct {
    /* 6-DOF IMU from MPU-6050 */
    float ax_g, ay_g, az_g;
    float gx_dps, gy_dps, gz_dps;
    float mpu_temp_c;
    float roll_deg;
    float pitch_deg;

    /* 3-DOF Magnetometer from HMC5883L/QMC5883L */
    float mx_uT, my_uT, mz_uT;
    float heading_deg;

    /* Barometric Pressure & Temp from BMP180 */
    float pressure_hpa;
    float baro_temp_c;
    float altitude_m;
} GY87_Data_t;

/* GY-87 Board Handle */
typedef struct {
    MPU6050_t  mpu;
    HMC5883L_t mag;
    BMP180_t   baro;

    GY87_Data_t data;
    bool is_initialized;
} GY87_t;

/* Public API */
HAL_StatusTypeDef GY87_Init(GY87_t *board, I2C_HandleTypeDef *hi2c, uint8_t mpu_addr, float declination_deg);
HAL_StatusTypeDef GY87_ReadAll(GY87_t *board, float dt_seconds);

#ifdef __cplusplus
}
#endif

#endif /* __GY87_H */
