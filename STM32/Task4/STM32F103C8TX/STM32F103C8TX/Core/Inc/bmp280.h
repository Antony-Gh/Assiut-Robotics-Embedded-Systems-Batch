/**
 * @file bmp280.h
 * @brief High-Precision I2C Driver for Bosch BMP280 / BMP180 Barometric Pressure & Temperature Sensors
 * @author Assiut Robotics Team
 * @date 2026-10-29
 *
 * @details Provides comprehensive support for Bosch digital pressure sensors over I2C.
 *          Automatically identifies whether a BMP280 or BMP180 is connected, reads factory
 *          calibration coefficients, performs compensation, and outputs accurate Temperature
 *          in degrees Celsius and Barometric Pressure in hPa and Pa.
 */

#ifndef __BMP280_H__
#define __BMP280_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup BMP_Constants I2C Addresses and Sensor IDs
 * @{
 */
#define BMP280_I2C_ADDR_PRIM        (0x76 << 1) /**< BMP280 Default I2C Address (SDO -> GND) */
#define BMP280_I2C_ADDR_SEC         (0x77 << 1) /**< BMP280 Secondary I2C Address (SDO -> VCC) */
#define BMP180_I2C_ADDR             (0x77 << 1) /**< BMP180 Standard I2C Address */

#define BMP280_CHIP_ID              0x58        /**< BMP280 Identification Register Value */
#define BMP180_CHIP_ID              0x55        /**< BMP180 Identification Register Value */
#define BME280_CHIP_ID              0x60        /**< BME280 Identification Register Value */

#define BMP280_REG_ID               0xD0        /**< Chip ID Register */
#define BMP280_REG_RESET            0xE0        /**< Soft Reset Register */
#define BMP280_REG_CTRL_MEAS        0xF4        /**< Control Measurement (Oversampling & Mode) */
#define BMP280_REG_CONFIG           0xF5        /**< Configuration (Filter & Standby) */
#define BMP280_REG_PRESS_MSB        0xF7        /**< Pressure Data MSB */
#define BMP280_REG_CALIB_START      0x88        /**< Calibration Data Start Address (24 bytes) */
/** @} */

/**
 * @brief BMP Sensor Type Enumeration
 */
typedef enum
{
    BMP_TYPE_UNKNOWN = 0,
    BMP_TYPE_BMP180  = 1,
    BMP_TYPE_BMP280  = 2
} BMP_Type_t;

/**
 * @brief BMP280 Trimming Calibration Parameters Structure
 */
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData_t;

/**
 * @brief BMP180 Trimming Calibration Parameters Structure
 */
typedef struct
{
    int16_t  ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t  b1, b2;
    int16_t  mb, mc, md;
} BMP180_CalibData_t;

/**
 * @brief Unified BMP Sensor Device Handle Structure
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;      /**< Pointer to STM32 HAL I2C Handle */
    uint8_t i2c_addr;             /**< Shifted 8-bit I2C address */
    BMP_Type_t sensor_type;       /**< Detected sensor type (BMP180 or BMP280) */
    uint8_t chip_id;              /**< Chip ID read from register 0xD0 */
    bool is_initialized;          /**< True if sensor is successfully initialized */

    /* Calibration data storage */
    union {
        BMP280_CalibData_t bmp280;
        BMP180_CalibData_t bmp180;
    } calib;

    int32_t t_fine;               /**< Bosch internal fine temperature for pressure compensation */

    /* Physical measured output values */
    float temperature;            /**< Ambient temperature in degrees Celsius */
    float pressure_hpa;           /**< Barometric pressure in hectopascals (hPa) / millibars */
    float pressure_pa;            /**< Barometric pressure in Pascals (Pa) */
    float altitude;               /**< Estimated barometric altitude in meters */
} BMP_t;

/**
 * @brief Initializes the BMP sensor, auto-detects model, and reads calibration coefficients.
 * @param[in,out] dev Pointer to BMP handle structure.
 * @param[in] hi2c Pointer to HAL I2C peripheral handle.
 * @retval HAL_StatusTypeDef HAL_OK if sensor was successfully found and configured.
 */
HAL_StatusTypeDef BMP_Init(BMP_t *dev, I2C_HandleTypeDef *hi2c);

/**
 * @brief Triggers measurement and reads compensated Temperature and Pressure.
 * @param[in,out] dev Pointer to BMP handle structure.
 * @retval HAL_StatusTypeDef HAL_OK if reading was successful.
 */
HAL_StatusTypeDef BMP_Read(BMP_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H__ */
