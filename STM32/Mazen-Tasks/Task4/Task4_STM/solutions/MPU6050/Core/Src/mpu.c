/*
 * mpu.c
 *
 *  Created on: Oct 23, 2026
 *      Author: m0101
 */

#include "mpu.h"

#include "../Inc/main.h"
#include "stdio.h"
#include "math.h"



uint16_t tlast = 0;
uint16_t t = 0;
uint8_t alpha = 0.98;


HAL_StatusTypeDef checkMPU(hmpu* h)
{
	HAL_StatusTypeDef e = HAL_I2C_Master_Transmit(h->hi2cx, MPU_7_bit_ADD << 1, (uint8_t*)MPU_WHO_AM_I_ADD , 1, 1000);
	if(e == HAL_OK)
		HAL_UART_Transmit(&huart1, (uint8_t* )"OK" , 3, 1000);
	else if(e == HAL_ERROR)
		HAL_UART_Transmit(&huart1, (uint8_t* )"ERR" , 4, 1000);
	else if(e == HAL_BUSY)
		HAL_UART_Transmit(&huart1, (uint8_t* )"BUS" , 4, 1000);
	else if(e == HAL_TIMEOUT)
		HAL_UART_Transmit(&huart1, (uint8_t* )"TIM" , 4, 1000);
	else
		HAL_UART_Transmit(&huart1, (uint8_t* )"NAC" , 4, 1000);

	return e;

//	HAL_StatusTypeDef e;
//	uint8_t who_am_i = 0;
//	e = HAL_I2C_Mem_Read(
//	    h->hi2cx,
//	    (0x68 << 1),
//	    0x75,
//	    I2C_MEMADD_SIZE_8BIT,
//	    &who_am_i,
//	    1,
//	    1000
//	);
//
//	if (e == HAL_OK)
//	{
//	    char msg[40];
//
//	    snprintf(msg, sizeof(msg),
//	             "WHO_AM_I = 0x%02X\r\n", who_am_i);
//
//	    HAL_UART_Transmit(&huart1,


//	                      (uint8_t *)msg,
//	                      40,
//	                      1000);
//	}
//	else
//	{
//	    uint32_t err = HAL_I2C_GetError(h->hi2cx);
//
//	    char msg[50];
//
//	    snprintf(msg, sizeof(msg),
//	             "Status=%d, I2C_Error=0x%lX\r\n",
//	             (int)e, (unsigned long)err);
//
//	    HAL_UART_Transmit(&huart1,
//	                      (uint8_t *)msg,
//	                      40,
//	                      1000);
//	}
}


HAL_StatusTypeDef MPU_init(hmpu* h)
{
	if(checkMPU(h) == HAL_OK)
	{
		HAL_StatusTypeDef e = HAL_I2C_Mem_Write(
			    h->hi2cx,
			    MPU_7_bit_ADD << 1,
				PWR_MGMT_1,                  // PWR_MGMT_1
			    I2C_MEMADD_SIZE_8BIT,
			    (uint8_t *)0x00,
			    1,
			    100
			);
		if(e != HAL_OK)
			HAL_UART_Transmit(&huart1, (uint8_t* )"ERR1" , 5, 1000);
		 e = MPU_read(h);

	}
}

HAL_StatusTypeDef MPU_read(hmpu* h){

	uint8_t data[14];

	HAL_StatusTypeDef e = HAL_I2C_Mem_Read(h->hi2cx, MPU_7_bit_ADD << 1, MPU_DATA_BASE_ADD, 1, data, 14, 1000);
	if(e != HAL_OK)
		HAL_UART_Transmit(&huart1, (uint8_t* )"ERR2" , 5, 1000);
	else
		HAL_UART_Transmit(&huart1, (uint8_t* )"Correct" , 10, 1000);

	h->ax =
	    (int16_t)((data[0] << 8) | data[1]);

	h->ay =
	    (int16_t)((data[2] << 8) | data[3]);

	h->az =
	    (int16_t)((data[4] << 8) | data[5]);

	h->temp=
	    (int16_t)((data[6] << 8) | data[7]);

	h->gx =
	    (int16_t)((data[8] << 8) | data[9]);

	h->gy =
	    (int16_t)((data[10] << 8) | data[11]);

	h->gz =
	    (int16_t)((data[12] << 8) | data[13]);
	fuse(h);
}


void fuse(hmpu* h)
{
	 t=  HAL_GetTick();
	 uint16_t dt = t - tlast;
	 tlast = t;


	 float ax = h->ax / ACCEL_SCALE;
	 float ay = h->ay / ACCEL_SCALE;
	 float az = h->az / ACCEL_SCALE;

	 float gx = h->gx / GYRO_SCALE;
	 float gy = h->gy / GYRO_SCALE;
	 float gz = h->gz / GYRO_SCALE;

	 float accel_roll =
	 atan2f(ay, az) * RAD_TO_DEG;

	 float accel_pitch =
	 atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;


	 h->roll =
	         alpha * (  h->roll + gx * dt)
	         + (1.0f - alpha) * accel_roll;

	 h->pitch =
	         alpha * ( h->pitch + gy * dt)
	         + (1.0f - alpha) * accel_pitch;

}
