/*
 * US_int.h
 *
 *  Created on: Oct 9, 2026
 *      Author: m0101
 */

#ifndef US_DRIVER_US_INT_H_
#define US_DRIVER_US_INT_H_


typedef enum{
	CM,
	INCHE
}US_measure;
typedef struct{
	GPIO_TypeDef* port;
		uint32_t pin;
}pin;
typedef struct{
	pin trig;
	pin echo;
	US_measure mode;
}US;
void US_read(US,uint16_t* buffer);
#endif /* US_DRIVER_US_INT_H_ */
