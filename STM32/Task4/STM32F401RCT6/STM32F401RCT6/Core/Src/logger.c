/**
  ******************************************************************************
  * @file           : logger.c
  * @brief          : High-Efficiency DMA Ring-Buffer Logger implementation
  *                   STM32F401 HAL
  ******************************************************************************
  */

#include "logger.h"
#include <string.h>

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t s_ring_buffer[LOGGER_BUFFER_SIZE];
static volatile uint16_t s_head = 0;
static volatile uint16_t s_tail = 0;
static volatile bool s_dma_busy = false;

static uint8_t s_dma_tx_buf[LOGGER_BUFFER_SIZE];

static void StartDmaTransfer(void) {
    if (s_huart == NULL) return;

    if (s_dma_busy) {
        /* Auto-recover if UART hardware has returned to ready state */
        if (s_huart->gState == HAL_UART_STATE_READY) {
            s_dma_busy = false;
        } else {
            return;
        }
    }

    if (s_head == s_tail) {
        return;
    }

    uint16_t count = 0;
    while (s_tail != s_head) {
        s_dma_tx_buf[count++] = s_ring_buffer[s_tail];
        s_tail = (s_tail + 1) % LOGGER_BUFFER_SIZE;
    }

    s_dma_busy = true;
    if (HAL_UART_Transmit_DMA(s_huart, s_dma_tx_buf, count) != HAL_OK) {
        /* Fallback if DMA is not configured or fails */
        HAL_UART_Transmit(s_huart, s_dma_tx_buf, count, 50);
        s_dma_busy = false;
    }
}

void LOGGER_Init(UART_HandleTypeDef *huart) {
    s_huart = huart;
    s_head = 0;
    s_tail = 0;
    s_dma_busy = false;
}

void LOGGER_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == s_huart) {
        s_dma_busy = false;
        StartDmaTransfer();
    }
}

static void EnqueueBytes(const uint8_t *data, uint16_t len) {
    /* 1. Also transmit via UART2 DMA if initialized */
    if (s_huart != NULL) {
        for (uint16_t i = 0; i < len; i++) {
            uint16_t next_head = (s_head + 1) % LOGGER_BUFFER_SIZE;
            if (next_head == s_tail) {
                /* Buffer full, drop oldest byte */
                s_tail = (s_tail + 1) % LOGGER_BUFFER_SIZE;
            }
            s_ring_buffer[s_head] = data[i];
            s_head = next_head;
        }

        if (!s_dma_busy) {
            StartDmaTransfer();
        }
    }
}

void LOGGER_Log(LogLevel_t level, const char *fmt, ...) {
    char msg_buf[256];
    char out_buf[300];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    const char *lvl_str = "INFO";
    switch (level) {
        case LOG_LEVEL_DEBUG: lvl_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:  lvl_str = "INFO";  break;
        case LOG_LEVEL_WARN:  lvl_str = "WARN";  break;
        case LOG_LEVEL_ERROR: lvl_str = "ERROR"; break;
    }

    uint32_t ms = HAL_GetTick();
    int len = snprintf(out_buf, sizeof(out_buf), "[%lu ms] [%s] %s\r\n", ms, lvl_str, msg_buf);
    if (len > 0) {
        EnqueueBytes((const uint8_t *)out_buf, (uint16_t)len);
    }
}

#include <math.h>

/* Robust float to string formatter (bypasses newlib-nano float printf limitation) */
void ftoa_custom(char *buf, float val, int decimals) {
    if (isnan(val) || isinf(val)) {
        strcpy(buf, "0.0");
        return;
    }

    if (val < 0.0f) {
        *buf++ = '-';
        val = -val;
    }

    /* Rounding offset */
    float round_off = 0.5f;
    for (int i = 0; i < decimals; i++) {
        round_off /= 10.0f;
    }
    val += round_off;

    uint32_t int_part = (uint32_t)val;
    float frac_val = val - (float)int_part;

    if (decimals == 1) {
        uint32_t frac_part = (uint32_t)(frac_val * 10.0f);
        if (frac_part >= 10) frac_part = 9;
        sprintf(buf, "%lu.%01lu", (unsigned long)int_part, (unsigned long)frac_part);
    } else if (decimals == 2) {
        uint32_t frac_part = (uint32_t)(frac_val * 100.0f);
        if (frac_part >= 100) frac_part = 99;
        sprintf(buf, "%lu.%02lu", (unsigned long)int_part, (unsigned long)frac_part);
    } else {
        sprintf(buf, "%lu", (unsigned long)int_part);
    }
}

void LOGGER_SendTelemetryJSON(const MPU6050_Data_t *gy521, const GY87_Data_t *gy87, const MS5540C_Data_t *ms5540) {
    char s_g5_ax[16], s_g5_ay[16], s_g5_az[16], s_g5_gx[16], s_g5_gy[16], s_g5_gz[16], s_g5_roll[16], s_g5_pitch[16];
    char s_g8_ax[16], s_g8_ay[16], s_g8_az[16], s_g8_gx[16], s_g8_gy[16], s_g8_gz[16];
    char s_g8_mx[16], s_g8_my[16], s_g8_mz[16], s_g8_hdg[16], s_g8_press[16], s_g8_alt[16];
    char s_ms_press[16], s_ms_temp[16], s_ms_depth[16];

    ftoa_custom(s_g5_ax, gy521->ax_g, 2);
    ftoa_custom(s_g5_ay, gy521->ay_g, 2);
    ftoa_custom(s_g5_az, gy521->az_g, 2);
    ftoa_custom(s_g5_gx, gy521->gx_dps, 1);
    ftoa_custom(s_g5_gy, gy521->gy_dps, 1);
    ftoa_custom(s_g5_gz, gy521->gz_dps, 1);
    ftoa_custom(s_g5_roll, gy521->roll_deg, 1);
    ftoa_custom(s_g5_pitch, gy521->pitch_deg, 1);

    ftoa_custom(s_g8_ax, gy87->ax_g, 2);
    ftoa_custom(s_g8_ay, gy87->ay_g, 2);
    ftoa_custom(s_g8_az, gy87->az_g, 2);
    ftoa_custom(s_g8_gx, gy87->gx_dps, 1);
    ftoa_custom(s_g8_gy, gy87->gy_dps, 1);
    ftoa_custom(s_g8_gz, gy87->gz_dps, 1);
    ftoa_custom(s_g8_mx, gy87->mx_uT, 1);
    ftoa_custom(s_g8_my, gy87->my_uT, 1);
    ftoa_custom(s_g8_mz, gy87->mz_uT, 1);
    ftoa_custom(s_g8_hdg, gy87->heading_deg, 1);
    ftoa_custom(s_g8_press, gy87->pressure_hpa, 1);
    ftoa_custom(s_g8_alt, gy87->altitude_m, 1);

    ftoa_custom(s_ms_press, ms5540->pressure_mbar, 1);
    ftoa_custom(s_ms_temp, ms5540->temperature_c, 1);
    ftoa_custom(s_ms_depth, ms5540->depth_meters, 2);

    char json_buf[512];
    uint32_t ts = HAL_GetTick();

    int len = snprintf(json_buf, sizeof(json_buf),
        "{\"ts\":%lu,"
        "\"gy521\":{\"ax\":%s,\"ay\":%s,\"az\":%s,\"gx\":%s,\"gy\":%s,\"gz\":%s,\"roll\":%s,\"pitch\":%s},"
        "\"gy87\":{\"ax\":%s,\"ay\":%s,\"az\":%s,\"gx\":%s,\"gy\":%s,\"gz\":%s,\"mx\":%s,\"my\":%s,\"mz\":%s,\"hdg\":%s,\"press\":%s,\"alt\":%s},"
        "\"ms5540\":{\"press\":%s,\"temp\":%s,\"depth\":%s}}\r\n",
        (unsigned long)ts,
        s_g5_ax, s_g5_ay, s_g5_az, s_g5_gx, s_g5_gy, s_g5_gz, s_g5_roll, s_g5_pitch,
        s_g8_ax, s_g8_ay, s_g8_az, s_g8_gx, s_g8_gy, s_g8_gz,
        s_g8_mx, s_g8_my, s_g8_mz, s_g8_hdg, s_g8_press, s_g8_alt,
        s_ms_press, s_ms_temp, s_ms_depth
    );

    if (len > 0) {
        EnqueueBytes((const uint8_t *)json_buf, (uint16_t)len);
    }
}

void LOGGER_SendTelemetryCSV(const MPU6050_Data_t *gy521, const GY87_Data_t *gy87, const MS5540C_Data_t *ms5540) {
    char s_ax[16], s_ay[16], s_az[16], s_gx[16], s_gy[16], s_gz[16], s_hdg[16], s_press[16], s_temp[16], s_depth[16];

    ftoa_custom(s_ax, gy87->ax_g, 2);
    ftoa_custom(s_ay, gy87->ay_g, 2);
    ftoa_custom(s_az, gy87->az_g, 2);
    ftoa_custom(s_gx, gy87->gx_dps, 1);
    ftoa_custom(s_gy, gy87->gy_dps, 1);
    ftoa_custom(s_gz, gy87->gz_dps, 1);
    ftoa_custom(s_hdg, gy87->heading_deg, 1);
    ftoa_custom(s_press, ms5540->pressure_mbar, 1);
    ftoa_custom(s_temp, ms5540->temperature_c, 1);
    ftoa_custom(s_depth, ms5540->depth_meters, 2);

    char csv_buf[256];
    uint32_t ts = HAL_GetTick();

    int len = snprintf(csv_buf, sizeof(csv_buf),
        "$ROV,%lu,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n",
        (unsigned long)ts,
        s_ax, s_ay, s_az,
        s_gx, s_gy, s_gz,
        s_hdg,
        s_press,
        s_temp,
        s_depth
    );

    if (len > 0) {
        EnqueueBytes((const uint8_t *)csv_buf, (uint16_t)len);
    }
}
