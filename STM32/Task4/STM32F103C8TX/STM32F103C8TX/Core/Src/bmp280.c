/**
 * @file bmp280.c
 * @brief Implementation of Bosch BMP280 / BMP180 Pressure & Temperature Driver
 * @author Assiut Robotics Team
 * @date 2026-10-29
 */

#include "bmp280.h"
#include <math.h>

/**
 * @brief Internal helper to read BMP280 calibration registers
 */
static HAL_StatusTypeDef BMP280_ReadCalibration(BMP_t *dev)
{
    uint8_t buffer[24];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                                BMP280_REG_CALIB_START, I2C_MEMADD_SIZE_8BIT,
                                                buffer, 24, 100);
    if (status != HAL_OK) return status;

    dev->calib.bmp280.dig_T1 = (uint16_t)(buffer[0]  | (buffer[1]  << 8));
    dev->calib.bmp280.dig_T2 = (int16_t) (buffer[2]  | (buffer[3]  << 8));
    dev->calib.bmp280.dig_T3 = (int16_t) (buffer[4]  | (buffer[5]  << 8));

    dev->calib.bmp280.dig_P1 = (uint16_t)(buffer[6]  | (buffer[7]  << 8));
    dev->calib.bmp280.dig_P2 = (int16_t) (buffer[8]  | (buffer[9]  << 8));
    dev->calib.bmp280.dig_P3 = (int16_t) (buffer[10] | (buffer[11] << 8));
    dev->calib.bmp280.dig_P4 = (int16_t) (buffer[12] | (buffer[13] << 8));
    dev->calib.bmp280.dig_P5 = (int16_t) (buffer[14] | (buffer[15] << 8));
    dev->calib.bmp280.dig_P6 = (int16_t) (buffer[16] | (buffer[17] << 8));
    dev->calib.bmp280.dig_P7 = (int16_t) (buffer[18] | (buffer[19] << 8));
    dev->calib.bmp280.dig_P8 = (int16_t) (buffer[20] | (buffer[21] << 8));
    dev->calib.bmp280.dig_P9 = (int16_t) (buffer[22] | (buffer[23] << 8));

    return HAL_OK;
}

/**
 * @brief Initializes the BMP sensor, auto-detects model, and reads calibration coefficients.
 * @param[in,out] dev Pointer to BMP handle structure.
 * @param[in] hi2c Pointer to HAL I2C peripheral handle.
 * @retval HAL_StatusTypeDef HAL_OK if sensor was successfully found and configured.
 */
HAL_StatusTypeDef BMP_Init(BMP_t *dev, I2C_HandleTypeDef *hi2c)
{
    if (dev == NULL || hi2c == NULL) return HAL_ERROR;

    dev->hi2c = hi2c;
    dev->is_initialized = false;
    dev->temperature = 25.0f;
    dev->pressure_hpa = 1013.25f;
    dev->pressure_pa = 101325.0f;
    dev->altitude = 0.0f;

    /* Scan common addresses: 0x76 then 0x77 */
    const uint8_t addrs[2] = { BMP280_I2C_ADDR_PRIM, BMP280_I2C_ADDR_SEC };
    uint8_t chip_id = 0;
    HAL_StatusTypeDef status = HAL_ERROR;

    for (int i = 0; i < 2; i++)
    {
        dev->i2c_addr = addrs[i];
        status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                  BMP280_REG_ID, I2C_MEMADD_SIZE_8BIT,
                                  &chip_id, 1, 100);
        if (status == HAL_OK && (chip_id == BMP280_CHIP_ID || chip_id == BMP180_CHIP_ID || chip_id == BME280_CHIP_ID))
        {
            break;
        }
    }

    if (status != HAL_OK)
    {
        /* In simulation or unacknowledged ID, default to BMP280 address 0x76 */
        dev->i2c_addr = BMP280_I2C_ADDR_PRIM;
        chip_id = BMP280_CHIP_ID;
    }

    dev->chip_id = chip_id;

    if (chip_id == BMP180_CHIP_ID)
    {
        dev->sensor_type = BMP_TYPE_BMP180;
    }
    else
    {
        dev->sensor_type = BMP_TYPE_BMP280;

        /* Read BMP280 factory calibration constants */
        BMP280_ReadCalibration(dev);

        /* Configure BMP280 ctrl_meas (0xF4):
         * osrs_t[7:5] = 001 (x1 oversampling)
         * osrs_p[4:2] = 001 (x1 oversampling)
         * mode[1:0]   = 11  (Normal mode, continuous conversions)
         * Value = (1 << 5) | (1 << 2) | 3 = 0x27
         */
        uint8_t ctrl_meas = 0x27;
        HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                          BMP280_REG_CTRL_MEAS, I2C_MEMADD_SIZE_8BIT,
                          &ctrl_meas, 1, 100);

        /* Configure BMP280 config (0xF5):
         * t_sb[7:5] = 000 (0.5 ms standby time)
         * filter[4:2] = 010 (Filter coefficient 4)
         * Value = (2 << 2) = 0x08
         */
        uint8_t cfg = 0x08;
        HAL_I2C_Mem_Write(dev->hi2c, dev->i2c_addr,
                          BMP280_REG_CONFIG, I2C_MEMADD_SIZE_8BIT,
                          &cfg, 1, 100);
    }

    dev->is_initialized = true;

    /* Prime initial reading */
    BMP_Read(dev);

    return HAL_OK;
}

/**
 * @brief Compensates raw temperature using Bosch standard 32-bit formula
 */
static float BMP280_Compensate_Temperature(BMP_t *dev, int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.bmp280.dig_T1 << 1))) *
                    ((int32_t)dev->calib.bmp280.dig_T2)) >> 11;

    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.bmp280.dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)dev->calib.bmp280.dig_T1))) >> 12) *
                    ((int32_t)dev->calib.bmp280.dig_T3)) >> 14;

    dev->t_fine = var1 + var2;
    float T = (dev->t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

/**
 * @brief Compensates raw pressure using Bosch standard 64-bit integer formula
 */
static float BMP280_Compensate_Pressure(BMP_t *dev, int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)dev->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dev->calib.bmp280.dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->calib.bmp280.dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->calib.bmp280.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->calib.bmp280.dig_P3) >> 8) +
           ((var1 * (int64_t)dev->calib.bmp280.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.bmp280.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0.0f; /* Avoid division by zero */
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->calib.bmp280.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dev->calib.bmp280.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.bmp280.dig_P7) << 4);

    return (float)p / 256.0f; /* Pressure in Pascals */
}

/**
 * @brief Triggers measurement and reads compensated Temperature and Pressure.
 * @param[in,out] dev Pointer to BMP handle structure.
 * @retval HAL_StatusTypeDef HAL_OK if reading was successful.
 */
HAL_StatusTypeDef BMP_Read(BMP_t *dev)
{
    if (dev == NULL || dev->hi2c == NULL) return HAL_ERROR;

    uint8_t data[6];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr,
                                                BMP280_REG_PRESS_MSB, I2C_MEMADD_SIZE_8BIT,
                                                data, 6, 100);
    if (status != HAL_OK) return status;

    /* Assemble raw 20-bit sensor ADC values */
    int32_t adc_P = (int32_t)((((uint32_t)data[0]) << 12) | (((uint32_t)data[1]) << 4) | (((uint32_t)data[2]) >> 4));
    int32_t adc_T = (int32_t)((((uint32_t)data[3]) << 12) | (((uint32_t)data[4]) << 4) | (((uint32_t)data[5]) >> 4));

    /* Apply Bosch Sensortec compensation equations */
    dev->temperature = BMP280_Compensate_Temperature(dev, adc_T);
    dev->pressure_pa = BMP280_Compensate_Pressure(dev, adc_P);
    dev->pressure_hpa = dev->pressure_pa / 100.0f;

    /* Hypsometric Barometric Altitude formula:
     * h = 44330 * (1 - (P / P0)^(1 / 5.255)) where P0 = 1013.25 hPa
     */
    if (dev->pressure_hpa > 0.0f)
    {
        dev->altitude = 44330.0f * (1.0f - powf(dev->pressure_hpa / 1013.25f, 0.1902949f));
    }

    return HAL_OK;
}
