/**
 * @file mpu6050.c
 * @brief Implementation of MPU6050 I2C Driver and Complementary Orientation Filter
 * @author Assiut Robotics Team
 * @date 2026-10-29
 */

#include "mpu6050.h"
#include <math.h>

/**
 * @brief Initializes the MPU6050 sensor, wakes it from sleep, and configures ranges.
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @param[in] hi2c Pointer to HAL I2C peripheral handle.
 * @param[in] addr 7-bit or shifted 8-bit I2C slave address.
 * @retval HAL_StatusTypeDef HAL_OK if initialization succeeded, HAL_ERROR otherwise.
 */
HAL_StatusTypeDef MPU6050_Init(MPU6050_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    if (dev == NULL || hi2c == NULL) return HAL_ERROR;

    dev->hi2c = hi2c;
    /* If 7-bit address (<= 0x7F) provided, shift to 8-bit HAL format */
    dev->i2c_addr = (addr <= 0x7F) ? (addr << 1) : addr;
    dev->is_initialized = false;
    dev->roll = 0.0f;
    dev->pitch = 0.0f;
    dev->yaw = 0.0f;
    dev->alpha = 0.98f; /* 98% Gyro trust for high-frequency, 2% Accel for drift cancellation */
    dev->last_tick = HAL_GetTick();

    /* 1. Check Device Identification (WHO_AM_I register) */
    uint8_t who_am_i = 0;
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                                MPU6050_REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                                                &who_am_i, 1, 100);

    /* Note: In physical chips who_am_i returns 0x68. In Proteus simulation,
     * some models respond with 0x68 or acknowledge without strictly checking who_am_i.
     */
    if (status != HAL_OK)
    {
        /* Try alternative address 0x69 (0xD2) if 0x68 failed */
        dev->i2c_addr = MPU6050_ALT_ADDR;
        status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                  MPU6050_REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                                  &who_am_i, 1, 100);
        if (status != HAL_OK)
        {
            /* Return to default address */
            dev->i2c_addr = MPU6050_DEFAULT_ADDR;
        }
    }

    /* 2. Wake up MPU6050: Write 0x00 to PWR_MGMT_1 (resets sleep bit and selects 8MHz internal osc) */
    uint8_t pwr_data = 0x00;
    status = HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                               MPU6050_REG_PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT,
                               &pwr_data, 1, 100);
    if (status != HAL_OK) return status;

    /* 3. Configure Sample Rate Divider (SMPLRT_DIV = 7 -> 1kHz / (1 + 7) = 125 Hz) */
    uint8_t smplrt = 0x07;
    HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                      MPU6050_REG_SMPLRT_DIV, I2C_MEMADD_SIZE_8BIT,
                      &smplrt, 1, 100);

    /* 4. Configure Digital Low Pass Filter (DLPF_CFG = 0x03 -> ~44 Hz bandwidth) */
    uint8_t config = 0x03;
    HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                      MPU6050_REG_CONFIG, I2C_MEMADD_SIZE_8BIT,
                      &config, 1, 100);

    /* 5. Configure Accelerometer Full Scale Range (+/- 2g: FS_SEL = 0) */
    uint8_t accel_cfg = 0x00;
    HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                      MPU6050_REG_ACCEL_CONFIG, I2C_MEMADD_SIZE_8BIT,
                      &accel_cfg, 1, 100);

    /* 6. Configure Gyroscope Full Scale Range (+/- 250 deg/s: FS_SEL = 0) */
    uint8_t gyro_cfg = 0x00;
    HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                      MPU6050_REG_GYRO_CONFIG, I2C_MEMADD_SIZE_8BIT,
                      &gyro_cfg, 1, 100);

    dev->is_initialized = true;
    dev->last_tick = HAL_GetTick();

    /* Perform initial burst read to prime orientation values */
    MPU6050_Read(dev);

    return HAL_OK;
}

/**
 * @brief Reads all 14 burst registers from the MPU6050 (Accel, Temp, Gyro).
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @retval HAL_StatusTypeDef HAL_OK if I2C transaction succeeded.
 */
HAL_StatusTypeDef MPU6050_Read(MPU6050_t *dev)
{
    if (dev == NULL || dev->hi2c == NULL) return HAL_ERROR;

    uint8_t buffer[14];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                                MPU6050_REG_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT,
                                                buffer, 14, 100);
    if (status != HAL_OK) return status;

    /* Assemble 16-bit signed raw integer values (Big-Endian format) */
    dev->raw_ax   = (int16_t)(((uint16_t)buffer[0]  << 8) | buffer[1]);
    dev->raw_ay   = (int16_t)(((uint16_t)buffer[2]  << 8) | buffer[3]);
    dev->raw_az   = (int16_t)(((uint16_t)buffer[4]  << 8) | buffer[5]);
    dev->raw_temp = (int16_t)(((uint16_t)buffer[6]  << 8) | buffer[7]);
    dev->raw_gx   = (int16_t)(((uint16_t)buffer[8]  << 8) | buffer[9]);
    dev->raw_gy   = (int16_t)(((uint16_t)buffer[10] << 8) | buffer[11]);
    dev->raw_gz   = (int16_t)(((uint16_t)buffer[12] << 8) | buffer[13]);

    /* Convert to physical floating point units */
    dev->ax = (float)dev->raw_ax / MPU6050_ACCEL_SCALE_2G;
    dev->ay = (float)dev->raw_ay / MPU6050_ACCEL_SCALE_2G;
    dev->az = (float)dev->raw_az / MPU6050_ACCEL_SCALE_2G;

    dev->gx = (float)dev->raw_gx / MPU6050_GYRO_SCALE_250;
    dev->gy = (float)dev->raw_gy / MPU6050_GYRO_SCALE_250;
    dev->gz = (float)dev->raw_gz / MPU6050_GYRO_SCALE_250;

    /* On-chip temperature formula: Temperature in deg C = (TEMP_OUT / 340) + 36.53 */
    dev->temperature = ((float)dev->raw_temp / 340.0f) + 36.53f;

    /* Update filtered Pitch, Roll, and Yaw angles */
    MPU6050_UpdateAngles(dev);

    return HAL_OK;
}

/**
 * @brief Updates orientation angles (Roll, Pitch, Yaw) using a Complementary Filter.
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @retval None
 */
void MPU6050_UpdateAngles(MPU6050_t *dev)
{
    if (dev == NULL) return;

    uint32_t now = HAL_GetTick();
    float dt = (float)(now - dev->last_tick) / 1000.0f;
    dev->last_tick = now;

    /* Guard against dt bounds during initial boot or system stalls */
    if (dt <= 0.0f || dt > 1.0f)
    {
        dt = 0.05f; /* Default to 50ms */
    }

    /* 1. Calculate static angles from Accelerometer (using gravity vector) */
    /* Roll: rotation around X axis (tilt left/right) */
    float accel_roll = atan2f(dev->ay, dev->az) * MPU6050_RAD_TO_DEG;

    /* Pitch: rotation around Y axis (tilt forward/backward) */
    float accel_pitch = atan2f(-dev->ax, sqrtf(dev->ay * dev->ay + dev->az * dev->az)) * MPU6050_RAD_TO_DEG;

    /* 2. Complementary Filter:
     * High-pass on Gyroscope (responsive to quick motion, immune to linear accelerations)
     * Low-pass on Accelerometer (eliminates long-term gyro drift, establishes true vertical)
     */
    dev->roll  = dev->alpha * (dev->roll  + dev->gx * dt) + (1.0f - dev->alpha) * accel_roll;
    dev->pitch = dev->alpha * (dev->pitch + dev->gy * dt) + (1.0f - dev->alpha) * accel_pitch;

    /* 3. Yaw (Rotation around Z axis):
     * Because gravity is orthogonal to horizontal rotation, the accelerometer CANNOT measure Yaw.
     * Yaw is calculated purely by integrating the Z-axis gyroscope angular rate.
     */
    dev->yaw += dev->gz * dt;

    /* Wrap Yaw within -180 to +180 degrees */
    if (dev->yaw > 180.0f)  dev->yaw -= 360.0f;
    if (dev->yaw < -180.0f) dev->yaw += 360.0f;
}
