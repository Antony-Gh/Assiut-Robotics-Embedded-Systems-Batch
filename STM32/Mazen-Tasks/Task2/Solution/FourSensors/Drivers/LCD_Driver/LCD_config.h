/*
 * LCD_config.h
 *
 *  Created on: Jul 22, 2026
 *      Author: m0101
 */

#ifndef LCD_DRIVER_LCD_CONFIG_H_
#define LCD_DRIVER_LCD_CONFIG_H_
#include "../Inc/stm32f1xx_hal.h"
typedef struct {
    GPIO_TypeDef* port;  // Port: GPIOA, GPIOB, etc.
    uint16_t      pin;   // Pin: GPIO_PIN_0, GPIO_PIN_1, etc.
} LCD_Pin;
typedef struct {
    LCD_Pin EN;
    LCD_Pin RS;
    LCD_Pin RW;
    LCD_Pin DB7;
    LCD_Pin DB6;
    LCD_Pin DB5;
    LCD_Pin DB4;
    LCD_Pin DB3;
    LCD_Pin DB2;
    LCD_Pin DB1;
    LCD_Pin DB0;
} LCD_8;

typedef enum {
    R1,
    R2
}RN;
typedef enum {
    C1, C2, C3, C4, 
    C5, C6, C7, C8,
    C9, C10, C11, C12,
    C13, C14, C15, C16
}CN;


#define NROWS   2
#define FONT    0
#define DCURSOR 1
#define BLINK   1
#define DI      1
#define SHIFT   0


#define SCREENS 5

#endif /* LCD_DRIVER_LCD_CONFIG_H_ */
