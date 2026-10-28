/*
 * LCD_prog.c
 *
 *  Adapted for STM32 HAL
 */

#include "LCD_int.h"
#include "LCD_config.h"
#include "LCD_priv.h"
#define _delay_ms(x) HAL_Delay(x)

static void LCD_DelayUs(uint32_t us)
{
    volatile uint32_t count = us * 8;
    while (count--)
    {
        __NOP();
    }
}

static void LCD_SetDataPinsMode(LCD_8 l, uint32_t mode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    LCD_Pin dataPins[8] = {l.DB0, l.DB1, l.DB2, l.DB3, l.DB4, l.DB5, l.DB6, l.DB7};

    //enableing the Clock
    for(int i =0; i< 8;i++)
    {
        if(dataPins[i].port == GPIOA)
        	__HAL_RCC_GPIOA_CLK_ENABLE();
          else if(dataPins[i].port == GPIOB)
        	  __HAL_RCC_GPIOB_CLK_ENABLE();
          else if(dataPins[i].port == GPIOC)
        	  __HAL_RCC_GPIOC_CLK_ENABLE();
          else if(dataPins[i].port == GPIOD)
        	  __HAL_RCC_GPIOD_CLK_ENABLE();
    }
   // intializing pins
    for (int i = 0; i < 8; i++)
    {
        GPIO_InitStruct.Pin = dataPins[i].pin;
        HAL_GPIO_Init(dataPins[i].port, &GPIO_InitStruct);
    }

}

void vidLCDInit(LCD_8 l)
{
    _delay_ms(30);

    LCD_SetDataPinsMode(l, GPIO_MODE_OUTPUT_PP);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    LCD_Pin ctrlPins[3] = {l.RS, l.RW, l.EN};
    for (int i = 0; i < 3; i++)
    {
    	// enable clock
		if(ctrlPins[i].port == GPIOA)
    			__HAL_RCC_GPIOA_CLK_ENABLE();
		  else if(ctrlPins[i].port == GPIOB)
			  __HAL_RCC_GPIOB_CLK_ENABLE();
		  else if(ctrlPins[i].port == GPIOC)
			  __HAL_RCC_GPIOC_CLK_ENABLE();
		  else if(ctrlPins[i].port == GPIOD)
			  __HAL_RCC_GPIOD_CLK_ENABLE();
        GPIO_InitStruct.Pin = ctrlPins[i].pin;
        HAL_GPIO_Init(ctrlPins[i].port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(ctrlPins[i].port, ctrlPins[i].pin, GPIO_PIN_RESET);
    }

    uint16_t functionSet = 0b0000110000 | ((NROWS - 1) << 3) | (FONT << 2);
    vidLCDWriteReg(l, functionSet);
    _delay_ms(1);

    uint16_t DisplayNF = 0x008 | (1 << 2) | (DCURSOR << 1) | BLINK;
    vidLCDWriteReg(l, DisplayNF);
    _delay_ms(1);

    uint16_t DisplayClr = 1;
    vidLCDWriteReg(l, DisplayClr);
    _delay_ms(10);

    uint16_t EnterModeSet = 0x004 | (DI << 1) | SHIFT;
    vidLCDWriteReg(l, EnterModeSet);
    _delay_ms(1);
}

void vidLCDWriteReg(LCD_8 l, uint16_t reg)
{
    HAL_GPIO_WritePin(l.RS.port, l.RS.pin, (GPIO_PinState)GET_BIT(reg, 9));
    HAL_GPIO_WritePin(l.RW.port, l.RW.pin, (GPIO_PinState)GET_BIT(reg, 8));

    HAL_GPIO_WritePin(l.DB7.port, l.DB7.pin, (GPIO_PinState)GET_BIT(reg, 7));
    HAL_GPIO_WritePin(l.DB6.port, l.DB6.pin, (GPIO_PinState)GET_BIT(reg, 6));
    HAL_GPIO_WritePin(l.DB5.port, l.DB5.pin, (GPIO_PinState)GET_BIT(reg, 5));
    HAL_GPIO_WritePin(l.DB4.port, l.DB4.pin, (GPIO_PinState)GET_BIT(reg, 4));
    HAL_GPIO_WritePin(l.DB3.port, l.DB3.pin, (GPIO_PinState)GET_BIT(reg, 3));
    HAL_GPIO_WritePin(l.DB2.port, l.DB2.pin, (GPIO_PinState)GET_BIT(reg, 2));
    HAL_GPIO_WritePin(l.DB1.port, l.DB1.pin, (GPIO_PinState)GET_BIT(reg, 1));
    HAL_GPIO_WritePin(l.DB0.port, l.DB0.pin, (GPIO_PinState)GET_BIT(reg, 0));

    HAL_GPIO_WritePin(l.EN.port, l.EN.pin, GPIO_PIN_SET);
    LCD_DelayUs(10);
    HAL_GPIO_WritePin(l.EN.port, l.EN.pin, GPIO_PIN_RESET);
    LCD_DelayUs(50);
}

void vidLCDScreenClr(LCD_8 l)
{
    vidLCDWriteReg(l, 1);
    _delay_ms(2);
}

uint8_t u8LCDReadAdd(LCD_8 l)
{
    LCD_SetDataPinsMode(l, GPIO_MODE_INPUT);

    HAL_GPIO_WritePin(l.RS.port, l.RS.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(l.RW.port, l.RW.pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(l.EN.port, l.EN.pin, GPIO_PIN_SET);
    _delay_ms(1);

    uint8_t result = 0;
    result |= (HAL_GPIO_ReadPin(l.DB7.port, l.DB7.pin) << 7)
            | (HAL_GPIO_ReadPin(l.DB6.port, l.DB6.pin) << 6)
            | (HAL_GPIO_ReadPin(l.DB5.port, l.DB5.pin) << 5)
            | (HAL_GPIO_ReadPin(l.DB4.port, l.DB4.pin) << 4)
            | (HAL_GPIO_ReadPin(l.DB3.port, l.DB3.pin) << 3)
            | (HAL_GPIO_ReadPin(l.DB2.port, l.DB2.pin) << 2)
            | (HAL_GPIO_ReadPin(l.DB1.port, l.DB1.pin) << 1)
            | (HAL_GPIO_ReadPin(l.DB0.port, l.DB0.pin) << 0);

    HAL_GPIO_WritePin(l.EN.port, l.EN.pin, GPIO_PIN_RESET);

    LCD_SetDataPinsMode(l, GPIO_MODE_OUTPUT_PP);

    return result;
}

void vidLCDSetCursor(LCD_8 l, RN r, CN c)
{
	uint16_t regValue = (1 << 7);
    if (r)
    {
        regValue |= (1 << 6); /* 0x40: Set DDRAM address to row 2 */
    }
    regValue |= (uint16_t)c;
    vidLCDWriteReg(l, regValue);
    LCD_DelayUs(50);
}

void vidLCDWriteChar(LCD_8 l, uint8_t c)
{
	uint16_t regValue = 1 << 9;
    if (c == '\n')
    {
    	uint8_t currentAdd = u8LCDReadAdd(l);
        if (currentAdd >= 0x40)
            vidLCDSetCursor(l, R1, C1);
        else            
            vidLCDSetCursor(l, R2, C1);
        LCD_DelayUs(50);
        return;
    }
    else if (c == '\t')
    {
    	uint8_t currentAdd = u8LCDReadAdd(l);
        RN row = (currentAdd >= 0x40);
        CN col = (uint8_t)(currentAdd) + 4;
        vidLCDSetCursor(l, row, col);
        return;
    }
    regValue |= c;
    vidLCDWriteReg(l, regValue);
    LCD_DelayUs(50);
}

void vidLCDWriteStr(LCD_8 l, uint8_t* cptr)
{
    int i = 0;
    int c = 0;
    while (cptr[i] != '\0')
    {
        if (cptr[i] == '\n')
            c = 0;
        vidLCDWriteChar(l, cptr[i++]);
        c++;
    }
}
