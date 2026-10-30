/**
  ******************************************************************************
  * @file           : ms5540c.h
  * @brief          : High-Precision Depth & Pressure Driver for MS5540C
  *                   10 to 1100 mbar (0.1 mbar res) with 2nd-order Temp Comp
  *                   STM32F401 HAL Driver
  ******************************************************************************
  */

#ifndef __MS5540C_H
#define __MS5540C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor_config.h"
#include <stdbool.h>
#include <stdint.h>

/* PROM Calibration Coefficients */
typedef struct {
    uint16_t c1; /* Pressure sensitivity (13-bit) */
    uint16_t c2; /* Pressure offset (13-bit) */
    uint16_t c3; /* Temp coeff of pressure sensitivity (10-bit) */
    uint16_t c4; /* Temp coeff of pressure offset (9-bit) */
    uint16_t c5; /* Reference temperature (12-bit) */
    uint16_t c6; /* Temp coeff of temperature (7-bit) */
} MS5540C_Calib_t;

/* Sensor Data Structure */
typedef struct {
    uint16_t raw_d1;        /* Raw uncompensated digital pressure */
    uint16_t raw_d2;        /* Raw uncompensated digital temperature */

    float pressure_mbar;    /* Pressure in mbar (hPa) */
    float temperature_c;    /* Water/Ambient Temperature in deg C */
    float depth_meters;     /* Computed Water Depth in meters */
} MS5540C_Data_t;

/* Pin Configuration for Bit-Banged 3-wire Interface */
typedef struct {
    GPIO_TypeDef *sclk_port;
    uint16_t     sclk_pin;
    GPIO_TypeDef *din_port;  /* MOSI (STM32 OUT -> MS5540C DIN) */
    uint16_t     din_pin;
    GPIO_TypeDef *dout_port; /* MISO (STM32 IN <- MS5540C DOUT) */
    uint16_t     dout_pin;
} MS5540C_Pins_t;

/* MS5540C Device Handle */
typedef struct {
    MS5540C_Pins_t pins;
    MS5540C_Calib_t calib;
    MS5540C_Data_t data;

    float surface_pressure_mbar; /* Surface atmospheric baseline */
    float water_density;         /* kg/m^3 (1000 for fresh, 1025 for seawater) */
    bool is_initialized;
} MS5540C_t;

/* Public API */
HAL_StatusTypeDef MS5540C_Init(MS5540C_t *dev, float water_density);
HAL_StatusTypeDef MS5540C_StartMCLK(TIM_HandleTypeDef *htim, uint32_t channel);
HAL_StatusTypeDef MS5540C_Read(MS5540C_t *dev);
void MS5540C_TareSurface(MS5540C_t *dev);
void MS5540C_SetWaterDensity(MS5540C_t *dev, float density_kg_m3);

#ifdef __cplusplus
}
#endif

#endif /* __MS5540C_H */
