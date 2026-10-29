/**
 * @file mpu6050.h
 * @brief High-Precision I2C Driver for MPU6050 6-Axis MotionTracking Device
 * @author Assiut Robotics Team
 * @date 2026-10-29
 *
 * @details This header defines register addresses, configuration macros, and the
 *          complete API for interfacing the InvenSense MPU-6050 (GY-521 module)
 *          over I2C on STM32F103 microcontrollers. Provides raw sensor acquisition,
 *          unit scaling, and complementary filtering to compute Roll, Pitch, and Yaw.
 */

#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup MPU6050_Registers MPU6050 Register Map
 * @{
 */
#define MPU6050_DEFAULT_ADDR        (0x68 << 1) /**< Default 7-bit I2C Address (AD0 low) shifted for HAL */
#define MPU6050_ALT_ADDR            (0x69 << 1) /**< Alternative I2C Address (AD0 high) */

#define MPU6050_REG_SMPLRT_DIV      0x19        /**< Sample Rate Divider */
#define MPU6050_REG_CONFIG          0x1A        /**< Configuration (DLPF) */
#define MPU6050_REG_GYRO_CONFIG     0x1B        /**< Gyroscope Configuration */
#define MPU6050_REG_ACCEL_CONFIG    0x1C        /**< Accelerometer Configuration */
#define MPU6050_REG_ACCEL_XOUT_H    0x3B        /**< Accelerometer Measurements Base Address */
#define MPU6050_REG_TEMP_OUT_H      0x41        /**< Temperature Measurement High Byte */
#define MPU6050_REG_GYRO_XOUT_H     0x43        /**< Gyroscope Measurements Base Address */
#define MPU6050_REG_PWR_MGMT_1      0x6B        /**< Power Management 1 */
#define MPU6050_REG_WHO_AM_I        0x75        /**< Device Identification (Returns 0x68) */
/** @} */

/**
 * @defgroup MPU6050_Scales Sensor Sensitivity Scales
 * @{
 */
#define MPU6050_ACCEL_SCALE_2G      16384.0f    /**< LSB/g for +/- 2g range */
#define MPU6050_ACCEL_SCALE_4G      8192.0f     /**< LSB/g for +/- 4g range */
#define MPU6050_ACCEL_SCALE_8G      4096.0f     /**< LSB/g for +/- 8g range */
#define MPU6050_ACCEL_SCALE_16G     2048.0f     /**< LSB/g for +/- 16g range */

#define MPU6050_GYRO_SCALE_250      131.0f      /**< LSB/(deg/s) for +/- 250 deg/s range */
#define MPU6050_GYRO_SCALE_500      65.5f       /**< LSB/(deg/s) for +/- 500 deg/s range */
#define MPU6050_GYRO_SCALE_1000     32.8f       /**< LSB/(deg/s) for +/- 1000 deg/s range */
#define MPU6050_GYRO_SCALE_2000     16.4f       /**< LSB/(deg/s) for +/- 2000 deg/s range */
/** @} */

#define MPU6050_RAD_TO_DEG          (180.0f / 3.14159265358979323846f) /**< Conversion factor */

/**
 * @brief MPU6050 Device Handle Structure
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;      /**< Pointer to STM32 HAL I2C Handle */
    uint8_t i2c_addr;             /**< Shifted 8-bit I2C address */
    bool is_initialized;          /**< True if sensor acknowledged and initialized successfully */

    /* Raw 16-bit sensor data */
    int16_t raw_ax;               /**< Raw X-axis accelerometer reading */
    int16_t raw_ay;               /**< Raw Y-axis accelerometer reading */
    int16_t raw_az;               /**< Raw Z-axis accelerometer reading */
    int16_t raw_gx;               /**< Raw X-axis gyroscope reading */
    int16_t raw_gy;               /**< Raw Y-axis gyroscope reading */
    int16_t raw_gz;               /**< Raw Z-axis gyroscope reading */
    int16_t raw_temp;             /**< Raw on-chip temperature reading */

    /* Scaled physical values */
    float ax;                     /**< Linear acceleration X in g */
    float ay;                     /**< Linear acceleration Y in g */
    float az;                     /**< Linear acceleration Z in g */
    float gx;                     /**< Angular velocity X in degrees/sec */
    float gy;                     /**< Angular velocity Y in degrees/sec */
    float gz;                     /**< Angular velocity Z in degrees/sec */
    float temperature;            /**< Die temperature in degrees Celsius */

    /* Calibrated / Filtered Orientation Angles */
    float roll;                   /**< Roll angle in degrees (rotation around X-axis) */
    float pitch;                  /**< Pitch angle in degrees (rotation around Y-axis) */
    float yaw;                    /**< Yaw angle in degrees (rotation around Z-axis, gyro integration) */

    /* Filter configuration */
    float alpha;                  /**< Complementary filter weighting factor (typically 0.98f) */
    uint32_t last_tick;           /**< Timestamp in milliseconds of last reading */
} MPU6050_t;

/**
 * @brief Initializes the MPU6050 sensor, wakes it from sleep, and configures ranges.
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @param[in] hi2c Pointer to HAL I2C peripheral handle.
 * @param[in] addr 7-bit or shifted 8-bit I2C slave address.
 * @retval HAL_StatusTypeDef HAL_OK if initialization succeeded, HAL_ERROR otherwise.
 */
HAL_StatusTypeDef MPU6050_Init(MPU6050_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);

/**
 * @brief Reads all 14 burst registers from the MPU6050 (Accel, Temp, Gyro).
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @retval HAL_StatusTypeDef HAL_OK if I2C transaction succeeded.
 */
HAL_StatusTypeDef MPU6050_Read(MPU6050_t *dev);

/**
 * @brief Updates orientation angles (Roll, Pitch, Yaw) using a Complementary Filter.
 * @param[in,out] dev Pointer to MPU6050 handle structure.
 * @retval None
 */
void MPU6050_UpdateAngles(MPU6050_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H__ */
