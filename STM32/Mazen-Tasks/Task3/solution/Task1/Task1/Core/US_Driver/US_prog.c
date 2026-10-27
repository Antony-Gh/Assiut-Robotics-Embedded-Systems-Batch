/*
 * US_prog.c
 *
 *  Created on: Oct 9, 2026
 *      Author: m0101
 */
#include "../Inc/main.h"
#include "US_int.h"
uint16_t* read;
US active_us;
uint16_t start = 0;
void US_read(US us,uint16_t* buffer)
{
	active_us = us;
	read = buffer;
	HAL_GPIO_WritePin(us.trig.port, us.trig.pin, SET);
	delay_us(10);
	HAL_GPIO_WritePin(us.trig.port, us.trig.pin, RESET);
	HAL_Delay(200);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == active_us.echo.pin)
	{
		if(HAL_GPIO_ReadPin(active_us.echo.port, active_us.echo.pin))
		{
			start = HAL_GetTick();
		}
		else{
			uint16_t time = HAL_GetTick() - start;
//			*read = time;
			if(active_us.mode == CM)
				*read = time/58;
			else
				*read = time/148;
		}
	}
}
