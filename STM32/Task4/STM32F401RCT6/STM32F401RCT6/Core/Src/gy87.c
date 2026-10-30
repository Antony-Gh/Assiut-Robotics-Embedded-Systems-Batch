/**
  ******************************************************************************
  * @file           : gy87.c
  * @brief          : 10-DOF Composite Driver implementation for GY-87
  *                   (MPU-6050 + HMC5883L/QMC5883L + BMP180)
  *                   STM32F401 HAL
  ******************************************************************************
  */

#include "gy87.h"

HAL_StatusTypeDef GY87_Init(GY87_t *board, I2C_HandleTypeDef *hi2c, uint8_t mpu_addr, float declination_deg) {
    board->is_initialized = false;

    /* 1. Initialize MPU-6050 (e.g., +/- 4g, +/- 500 dps, 44Hz DLPF) */
    if (MPU6050_Init(&board->mpu, hi2c, mpu_addr, MPU6050_ACCEL_FS_4G, MPU6050_GYRO_FS_500DPS, MPU6050_DLPF_44HZ) != HAL_OK) {
        return HAL_ERROR;
    }

    /* 2. Enable I2C Bypass Mode on MPU-6050 so that HMC5883L and BMP180 are exposed on the main I2C bus */
    if (MPU6050_SetBypass(&board->mpu, true) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(10);

    /* 3. Initialize Magnetometer (auto-detects HMC5883L or QMC5883L) */
    if (HMC5883L_Init(&board->mag, hi2c, declination_deg) != HAL_OK) {
        /* Continue even if magnetometer has issue, but flag could be checked */
    }

    /* 4. Initialize BMP180 Barometer */
    if (BMP180_Init(&board->baro, hi2c, BMP180_OSS_STANDARD) != HAL_OK) {
        /* Continue even if barometer has issue */
    }

    board->is_initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef GY87_ReadAll(GY87_t *board, float dt_seconds) {
    if (!board->is_initialized) return HAL_ERROR;

    /* Read MPU-6050 */
    if (MPU6050_UpdateAttitude(&board->mpu, dt_seconds) == HAL_OK) {
        board->data.ax_g       = board->mpu.data.ax_g;
        board->data.ay_g       = board->mpu.data.ay_g;
        board->data.az_g       = board->mpu.data.az_g;
        board->data.gx_dps     = board->mpu.data.gx_dps;
        board->data.gy_dps     = board->mpu.data.gy_dps;
        board->data.gz_dps     = board->mpu.data.gz_dps;
        board->data.mpu_temp_c = board->mpu.data.temp_c;
        board->data.roll_deg   = board->mpu.data.roll_deg;
        board->data.pitch_deg  = board->mpu.data.pitch_deg;
    }

    /* Read Magnetometer */
    if (HMC5883L_Read(&board->mag) == HAL_OK) {
        board->data.mx_uT       = board->mag.data.mx_uT;
        board->data.my_uT       = board->mag.data.my_uT;
        board->data.mz_uT       = board->mag.data.mz_uT;
        board->data.heading_deg = board->mag.data.heading_deg;
    }

    /* Read Barometer */
    if (BMP180_Read(&board->baro) == HAL_OK) {
        board->data.pressure_hpa = board->baro.data.pressure_hpa;
        board->data.baro_temp_c  = board->baro.data.temperature_c;
        board->data.altitude_m   = board->baro.data.altitude_m;
    }

    return HAL_OK;
}
