/**
  ******************************************************************************
  * @file           : lcd.c
  * @brief          : Implementation of 16x2 Character LCD Driver (HD44780 / LM016L)
  *                   Assiut Robotics Embedded Systems Team - Task 2
  * @author         : Embedded Systems Team
  * @date           : 2026-10-28
  ******************************************************************************
  * @details
  * Implements 8-bit parallel control routines for standard alphanumeric LCDs.
  * Uses STM32 HAL library for GPIO pin control and hardware delay timing.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lcd.h"
#include <stdio.h>

/* Private Function Prototypes -----------------------------------------------*/
static void LCD_PulseEnable(void);
static void LCD_WriteBus(uint8_t value);
static void LCD_DelayUs(uint32_t us);

/* Public Functions ----------------------------------------------------------*/

/**
  * @brief  Initializes the LCD hardware pins and executes the HD44780 power-on sequence.
  * @retval None
  */
void LCD_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. Enable GPIO Clocks for Control Port (GPIOA) and Data Port (GPIOB) */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* 2. Configure Control Pins (PA0: EN, PA1: RS, PA2: RW) as Output Push-Pull */
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN | LCD_PIN_RS | LCD_PIN_RW, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = LCD_PIN_EN | LCD_PIN_RS | LCD_PIN_RW;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_CTRL_PORT, &GPIO_InitStruct);

  /* 3. Configure Data Bus Pins (PB0..PB7: D0..D7) as Output Push-Pull */
  HAL_GPIO_WritePin(LCD_DATA_PORT, LCD_DATA_PINS, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = LCD_DATA_PINS;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_DATA_PORT, &GPIO_InitStruct);

  /* 4. Power-on settling delay (HD44780 requires > 15ms after Vcc reaches 4.5V) */
  HAL_Delay(50);

  /* 5. Function Set: 8-bit bus mode, 2 display lines, 5x8 dot font matrix */
  LCD_SendCommand(LCD_CMD_FUNCTION_SET_8BIT_2L);
  HAL_Delay(5);

  /* Re-issue Function Set to guarantee initialization */
  LCD_SendCommand(LCD_CMD_FUNCTION_SET_8BIT_2L);
  HAL_Delay(1);

  /* 6. Display Control: Display ON, Cursor OFF, Blink OFF */
  LCD_SendCommand(LCD_CMD_DISPLAY_ON_NO_CURSOR);
  HAL_Delay(1);

  /* 7. Clear Screen */
  LCD_Clear();
  HAL_Delay(5);

  /* 8. Entry Mode: Auto-increment address pointer after write, no display shift */
  LCD_SendCommand(LCD_CMD_ENTRY_MODE_SET);
  HAL_Delay(2);
}

/**
  * @brief  Sends a command byte to the LCD instruction register (IR).
  * @param  cmd: Command code to execute.
  * @retval None
  */
void LCD_SendCommand(uint8_t cmd)
{
  /* RS = 0: Instruction Register */
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_RESET);

  /* RW = 0: Write Operation */
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

  /* Write 8-bit command to data bus */
  LCD_WriteBus(cmd);

  /* Latch data on falling edge of Enable */
  LCD_PulseEnable();

  /* Execution time delay (Clear and Return Home require longer delay ~2ms) */
  if (cmd == LCD_CMD_CLEAR_DISPLAY || cmd == LCD_CMD_RETURN_HOME)
  {
    HAL_Delay(2);
  }
  else
  {
    LCD_DelayUs(40);
  }
}

/**
  * @brief  Sends a data byte to the LCD data register (DR) to be displayed.
  * @param  data: ASCII character code.
  * @retval None
  */
void LCD_SendData(uint8_t data)
{
  /* RS = 1: Data Register */
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_SET);

  /* RW = 0: Write Operation */
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

  /* Write 8-bit data to data bus */
  LCD_WriteBus(data);

  /* Latch data on falling edge of Enable */
  LCD_PulseEnable();

  LCD_DelayUs(40);
}

/**
  * @brief  Prints a single ASCII character at current cursor position.
  * @param  ch: ASCII character to print.
  * @retval None
  */
void LCD_SendChar(char ch)
{
  LCD_SendData((uint8_t)ch);
}

/**
  * @brief  Prints a null-terminated string onto the LCD.
  * @param  str: Pointer to null-terminated C string.
  * @retval None
  */
void LCD_SendString(const char *str)
{
  if (str == NULL) return;

  while (*str != '\0')
  {
    LCD_SendChar(*str);
    str++;
  }
}

/**
  * @brief  Positions the cursor at specified row and column.
  * @param  row: Row index (0 for Row 1, 1 for Row 2).
  * @param  col: Column index (0 to 15 for 16-column display).
  * @retval None
  */
void LCD_SetCursor(uint8_t row, uint8_t col)
{
  uint8_t address = 0;

  if (col > 15) col = 15;

  if (row == 0)
  {
    address = LCD_CMD_SET_DDRAM_ADDR | (LCD_LINE1_BASE_ADDR + col);
  }
  else
  {
    address = LCD_CMD_SET_DDRAM_ADDR | (LCD_LINE2_BASE_ADDR + col);
  }

  LCD_SendCommand(address);
}

/**
  * @brief  Clears the entire LCD screen and resets cursor position to (0, 0).
  * @retval None
  */
void LCD_Clear(void)
{
  LCD_SendCommand(LCD_CMD_CLEAR_DISPLAY);
  HAL_Delay(3);
}

/**
  * @brief  Prints an integer number onto the LCD.
  * @param  num: 32-bit signed integer value to display.
  * @retval None
  */
void LCD_PrintNumber(int32_t num)
{
  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%ld", (long)num);
  LCD_SendString(buffer);
}

/**
  * @brief  Prints a floating point number with specified decimal precision.
  * @param  val: Float value to display.
  * @param  decimal_places: Number of digits to display after the decimal point.
  * @retval None
  */
void LCD_PrintFloat(float val, uint8_t decimal_places)
{
  char buffer[16];
  if (decimal_places > 4) decimal_places = 4;

  if (decimal_places == 1)
  {
    snprintf(buffer, sizeof(buffer), "%.1f", (double)val);
  }
  else if (decimal_places == 2)
  {
    snprintf(buffer, sizeof(buffer), "%.2f", (double)val);
  }
  else if (decimal_places == 3)
  {
    snprintf(buffer, sizeof(buffer), "%.3f", (double)val);
  }
  else
  {
    snprintf(buffer, sizeof(buffer), "%d", (int)val);
  }

  LCD_SendString(buffer);
}

/* Private Functions ---------------------------------------------------------*/

/**
  * @brief  Generates microsecond delay for LCD command/data cycle execution.
  * @param  us: Microseconds to delay.
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
  * @brief  Generates an Enable (EN) pulse to latch instruction or data into the LCD controller.
  * @retval None
  */
static void LCD_PulseEnable(void)
{
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN, GPIO_PIN_SET);
  LCD_DelayUs(5);
  HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN, GPIO_PIN_RESET);
  LCD_DelayUs(5);
}

/**
  * @brief  Writes an 8-bit byte to the LCD parallel data pins (PB0..PB7).
  * @param  value: 8-bit data/command value.
  * @retval None
  */
static void LCD_WriteBus(uint8_t value)
{
  /* Since PB0..PB7 map 1-to-1 to D0..D7, write lower 8 bits of GPIOB directly */
  GPIOB->ODR = (GPIOB->ODR & 0xFF00) | (uint16_t)value;
}
