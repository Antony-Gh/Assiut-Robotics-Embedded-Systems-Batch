/**
  ******************************************************************************
  * @file           : bmp180.h
  * @brief          : High-Precision Driver for BMP180 Barometer (On GY-87)
  *                   STM32F401 HAL Driver
  ******************************************************************************
  */

#ifndef __BMP180_H
#define __BMP180_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define BMP180_I2C_ADDR_DEFAULT         (0x77 << 1)
#define BMP180_CHIP_ID_REG              0xD0
#define BMP180_CHIP_ID_VAL              0x55

#define BMP180_CTRL_MEAS_REG            0xF4
#define BMP180_OUT_MSB_REG              0xF6

#define BMP180_CMD_READ_TEMP            0x2E
#define BMP180_CMD_READ_PRESS_OSS0      0x34

/* Oversampling Modes */
typedef enum {
    BMP180_OSS_ULTRA_LOW_POWER = 0, /* 1 sample,  4.5 ms conversion */
    BMP180_OSS_STANDARD        = 1, /* 2 samples, 7.5 ms conversion */
    BMP180_OSS_HIGH_RES        = 2, /* 4 samples, 13.5 ms conversion */
    BMP180_OSS_ULTRA_HIGH_RES  = 3  /* 8 samples, 25.5 ms conversion */
} BMP180_OSS_t;

/* Calibration Parameters from internal EEPROM */
typedef struct {
    int16_t  ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t  b1, b2;
    int16_t  mb, mc, md;
} BMP180_Calib_t;

/* Sensor Data Output */
typedef struct {
    float temperature_c;    /* Temperature in deg C */
    float pressure_hpa;     /* Pressure in hPa (mbar) */
    float pressure_pa;      /* Pressure in Pascals */
    float altitude_m;       /* Calculated barometric altitude in meters */
} BMP180_Data_t;

/* BMP180 Device Handle */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
    BMP180_OSS_t oss;
    BMP180_Calib_t calib;
    BMP180_Data_t data;
    float sea_level_pressure_hpa; /* Baseline for altitude (default: 1013.25) */
    bool is_initialized;
} BMP180_t;

/* Public API */
HAL_StatusTypeDef BMP180_Init(BMP180_t *dev, I2C_HandleTypeDef *hi2c, BMP180_OSS_t oss);
HAL_StatusTypeDef BMP180_Read(BMP180_t *dev);
void BMP180_SetSeaLevelPressure(BMP180_t *dev, float sea_level_hpa);

#ifdef __cplusplus
}
#endif

#endif /* __BMP180_H */
