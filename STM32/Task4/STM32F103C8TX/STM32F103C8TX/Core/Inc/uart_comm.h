/**
 * @file uart_comm.h
 * @brief Virtual Terminal UART Telemetry Logger for STM32
 * @author Assiut Robotics Team
 * @date 2026-10-29
 */

#ifndef __UART_COMM_H__
#define __UART_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "mpu6050.h"
#include "bmp280.h"

/**
 * @brief Initializes USART1 for serial transmission to Proteus Virtual Terminal (9600 baud, 8-N-1).
 * @param[in] huart Pointer to HAL UART Handle.
 * @retval HAL_StatusTypeDef HAL_OK if initialized successfully.
 */
HAL_StatusTypeDef UART_Comm_Init(UART_HandleTypeDef *huart);

/**
 * @brief Sends a null-terminated string over USART1.
 * @param[in] str Pointer to string buffer.
 * @retval None
 */
void UART_Print(const char *str);

/**
 * @brief Sends formatted sensor telemetry data to the Proteus Virtual Terminal.
 * @param[in] mpu Pointer to MPU6050 device handle.
 * @param[in] bmp Pointer to BMP device handle.
 * @retval None
 */
void UART_SendTelemetry(const MPU6050_t *mpu, const BMP_t *bmp);

#ifdef __cplusplus
}
#endif

#endif /* __UART_COMM_H__ */
