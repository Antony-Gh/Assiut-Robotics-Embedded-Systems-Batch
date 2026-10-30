/**
 * @file uart_comm.c
 * @brief USART Telemetry Transmission Implementation for Task 4
 * @author Assiut Robotics Team
 * @date 2026-10-30
 */

#include "uart_comm.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;
static UART_HandleTypeDef *s_huart = NULL;

void UART_Comm_Init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
}

void UART_SendTelemetry(float roll, float pitch, float yaw, float temp, float press_hpa, float alt_m)
{
    char buf[160];
    int len = snprintf(buf, sizeof(buf),
        "[TELEMETRY] Roll: %+6.1f deg | Pitch: %+6.1f deg | Yaw: %+6.1f deg | Temp: %5.1f C | Press: %7.2f hPa | Alt: %6.1f m\r\n",
        roll, pitch, yaw, temp, press_hpa, alt_m);

    if (len > 0) {
        if (s_huart != NULL) {
            HAL_UART_Transmit(s_huart, (uint8_t *)buf, (uint16_t)len, 100);
        }
        /* Only transmit over USB if device is configured by a host */
        if (hUsbDeviceFS.pClassData != NULL && hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
            CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
        }
    }
}

void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        if (s_huart != NULL) {
            HAL_UART_Transmit(s_huart, (uint8_t *)buf, (uint16_t)len, 100);
        }
        if (hUsbDeviceFS.pClassData != NULL && hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
            CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
        }
    }
}
