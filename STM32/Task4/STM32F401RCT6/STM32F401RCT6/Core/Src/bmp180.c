/**
  ******************************************************************************
  * @file           : bmp180.c
  * @brief          : High-Precision Driver implementation for BMP180 Barometer
  *                   STM32F401 HAL
  ******************************************************************************
  */

#include "bmp180.h"
#include <math.h>

static HAL_StatusTypeDef BMP180_WriteReg(BMP180_t *dev, uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 50);
}

static HAL_StatusTypeDef BMP180_ReadRegs(BMP180_t *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 50);
}

HAL_StatusTypeDef BMP180_Init(BMP180_t *dev, I2C_HandleTypeDef *hi2c, BMP180_OSS_t oss) {
    dev->hi2c = hi2c;
    dev->i2c_addr = BMP180_I2C_ADDR_DEFAULT;
    dev->oss = oss;
    dev->sea_level_pressure_hpa = 1013.25f;
    dev->is_initialized = false;

    /* Verify Chip ID */
    uint8_t chip_id = 0;
    if (BMP180_ReadRegs(dev, BMP180_CHIP_ID_REG, &chip_id, 1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (chip_id != BMP180_CHIP_ID_VAL) {
        return HAL_ERROR;
    }

    /* Read 22 bytes of calibration data from internal EEPROM (0xAA to 0xBF) */
    uint8_t cal_buf[22];
    if (BMP180_ReadRegs(dev, 0xAA, cal_buf, 22) != HAL_OK) {
        return HAL_ERROR;
    }

    dev->calib.ac1 = (int16_t)((cal_buf[0]  << 8) | cal_buf[1]);
    dev->calib.ac2 = (int16_t)((cal_buf[2]  << 8) | cal_buf[3]);
    dev->calib.ac3 = (int16_t)((cal_buf[4]  << 8) | cal_buf[5]);
    dev->calib.ac4 = (uint16_t)((cal_buf[6] << 8) | cal_buf[7]);
    dev->calib.ac5 = (uint16_t)((cal_buf[8] << 8) | cal_buf[9]);
    dev->calib.ac6 = (uint16_t)((cal_buf[10]<< 8) | cal_buf[11]);
    dev->calib.b1  = (int16_t)((cal_buf[12] << 8) | cal_buf[13]);
    dev->calib.b2  = (int16_t)((cal_buf[14] << 8) | cal_buf[15]);
    dev->calib.mb  = (int16_t)((cal_buf[16] << 8) | cal_buf[17]);
    dev->calib.mc  = (int16_t)((cal_buf[18] << 8) | cal_buf[19]);
    dev->calib.md  = (int16_t)((cal_buf[20] << 8) | cal_buf[21]);

    dev->is_initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef BMP180_Read(BMP180_t *dev) {
    if (!dev->is_initialized) return HAL_ERROR;

    /* 1. Trigger Temperature Conversion */
    if (BMP180_WriteReg(dev, BMP180_CTRL_MEAS_REG, BMP180_CMD_READ_TEMP) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(5); /* Wait 4.5ms max */

    uint8_t temp_buf[2];
    if (BMP180_ReadRegs(dev, BMP180_OUT_MSB_REG, temp_buf, 2) != HAL_OK) {
        return HAL_ERROR;
    }
    int32_t ut = (int32_t)((temp_buf[0] << 8) | temp_buf[1]);

    /* 2. Trigger Pressure Conversion with selected OSS */
    uint8_t oss_cmd = BMP180_CMD_READ_PRESS_OSS0 + (dev->oss << 6);
    if (BMP180_WriteReg(dev, BMP180_CTRL_MEAS_REG, oss_cmd) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Wait conversion time according to OSS */
    switch (dev->oss) {
        case BMP180_OSS_ULTRA_LOW_POWER: HAL_Delay(5);  break;
        case BMP180_OSS_STANDARD:        HAL_Delay(8);  break;
        case BMP180_OSS_HIGH_RES:        HAL_Delay(14); break;
        case BMP180_OSS_ULTRA_HIGH_RES:  HAL_Delay(26); break;
        default:                         HAL_Delay(14); break;
    }

    uint8_t press_buf[3];
    if (BMP180_ReadRegs(dev, BMP180_OUT_MSB_REG, press_buf, 3) != HAL_OK) {
        return HAL_ERROR;
    }
    int32_t up = (((int32_t)press_buf[0] << 16) | ((int32_t)press_buf[1] << 8) | (int32_t)press_buf[2]) >> (8 - dev->oss);

    /* 3. Compute True Temperature per Bosch algorithm */
    int32_t x1 = ((ut - (int32_t)dev->calib.ac6) * (int32_t)dev->calib.ac5) >> 15;
    int32_t x2 = ((int32_t)dev->calib.mc << 11) / (x1 + dev->calib.md);
    int32_t b5 = x1 + x2;
    int32_t t = (b5 + 8) >> 4;
    dev->data.temperature_c = (float)t / 10.0f;

    /* 4. Compute True Pressure */
    int32_t b6 = b5 - 4000;
    x1 = (dev->calib.b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (dev->calib.ac2 * b6) >> 11;
    int32_t x3 = x1 + x2;
    int32_t b3 = ((((int32_t)dev->calib.ac1 * 4 + x3) << dev->oss) + 2) >> 2;

    x1 = (dev->calib.ac3 * b6) >> 13;
    x2 = (dev->calib.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    uint32_t b4 = (dev->calib.ac4 * (uint32_t)(x3 + 32768)) >> 15;
    uint32_t b7 = ((uint32_t)up - b3) * (50000 >> dev->oss);

    int32_t p;
    if (b7 < 0x80000000) {
        p = (b7 * 2) / b4;
    } else {
        p = (b7 / b4) * 2;
    }

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);

    dev->data.pressure_pa = (float)p;
    dev->data.pressure_hpa = (float)p / 100.0f;

    /* 5. Calculate Barometric Altitude using standard atmospheric hypsometric formula */
    /* Altitude = 44330 * (1 - (p / p0)^(1 / 5.255)) */
    dev->data.altitude_m = 44330.0f * (1.0f - powf(dev->data.pressure_hpa / dev->sea_level_pressure_hpa, 0.190294957f));

    return HAL_OK;
}

void BMP180_SetSeaLevelPressure(BMP180_t *dev, float sea_level_hpa) {
    if (sea_level_hpa > 800.0f && sea_level_hpa < 1200.0f) {
        dev->sea_level_pressure_hpa = sea_level_hpa;
    }
}
