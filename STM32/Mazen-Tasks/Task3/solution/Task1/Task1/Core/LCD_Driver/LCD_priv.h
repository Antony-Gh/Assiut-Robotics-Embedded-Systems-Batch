/*
 * LCD_priv.h
 *
 *  Created on: Jul 22, 2026
 *      Author: m0101
 */

#ifndef LCD_DRIVER_LCD_PRIV_H_
#define LCD_DRIVER_LCD_PRIV_H_


#define GET_BIT(var,bitNo)          (((var)  >>  (bitNo))  &  1U)

void vidLCDWriteReg(LCD_8 l, uint16_t reg);

#endif /* LCD_DRIVER_LCD_PRIV_H_ */
