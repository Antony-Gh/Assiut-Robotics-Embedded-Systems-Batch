/**
  ******************************************************************************
  * @file           : hmc5883l.h
  * @brief          : Unified Driver for HMC5883L and QMC5883L Compass / Mag
  *                   STM32F401 HAL Driver with Auto-Detection
  ******************************************************************************
  */

#ifndef __HMC5883L_H
#define __HMC5883L_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* Chip Types */
typedef enum {
    MAG_CHIP_UNKNOWN = 0,
    MAG_CHIP_HMC5883L,
    MAG_CHIP_QMC5883L
} Mag_ChipType_t;

/* HMC5883L Registers */
#define HMC5883L_REG_CONFIG_A           0x00
#define HMC5883L_REG_CONFIG_B           0x01
#define HMC5883L_REG_MODE               0x02
#define HMC5883L_REG_OUT_X_M            0x03
#define HMC5883L_REG_STATUS             0x09
#define HMC5883L_REG_ID_A               0x0A
#define HMC5883L_REG_ID_B               0x0B
#define HMC5883L_REG_ID_C               0x0C

/* QMC5883L Registers */
#define QMC5883L_REG_OUT_X_L            0x00
#define QMC5883L_REG_STATUS             0x06
#define QMC5883L_REG_CONTROL_1          0x09
#define QMC5883L_REG_CONTROL_2          0x0A
#define QMC5883L_REG_SET_RESET          0x0B
#define QMC5883L_REG_CHIP_ID            0x0D

/* Magnetometer Data Structure */
typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;

    float mx_uT;            /* Magnetic field in microteslas */
    float my_uT;
    float mz_uT;

    float heading_deg;      /* Computed compass heading (0.0 to 359.9 degrees) */
} HMC5883L_Data_t;

/* Magnetometer Device Handle */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
    Mag_ChipType_t chip_type;

    float declination_deg;  /* Local magnetic declination angle */

    /* Hard-iron calibration offsets */
    int16_t offset_x;
    int16_t offset_y;
    int16_t offset_z;

    /* Sensitivity scale factors */
    float scale_x;
    float scale_y;
    float scale_z;

    HMC5883L_Data_t data;
    bool is_initialized;
} HMC5883L_t;

/* Public API */
HAL_StatusTypeDef HMC5883L_Init(HMC5883L_t *dev, I2C_HandleTypeDef *hi2c, float declination_deg);
HAL_StatusTypeDef HMC5883L_Read(HMC5883L_t *dev);
HAL_StatusTypeDef HMC5883L_Calibrate(HMC5883L_t *dev, uint16_t duration_ms);
void HMC5883L_SetDeclination(HMC5883L_t *dev, float declination_deg);

#ifdef __cplusplus
}
#endif

#endif /* __HMC5883L_H */
