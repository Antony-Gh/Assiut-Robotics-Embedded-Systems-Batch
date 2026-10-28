/**
 * @file lcd.c
 * @brief Implementation of 16x2 Character LCD Driver for STM32F103 (8-Bit Mode)
 * @author Assiut Robotics Team
 * @date 2026-10-28
 */

#include "lcd.h"

/**
 * @brief Microsecond delay helper
 * @param[in] us Duration in microseconds
 * @retval None
 */
static void LCD_DelayUs(uint32_t us)
{
    volatile uint32_t count = us * 2;
    while (count--)
    {
        __NOP();
    }
}

/**
 * @brief Sends an 8-bit command byte to the LCD controller
 * @param[in] cmd Command byte
 * @retval None
 */
static void LCD_SendCommand(uint8_t cmd)
{
    /* RS = 0 (Command), RW = 0 (Write) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

    /* Output data byte directly on GPIOB pins PB0..PB7 */
    GPIOB->ODR = (GPIOB->ODR & 0xFF00U) | (uint16_t)cmd;

    /* Strobe Enable pin (pulse width >= 450 ns) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    LCD_DelayUs(10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    LCD_DelayUs(50);
}

/**
 * @brief Sends an 8-bit data character to the LCD controller
 * @param[in] data Character ASCII byte
 * @retval None
 */
static void LCD_SendData(uint8_t data)
{
    /* RS = 1 (Data), RW = 0 (Write) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

    /* Output data byte directly on GPIOB pins PB0..PB7 */
    GPIOB->ODR = (GPIOB->ODR & 0xFF00U) | (uint16_t)data;

    /* Strobe Enable pin */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    LCD_DelayUs(10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    LCD_DelayUs(50);
}

/**
 * @brief Initializes the LCD in 8-bit mode with cursor off and display cleared.
 * @retval None
 */
void LCD_Init(void)
{
    /* Enable GPIOA and GPIOB clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure Control Pins: PA0 (EN), PA1 (RS), PA2 (RW) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

    /* Configure Data Pins: PB0..PB7 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIOB->ODR &= 0xFF00U;

    /* Power-on delay (> 15 ms required by HD44780) */
    HAL_Delay(30);

    /* 8-bit mode initialization sequence */
    LCD_SendCommand(0x38); /* Function Set: 8-bit, 2 lines, 5x8 font */
    HAL_Delay(5);
    LCD_SendCommand(0x38);
    HAL_Delay(1);
    LCD_SendCommand(0x38);

    /* Display control: Display ON, Cursor OFF, Blink OFF */
    LCD_SendCommand(0x0C);
    HAL_Delay(1);

    /* Clear display */
    LCD_Clear();

    /* Entry mode: Increment cursor automatically, no display shift */
    LCD_SendCommand(0x06);
    HAL_Delay(1);
}

/**
 * @brief Clears the LCD screen and returns cursor to (0, 0).
 * @retval None
 */
void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    HAL_Delay(2); /* Clear command requires ~1.52 ms execution time */
}

/**
 * @brief Sets the LCD cursor position.
 * @param[in] row Line number (0 for first line, 1 for second line).
 * @param[in] col Column number (0 to 15).
 * @retval None
 */
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    /* Line 1 base = 0x80, Line 2 base = 0xC0 (0x80 | 0x40) */
    uint8_t addr = (row == 0) ? (0x80U | (col & 0x0FU)) : (0xC0U | (col & 0x0FU));
    LCD_SendCommand(addr);
}

/**
 * @brief Sends a single character to the LCD.
 * @param[in] ch ASCII character to display.
 * @retval None
 */
void LCD_SendChar(char ch)
{
    LCD_SendData((uint8_t)ch);
}

/**
 * @brief Sends a null-terminated string to the LCD.
 * @param[in] str Pointer to string.
 * @retval None
 */
void LCD_Print(const char *str)
{
    if (str == NULL) return;
    while (*str)
    {
        LCD_SendData((uint8_t)(*str++));
    }
}
