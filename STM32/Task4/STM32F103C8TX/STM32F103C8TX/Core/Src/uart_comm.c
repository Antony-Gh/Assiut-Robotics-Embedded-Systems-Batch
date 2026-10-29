/**
 * @file uart_comm.c
 * @brief Implementation of Virtual Terminal UART Telemetry Logger
 * @author Assiut Robotics Team
 * @date 2026-10-29
 */

#include "uart_comm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static UART_HandleTypeDef *g_huart = NULL;

/**
 * @brief Initializes USART1 for serial transmission to Proteus Virtual Terminal.
 * @param[in] huart Pointer to HAL UART Handle.
 * @retval HAL_StatusTypeDef HAL_OK if initialized successfully.
 */
HAL_StatusTypeDef UART_Comm_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return HAL_ERROR;
    g_huart = huart;
    return HAL_OK;
}

/**
 * @brief Sends a null-terminated string over USART1.
 * @param[in] str Pointer to string buffer.
 * @retval None
 */
void UART_Print(const char *str)
{
    if (str == NULL || g_huart == NULL) return;
    HAL_UART_Transmit(g_huart, (const uint8_t *)str, (uint16_t)strlen(str), 500);
}

/**
 * @brief Helper to format float values with 2 decimal places reliably without float-printf dependencies.
 */
static void FormatFloat(char *buf, size_t max_len, float val, int decimals)
{
    if (val < 0.0f)
    {
        *buf++ = '-';
        val = -val;
        max_len--;
    }
    int int_part = (int)val;
    float frac_part = val - (float)int_part;

    if (decimals == 1)
    {
        int frac = (int)(frac_part * 10.0f + 0.5f);
        if (frac >= 10) { int_part++; frac = 0; }
        snprintf(buf, max_len, "%d.%d", int_part, frac);
    }
    else
    {
        int frac = (int)(frac_part * 100.0f + 0.5f);
        if (frac >= 100) { int_part++; frac = 0; }
        snprintf(buf, max_len, "%d.%02d", int_part, frac);
    }
}

/**
 * @brief Sends formatted sensor telemetry data to the Proteus Virtual Terminal.
 * @param[in] mpu Pointer to MPU6050 device handle.
 * @param[in] bmp Pointer to BMP device handle.
 * @retval None
 */
void UART_SendTelemetry(const MPU6050_t *mpu, const BMP_t *bmp)
{
    char roll_str[16], pitch_str[16], yaw_str[16];
    char temp_str[16], press_str[16], alt_str[16];
    char mpu_temp_str[16];
    char buffer[512];

    /* Format MPU6050 Angles */
    FormatFloat(roll_str, sizeof(roll_str), mpu->roll, 2);
    FormatFloat(pitch_str, sizeof(pitch_str), mpu->pitch, 2);
    FormatFloat(yaw_str, sizeof(yaw_str), mpu->yaw, 2);
    FormatFloat(mpu_temp_str, sizeof(mpu_temp_str), mpu->temperature, 1);

    /* Format BMP Pressure & Temperature */
    FormatFloat(temp_str, sizeof(temp_str), bmp->temperature, 2);
    FormatFloat(press_str, sizeof(press_str), bmp->pressure_hpa, 2);
    FormatFloat(alt_str, sizeof(alt_str), bmp->altitude, 1);

    /* 1. Header Banner */
    UART_Print("\r\n=======================================================\r\n");
    UART_Print("   [TASK 4] SENSOR TELEMETRY (I2C + UART MONITOR)     \r\n");
    UART_Print("=======================================================\r\n");

    /* 2. MPU-6050 Section (Task 1 Deliverable) */
    snprintf(buffer, sizeof(buffer),
             "[1] MPU-6050 (Orientation & Gyro/Accel):\r\n"
             "    * Roll   (X) : %s deg\r\n"
             "    * Pitch  (Y) : %s deg\r\n"
             "    * Yaw    (Z) : %s deg\r\n"
             "    * Accel (g)  : [X:%d, Y:%d, Z:%d]\r\n"
             "    * Gyro (dps) : [X:%d, Y:%d, Z:%d]\r\n"
             "    * Die Temp   : %s C\r\n",
             roll_str, pitch_str, yaw_str,
             (int)(mpu->ax * 100), (int)(mpu->ay * 100), (int)(mpu->az * 100),
             (int)mpu->gx, (int)mpu->gy, (int)mpu->gz,
             mpu_temp_str);
    UART_Print(buffer);

    /* 3. BMP280 Section (Task 2 Deliverable) */
    UART_Print("-------------------------------------------------------\r\n");
    snprintf(buffer, sizeof(buffer),
             "[2] BMP280 / BMP180 (Environmental Sensor):\r\n"
             "    * Temperature : %s C\r\n"
             "    * Pressure    : %s hPa (%d Pa)\r\n"
             "    * Altitude    : %s m\r\n",
             temp_str, press_str, (int)bmp->pressure_pa, alt_str);
    UART_Print(buffer);

    UART_Print("=======================================================\r\n");
}
