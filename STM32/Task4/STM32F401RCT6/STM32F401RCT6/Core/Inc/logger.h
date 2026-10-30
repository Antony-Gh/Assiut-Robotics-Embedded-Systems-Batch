/**
  ******************************************************************************
  * @file           : logger.h
  * @brief          : High-Efficiency DMA Ring-Buffer Logger & Telemetry Streamer
  *                   STM32F401 HAL Driver for High-Speed ROV Telemetry
  ******************************************************************************
  */

#ifndef __LOGGER_H
#define __LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensor_config.h"
#include "mpu6050.h"
#include "gy87.h"
#include "ms5540c.h"
#include <stdio.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel_t;

/* Public API */
void LOGGER_Init(UART_HandleTypeDef *huart);
void LOGGER_Log(LogLevel_t level, const char *fmt, ...);
void LOGGER_SendTelemetryJSON(const MPU6050_Data_t *gy521, const GY87_Data_t *gy87, const MS5540C_Data_t *ms5540);
void LOGGER_SendTelemetryCSV(const MPU6050_Data_t *gy521, const GY87_Data_t *gy87, const MS5540C_Data_t *ms5540);
void ftoa_custom(char *buf, float val, int decimals);

/* Must be called from HAL_UART_TxCpltCallback in main.c or stm32f4xx_it.c */
void LOGGER_TxCpltCallback(UART_HandleTypeDef *huart);

/* Convenient Logging Macros */
#define LOG_DEBUG(fmt, ...) LOGGER_Log(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOGGER_Log(LOG_LEVEL_INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOGGER_Log(LOG_LEVEL_WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOGGER_Log(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __LOGGER_H */
