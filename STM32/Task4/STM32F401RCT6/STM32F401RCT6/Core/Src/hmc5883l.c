/**
  ******************************************************************************
  * @file           : hmc5883l.c
  * @brief          : Unified Driver implementation for HMC5883L and QMC5883L
  *                   STM32F401 HAL with Auto-Detection
  ******************************************************************************
  */

#include "hmc5883l.h"
#include <math.h>

#define RAD_TO_DEG 57.29577951308232f

static HAL_StatusTypeDef Mag_WriteReg(HMC5883L_t *dev, uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 50);
}

static HAL_StatusTypeDef Mag_ReadRegs(HMC5883L_t *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 50);
}

HAL_StatusTypeDef HMC5883L_Init(HMC5883L_t *dev, I2C_HandleTypeDef *hi2c, float declination_deg) {
    dev->hi2c = hi2c;
    dev->declination_deg = declination_deg;
    dev->is_initialized = false;
    dev->chip_type = MAG_CHIP_UNKNOWN;

    dev->offset_x = 0;
    dev->offset_y = 0;
    dev->offset_z = 0;

    dev->scale_x = 1.0f;
    dev->scale_y = 1.0f;
    dev->scale_z = 1.0f;

    /* 1. First probe for authentic Honeywell HMC5883L at I2C address 0x1E (0x3C in 8-bit) */
    dev->i2c_addr = (0x1E << 1);
    uint8_t id_buf[3] = {0};

    if (Mag_ReadRegs(dev, HMC5883L_REG_ID_A, id_buf, 3) == HAL_OK) {
        if (id_buf[0] == 0x48 && id_buf[1] == 0x34 && id_buf[2] == 0x33) {
            /* Authentic HMC5883L ("H43") detected */
            dev->chip_type = MAG_CHIP_HMC5883L;

            /* Config A: 8-average, 75 Hz data output rate, normal measurement */
            Mag_WriteReg(dev, HMC5883L_REG_CONFIG_A, 0x78);

            /* Config B: Gain = 1090 LSB/Gauss (+/- 1.3 Gauss default range) */
            Mag_WriteReg(dev, HMC5883L_REG_CONFIG_B, 0x20);

            /* Mode: Continuous-measurement mode */
            Mag_WriteReg(dev, HMC5883L_REG_MODE, 0x00);

            dev->is_initialized = true;
            return HAL_OK;
        }
    }

    /* 2. If not HMC5883L, probe for QMC5883L clone at I2C address 0x0D (0x1A in 8-bit) */
    dev->i2c_addr = (0x0D << 1);
    uint8_t qmc_id = 0;

    if (Mag_ReadRegs(dev, QMC5883L_REG_CHIP_ID, &qmc_id, 1) == HAL_OK) {
        /* QMC5883L detected (ID usually 0xFF, but any ACK at 0x0D is QMC) */
        dev->chip_type = MAG_CHIP_QMC5883L;

            /* Soft reset */
            Mag_WriteReg(dev, QMC5883L_REG_CONTROL_2, 0x80);
            HAL_Delay(10);

            /* Set SET/RESET period register (0x01 per datasheet) */
            Mag_WriteReg(dev, QMC5883L_REG_SET_RESET, 0x01);

            /* Control Register 1:
             * Mode: Continuous (0x01)
             * Output Data Rate: 100 Hz (0x08)
             * Full Scale Range: 2 Gauss (0x00) (or 8G = 0x10)
             * Over Sample Ratio: 512 (0x00)
             * Value: 0x01 | 0x08 | 0x00 | 0x00 = 0x09 (or 0x1D for 200Hz, 8G)
             */
            Mag_WriteReg(dev, QMC5883L_REG_CONTROL_1, 0x1D);

            dev->is_initialized = true;
            return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef HMC5883L_Read(HMC5883L_t *dev) {
    if (!dev->is_initialized) return HAL_ERROR;

    uint8_t buf[6];

    if (dev->chip_type == MAG_CHIP_HMC5883L) {
        /* HMC5883L stores: X_MSB, X_LSB, Z_MSB, Z_LSB, Y_MSB, Y_LSB (Big Endian, Z before Y) */
        if (Mag_ReadRegs(dev, HMC5883L_REG_OUT_X_M, buf, 6) != HAL_OK) {
            return HAL_ERROR;
        }

        dev->data.raw_x = (int16_t)((buf[0] << 8) | buf[1]);
        dev->data.raw_z = (int16_t)((buf[2] << 8) | buf[3]);
        dev->data.raw_y = (int16_t)((buf[4] << 8) | buf[5]);

        /* HMC5883L with +/-1.3 Gauss has 1090 LSB/Gauss (1 Gauss = 100 uT -> 1 uT = 10.90 LSB) */
        dev->data.mx_uT = ((float)(dev->data.raw_x - dev->offset_x) * dev->scale_x) / 10.90f;
        dev->data.my_uT = ((float)(dev->data.raw_y - dev->offset_y) * dev->scale_y) / 10.90f;
        dev->data.mz_uT = ((float)(dev->data.raw_z - dev->offset_z) * dev->scale_z) / 10.90f;

    } else if (dev->chip_type == MAG_CHIP_QMC5883L) {
        /* QMC5883L stores: X_LSB, X_MSB, Y_LSB, Y_MSB, Z_LSB, Z_MSB (Little Endian, X, Y, Z) */
        if (Mag_ReadRegs(dev, QMC5883L_REG_OUT_X_L, buf, 6) != HAL_OK) {
            return HAL_ERROR;
        }

        dev->data.raw_x = (int16_t)((buf[1] << 8) | buf[0]);
        dev->data.raw_y = (int16_t)((buf[3] << 8) | buf[2]);
        dev->data.raw_z = (int16_t)((buf[5] << 8) | buf[4]);

        /* QMC5883L with +/-8 Gauss has 3000 LSB/Gauss (1 uT = 30.0 LSB) */
        dev->data.mx_uT = ((float)(dev->data.raw_x - dev->offset_x) * dev->scale_x) / 30.0f;
        dev->data.my_uT = ((float)(dev->data.raw_y - dev->offset_y) * dev->scale_y) / 30.0f;
        dev->data.mz_uT = ((float)(dev->data.raw_z - dev->offset_z) * dev->scale_z) / 30.0f;
    } else {
        return HAL_ERROR;
    }

    /* Calculate Compass Heading in degrees */
    float heading = atan2f(dev->data.my_uT, dev->data.mx_uT) * RAD_TO_DEG;

    /* Add magnetic declination compensation */
    heading += dev->declination_deg;

    /* Normalize to 0 - 360 degrees */
    if (heading < 0.0f) {
        heading += 360.0f;
    } else if (heading >= 360.0f) {
        heading -= 360.0f;
    }

    dev->data.heading_deg = heading;
    return HAL_OK;
}

HAL_StatusTypeDef HMC5883L_Calibrate(HMC5883L_t *dev, uint16_t duration_ms) {
    if (!dev->is_initialized) return HAL_ERROR;

    int16_t min_x = 32767, max_x = -32768;
    int16_t min_y = 32767, max_y = -32768;
    int16_t min_z = 32767, max_z = -32768;

    uint32_t start_tick = HAL_GetTick();

    while ((HAL_GetTick() - start_tick) < duration_ms) {
        if (HMC5883L_Read(dev) == HAL_OK) {
            if (dev->data.raw_x < min_x) min_x = dev->data.raw_x;
            if (dev->data.raw_x > max_x) max_x = dev->data.raw_x;

            if (dev->data.raw_y < min_y) min_y = dev->data.raw_y;
            if (dev->data.raw_y > max_y) max_y = dev->data.raw_y;

            if (dev->data.raw_z < min_z) min_z = dev->data.raw_z;
            if (dev->data.raw_z > max_z) max_z = dev->data.raw_z;
        }
        HAL_Delay(10);
    }

    /* Compute Hard-iron offsets */
    dev->offset_x = (min_x + max_x) / 2;
    dev->offset_y = (min_y + max_y) / 2;
    dev->offset_z = (min_z + max_z) / 2;

    /* Soft-iron scaling factors */
    float avg_delta_x = (float)(max_x - min_x) / 2.0f;
    float avg_delta_y = (float)(max_y - min_y) / 2.0f;
    float avg_delta_z = (float)(max_z - min_z) / 2.0f;
    float avg_delta = (avg_delta_x + avg_delta_y + avg_delta_z) / 3.0f;

    if (avg_delta_x > 0.0f) dev->scale_x = avg_delta / avg_delta_x;
    if (avg_delta_y > 0.0f) dev->scale_y = avg_delta / avg_delta_y;
    if (avg_delta_z > 0.0f) dev->scale_z = avg_delta / avg_delta_z;

    return HAL_OK;
}

void HMC5883L_SetDeclination(HMC5883L_t *dev, float declination_deg) {
    dev->declination_deg = declination_deg;
}
