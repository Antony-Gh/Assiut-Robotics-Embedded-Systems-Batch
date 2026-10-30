/**
 * @file bmp280.h
 * @brief High-Precision I2C Driver for Bosch BMP280 / BMP180 Sensors
 * @author Assiut Robotics Team
 * @date 2026-10-30
 */

#ifndef __BMP280_H__
#define __BMP280_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define BMP280_I2C_ADDR_PRIM        (0x76 << 1)
#define BMP280_I2C_ADDR_SEC         (0x77 << 1)
#define BMP180_I2C_ADDR             (0x77 << 1)

#define BMP280_CHIP_ID              0x58
#define BMP180_CHIP_ID              0x55
#define BME280_CHIP_ID              0x60

#define BMP280_REG_ID               0xD0
#define BMP280_REG_RESET            0xE0
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_PRESS_MSB        0xF7
#define BMP280_REG_CALIB_START      0x88

typedef enum {
    BMP_TYPE_UNKNOWN = 0,
    BMP_TYPE_BMP180  = 1,
    BMP_TYPE_BMP280  = 2
} BMP_Type_t;

typedef struct {
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

typedef struct {
    int16_t  ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t  b1, b2;
    int16_t  mb, mc, md;
} BMP180_CalibData_t;

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
    BMP_Type_t sensor_type;
    uint8_t chip_id;
    bool is_initialized;

    union {
        BMP280_CalibData_t bmp280;
        BMP180_CalibData_t bmp180;
    } calib;

    int32_t t_fine;
    float temperature;
    float pressure_hpa;
    float pressure_pa;
    float altitude;
} BMP_t;

HAL_StatusTypeDef BMP_Init(BMP_t *dev, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BMP_Read(BMP_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H__ */
