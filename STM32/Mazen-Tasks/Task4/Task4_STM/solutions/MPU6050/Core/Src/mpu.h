/*
 * mpu.h
 *
 *  Created on: Oct 23, 2026
 *      Author: m0101
 */

#ifndef SRC_MPU_H_
#define SRC_MPU_H_
#include "stm32f1xx_hal.h"
#define MPU_7_bit_ADD 0x68
#define MPU_WHO_AM_I_ADD 0x75
#define PWR_MGMT_1 0x6B
#define MPU_DATA_BASE_ADD 0x3B


#define ACCEL_SCALE  16384.0f
#define GYRO_SCALE   131.0f
#define RAD_TO_DEG   57.2957795f
#define PI_F         3.14159265f

typedef struct{
	I2C_HandleTypeDef* hi2cx;
	uint16_t ax;
	uint16_t ay;
	uint16_t az;
	uint16_t gx;
	uint16_t gy;
	uint16_t gz;
	uint16_t temp;
	int16_t pitch;
	int16_t roll;
	int16_t yaw;
}hmpu;



uint8_t checkMPU(hmpu* h);
HAL_StatusTypeDef MPU_init(hmpu* h);
HAL_StatusTypeDef MPU_read(hmpu* h);
void fuse(hmpu* h);
#endif /* SRC_MPU_H_ */
