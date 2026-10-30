/**
 * @file uart_comm.h
 * @brief USART Telemetry Transmission Interface for Task 4
 * @author Assiut Robotics Team
 * @date 2026-10-30
 */

#ifndef __UART_COMM_H
#define __UART_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void UART_Comm_Init(UART_HandleTypeDef *huart);
void UART_SendTelemetry(float roll, float pitch, float yaw, float temp, float press_hpa, float alt_m);
void UART_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __UART_COMM_H */
