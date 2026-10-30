/**
  ******************************************************************************
  * @file           : sensor_config.h
  * @brief          : Centralized Hardware & Pin Configuration for ROV Sensors
  *                   (GY-521, HMC5883L/QMC5883L, GY-87, MS5540C, Logger)
  *                   STM32F401RCT6
  ******************************************************************************
  * @note
  *  Easily change any pin, bus, address, or rate by editing this file!
  ******************************************************************************
  */

#ifndef __SENSOR_CONFIG_H
#define __SENSOR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* ==============================================================================
 * 0. SENSOR SELECTION SWITCHES (Test Sensor One by One!)
 * ==============================================================================
 * Set to 1 to enable, 0 to disable:
 * - Test GY-521 alone first to verify I2C wiring!
 * - When ready, enable GY-87 and MS5540C.
 */
#define ENABLE_GY521                    1   /* 1 = Enable GY-521 (MPU-6050 6-Axis IMU) */
#define ENABLE_GY87                     0   /* 1 = Enable GY-87 (10-DOF Composite IMU) */
#define ENABLE_MS5540C                  0   /* 1 = Enable MS5540C Subsea Depth Sensor */

/* Enable Live Text Diagnostics in Console (prints real-time sensor metrics every 1s) */
#define SENSOR_LIVE_DIAGNOSTICS         1

/* ==============================================================================
 * 1. I2C BUS & PIN CONFIGURATION
 * ==============================================================================
 * Supported Pin Pairs for I2C1 on STM32F401:
 *   Pair 0: PB8 (SCL) and PB9 (SDA)  - Standard
 *   Pair 1: PB6 (SCL) and PB7 (SDA)  - Alternative
 *   Pair 2: AUTO - Probe BOTH pairs and automatically lock onto whichever has the sensor!
 */
#define I2C1_PIN_SELECTION              2   /* 0 = PB8/PB9, 1 = PB6/PB7, 2 = AUTO-DETECT */

/* Peripheral Handle */
extern I2C_HandleTypeDef hi2c1;
#define SENSORS_I2C_HANDLE              hi2c1
#define SENSORS_I2C_TIMEOUT_MS          50

#define SENSORS_I2C_PORT                GPIOB
#define SENSORS_I2C_SCL_PIN             GPIO_PIN_8
#define SENSORS_I2C_SDA_PIN             GPIO_PIN_9

/* ==============================================================================
 * 2. SENSOR I2C ADDRESSES (7-bit address shifted left by 1 for HAL)
 * ==============================================================================
 * GY-521 AD0 pin:
 *   Floating / GND -> Address 0x68
 *   Connected 3.3V -> Address 0x69
 * GY-87:
 *   MPU-6050 fixed at 0x68
 */
#define GY87_MPU6050_I2C_ADDR           (0x68 << 1)
#define GY521_MPU6050_I2C_ADDR          (0x68 << 1) /* 0x68 or 0x69 */

/* HMC5883L Compass (On GY-87 or standalone) */
#define HMC5883L_I2C_ADDR               (0x1E << 1)
/* QMC5883L Compass Clone (Auto-detected if HMC5883L not responding) */
#define QMC5883L_I2C_ADDR               (0x0D << 1)

/* BMP180 Barometer (On GY-87) */
#define BMP180_I2C_ADDR                 (0x77 << 1)

/* ==============================================================================
 * 3. MS5540C DEPTH / PRESSURE SENSOR CONFIGURATION
 * ==============================================================================
 * MS5540C requires:
 *   1. MCLK: 32.768 kHz continuous clock (50% duty cycle) for its internal ADC
 *   2. Synchronous 3-wire serial interface: SCLK, DIN (Master OUT), DOUT (Master IN)
 */
/* MCLK Generation: Uses Timer PWM (e.g. TIM2 Channel 1 on PA0 or TIM3 CH1 on PA6)
 * Or MCO1 (PA8) / GPIO Toggle. We use TIM2 CH1 on PA0 by default.
 */
extern TIM_HandleTypeDef htim2;
#define MS5540C_MCLK_TIM_HANDLE         htim2
#define MS5540C_MCLK_TIM_CHANNEL        TIM_CHANNEL_1
#define MS5540C_MCLK_PORT               GPIOA
#define MS5540C_MCLK_PIN                GPIO_PIN_0

/* 3-Wire Serial Interface Pins (GPIO Bit-Banging for robust 21-bit reset protocol) */
#define MS5540C_SCLK_PORT               GPIOB
#define MS5540C_SCLK_PIN                GPIO_PIN_13

#define MS5540C_DIN_PORT                GPIOB     /* MOSI: STM32 Out -> MS5540C DIN */
#define MS5540C_DIN_PIN                 GPIO_PIN_15

#define MS5540C_DOUT_PORT               GPIOB     /* MISO: STM32 In <- MS5540C DOUT */
#define MS5540C_DOUT_PIN                GPIO_PIN_14

/* Water Density for Depth Calculation */
#define WATER_DENSITY_FRESH             1000.0f   /* kg/m^3 for freshwater */
#define WATER_DENSITY_SEA               1025.0f   /* kg/m^3 for seawater */
#define ROV_WATER_DENSITY               WATER_DENSITY_FRESH
#define STANDARD_GRAVITY                9.80665f  /* m/s^2 */

/* ==============================================================================
 * 4. LOGGING & TELEMETRY CONFIGURATION
 * ==============================================================================
 * Primary: Native USB CDC (Virtual COM Port) over Black Pill Type-C cable (PA11/PA12)
 * Secondary: UART2 DMA on PA2 (TX) / PA3 (RX) - for external TTL adapter if needed
 * Both transmit simultaneously!
 */
extern UART_HandleTypeDef huart2;
#define LOGGER_UART_HANDLE              huart2
#define LOGGER_BAUDRATE                 115200    /* Recommended: 115200 or 921600 */
#define LOGGER_BUFFER_SIZE              1024      /* Ring buffer capacity in bytes */

/* Format of Telemetry:
 * 1 = JSON stream for Web Dashboard (High-level rich telemetry)
 * 0 = Compact CSV stream (Fast minimal bandwidth)
 */
#define TELEMETRY_USE_JSON              1

/* Magnetic Declination in degrees for Compass Heading calibration
 * (Adjust for your local geographic coordinates)
 */
#define LOCAL_MAGNETIC_DECLINATION_DEG  4.5f

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_CONFIG_H */
