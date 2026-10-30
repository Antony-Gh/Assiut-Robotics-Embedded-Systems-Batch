/**
  ******************************************************************************
  * @file           : mpu6050.c
  * @brief          : High-Performance Driver implementation for MPU-6050
  *                   STM32F401 HAL
  ******************************************************************************
  */

#include "mpu6050.h"
#include "logger.h"
#include <math.h>

#define RAD_TO_DEG 57.29577951308232f

static HAL_StatusTypeDef MPU6050_WriteRegister(MPU6050_t *dev, uint8_t reg, uint8_t data) {
    return HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 50);
}

static HAL_StatusTypeDef MPU6050_ReadRegisters(MPU6050_t *dev, uint8_t reg, uint8_t *buffer, uint16_t length) {
    return HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, buffer, length, 50);
}

HAL_StatusTypeDef MPU6050_Init(MPU6050_t *dev, I2C_HandleTypeDef *hi2c, uint8_t i2c_addr,
                              MPU6050_AccelFS_t accel_fs, MPU6050_GyroFS_t gyro_fs, MPU6050_DLPF_t dlpf) {
    dev->hi2c = hi2c;
    dev->i2c_addr = i2c_addr;
    dev->accel_fs = accel_fs;
    dev->gyro_fs = gyro_fs;
    dev->dlpf = dlpf;
    dev->is_initialized = false;

    dev->offset_ax = 0;
    dev->offset_ay = 0;
    dev->offset_az = 0;
    dev->offset_gx = 0;
    dev->offset_gy = 0;
    dev->offset_gz = 0;

    dev->data.roll_deg = 0.0f;
    dev->data.pitch_deg = 0.0f;

    /* Check WHO_AM_I */
    uint8_t who_am_i = 0;
    if (MPU6050_ReadRegisters(dev, MPU6050_REG_WHO_AM_I, &who_am_i, 1) != HAL_OK) {
        LOG_WARN("MPU6050 @ 0x%02X: I2C read WHO_AM_I failed (no ACK)", dev->i2c_addr >> 1);
        return HAL_ERROR;
    }

    LOG_INFO("MPU6050 @ 0x%02X responded with WHO_AM_I = 0x%02X", dev->i2c_addr >> 1, who_am_i);

    /* Accept any valid MPU-6050, MPU-6500, MPU-9250 or compatible clone ID */
    if (who_am_i == 0x00 || who_am_i == 0xFF) {
        LOG_WARN("MPU6050 @ 0x%02X: invalid ID 0x%02X", dev->i2c_addr >> 1, who_am_i);
        return HAL_ERROR;
    }

    /* Reset device */
    MPU6050_WriteRegister(dev, MPU6050_REG_PWR_MGMT_1, 0x80);
    HAL_Delay(50);

    /* Wake up device (clear SLEEP bit) */
    if (MPU6050_WriteRegister(dev, MPU6050_REG_PWR_MGMT_1, 0x00) != HAL_OK) {
        LOG_WARN("MPU6050 @ 0x%02X: wake up failed", dev->i2c_addr >> 1);
        return HAL_ERROR;
    }
    HAL_Delay(10);

    /* Select Auto PLL clock source */
    MPU6050_WriteRegister(dev, MPU6050_REG_PWR_MGMT_1, 0x01);
    HAL_Delay(10);

    /* Configure DLPF (Digital Low Pass Filter) */
    MPU6050_WriteRegister(dev, MPU6050_REG_CONFIG, (uint8_t)dlpf);

    /* Sample Rate Divider = 0 (Internal sample rate 1 kHz / (1 + 0) = 1 kHz) */
    MPU6050_WriteRegister(dev, MPU6050_REG_SMPLRT_DIV, 0x00);

    /* Configure Gyro Full Scale */
    MPU6050_WriteRegister(dev, MPU6050_REG_GYRO_CONFIG, ((uint8_t)gyro_fs) << 3);
    switch (gyro_fs) {
        case MPU6050_GYRO_FS_250DPS:  dev->gyro_scale = 131.0f; break;
        case MPU6050_GYRO_FS_500DPS:  dev->gyro_scale = 65.5f;  break;
        case MPU6050_GYRO_FS_1000DPS: dev->gyro_scale = 32.8f;  break;
        case MPU6050_GYRO_FS_2000DPS: dev->gyro_scale = 16.4f;  break;
        default:                      dev->gyro_scale = 131.0f; break;
    }

    /* Configure Accel Full Scale */
    MPU6050_WriteRegister(dev, MPU6050_REG_ACCEL_CONFIG, ((uint8_t)accel_fs) << 3);
    switch (accel_fs) {
        case MPU6050_ACCEL_FS_2G:  dev->accel_scale = 16384.0f; break;
        case MPU6050_ACCEL_FS_4G:  dev->accel_scale = 8192.0f;  break;
        case MPU6050_ACCEL_FS_8G:  dev->accel_scale = 4096.0f;  break;
        case MPU6050_ACCEL_FS_16G: dev->accel_scale = 2048.0f;  break;
        default:                   dev->accel_scale = 16384.0f; break;
    }

    dev->is_initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_SetBypass(MPU6050_t *dev, bool enable) {
    uint8_t user_ctrl = 0;
    uint8_t int_pin_cfg = 0;

    /* Read current USER_CTRL and INT_PIN_CFG */
    if (MPU6050_ReadRegisters(dev, MPU6050_REG_USER_CTRL, &user_ctrl, 1) != HAL_OK) return HAL_ERROR;
    if (MPU6050_ReadRegisters(dev, MPU6050_REG_INT_PIN_CFG, &int_pin_cfg, 1) != HAL_OK) return HAL_ERROR;

    if (enable) {
        /* Disable MPU-6050 internal I2C Master mode */
        user_ctrl &= ~(1 << 5); /* Clear I2C_MST_EN */
        MPU6050_WriteRegister(dev, MPU6050_REG_USER_CTRL, user_ctrl);

        /* Enable I2C Bypass multiplexer */
        int_pin_cfg |= (1 << 1); /* Set BYPASS_EN */
        return MPU6050_WriteRegister(dev, MPU6050_REG_INT_PIN_CFG, int_pin_cfg);
    } else {
        /* Disable bypass */
        int_pin_cfg &= ~(1 << 1);
        return MPU6050_WriteRegister(dev, MPU6050_REG_INT_PIN_CFG, int_pin_cfg);
    }
}

HAL_StatusTypeDef MPU6050_ReadSensors(MPU6050_t *dev) {
    if (!dev->is_initialized) return HAL_ERROR;

    uint8_t buffer[14];
    /* Burst read 14 bytes: Accel X/Y/Z, Temp, Gyro X/Y/Z starting at ACCEL_XOUT_H (0x3B) */
    if (MPU6050_ReadRegisters(dev, MPU6050_REG_ACCEL_XOUT_H, buffer, 14) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Reconstruct 16-bit values */
    dev->data.raw_ax = (int16_t)((buffer[0]  << 8) | buffer[1])  - dev->offset_ax;
    dev->data.raw_ay = (int16_t)((buffer[2]  << 8) | buffer[3])  - dev->offset_ay;
    dev->data.raw_az = (int16_t)((buffer[4]  << 8) | buffer[5])  - dev->offset_az;

    dev->data.raw_temp = (int16_t)((buffer[6] << 8) | buffer[7]);

    dev->data.raw_gx = (int16_t)((buffer[8]  << 8) | buffer[9])  - dev->offset_gx;
    dev->data.raw_gy = (int16_t)((buffer[10] << 8) | buffer[11]) - dev->offset_gy;
    dev->data.raw_gz = (int16_t)((buffer[12] << 8) | buffer[13]) - dev->offset_gz;

    /* Convert to engineering units */
    dev->data.ax_g = (float)dev->data.raw_ax / dev->accel_scale;
    dev->data.ay_g = (float)dev->data.raw_ay / dev->accel_scale;
    dev->data.az_g = (float)dev->data.raw_az / dev->accel_scale;

    dev->data.ax_ms2 = dev->data.ax_g * 9.80665f;
    dev->data.ay_ms2 = dev->data.ay_g * 9.80665f;
    dev->data.az_ms2 = dev->data.az_g * 9.80665f;

    dev->data.gx_dps = (float)dev->data.raw_gx / dev->gyro_scale;
    dev->data.gy_dps = (float)dev->data.raw_gy / dev->gyro_scale;
    dev->data.gz_dps = (float)dev->data.raw_gz / dev->gyro_scale;

    /* Temperature formula: Temperature in degrees C = (TEMP_OUT Register Value as a signed quantity)/340 + 36.53 */
    dev->data.temp_c = ((float)dev->data.raw_temp / 340.0f) + 36.53f;

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_CalibrateOffsets(MPU6050_t *dev, uint16_t sample_count) {
    if (sample_count == 0) sample_count = 200;

    int32_t sum_ax = 0, sum_ay = 0, sum_az = 0;
    int32_t sum_gx = 0, sum_gy = 0, sum_gz = 0;

    dev->offset_ax = 0;
    dev->offset_ay = 0;
    dev->offset_az = 0;
    dev->offset_gx = 0;
    dev->offset_gy = 0;
    dev->offset_gz = 0;

    for (uint16_t i = 0; i < sample_count; i++) {
        if (MPU6050_ReadSensors(dev) != HAL_OK) return HAL_ERROR;

        sum_ax += dev->data.raw_ax;
        sum_ay += dev->data.raw_ay;
        /* When stationary and flat, Z experiences +1g */
        sum_az += (dev->data.raw_az - (int16_t)dev->accel_scale);

        sum_gx += dev->data.raw_gx;
        sum_gy += dev->data.raw_gy;
        sum_gz += dev->data.raw_gz;

        HAL_Delay(3);
    }

    dev->offset_ax = (int16_t)(sum_ax / sample_count);
    dev->offset_ay = (int16_t)(sum_ay / sample_count);
    dev->offset_az = (int16_t)(sum_az / sample_count);

    dev->offset_gx = (int16_t)(sum_gx / sample_count);
    dev->offset_gy = (int16_t)(sum_gy / sample_count);
    dev->offset_gz = (int16_t)(sum_gz / sample_count);

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_UpdateAttitude(MPU6050_t *dev, float dt_seconds) {
    if (MPU6050_ReadSensors(dev) != HAL_OK) return HAL_ERROR;

    /* Compute Pitch and Roll from Accelerometer */
    float accel_roll  = atan2f(dev->data.ay_g, dev->data.az_g) * RAD_TO_DEG;
    float accel_pitch = atan2f(-dev->data.ax_g, sqrtf(dev->data.ay_g * dev->data.ay_g + dev->data.az_g * dev->data.az_g)) * RAD_TO_DEG;

    /* Complementary filter (alpha = 0.96 gyro, 0.04 accel) */
    const float alpha = 0.96f;
    dev->data.roll_deg  = alpha * (dev->data.roll_deg  + dev->data.gx_dps * dt_seconds) + (1.0f - alpha) * accel_roll;
    dev->data.pitch_deg = alpha * (dev->data.pitch_deg + dev->data.gy_dps * dt_seconds) + (1.0f - alpha) * accel_pitch;

    return HAL_OK;
}
