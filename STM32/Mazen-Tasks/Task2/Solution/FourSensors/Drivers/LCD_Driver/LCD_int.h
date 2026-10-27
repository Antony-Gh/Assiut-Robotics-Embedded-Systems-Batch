/*
 * LCD_int.h
 *
 *  Created on: Jul 22, 2026
 *      Author: m0101
 */

#ifndef LCD_DRIVER_LCD_INT_H_
#define LCD_DRIVER_LCD_INT_H_

#include "LCD_config.h"

void vidLCDInit(LCD_8 l);

void vidLCDWriteReg(LCD_8 l, uint16_t reg);

void vidLCDSetCursor(LCD_8 l, RN r, CN c);

void vidLCDWriteChar(LCD_8 l, uint8_t c);

void vidLCDScreenClr(LCD_8 l);

void vidLCDWriteStr(LCD_8 l, uint8_t* cptr);
#endif /* LCD_DRIVER_LCD_INT_H_ */
