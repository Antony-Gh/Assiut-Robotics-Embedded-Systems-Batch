/**
 * @file lcd.h
 * @brief High-Performance 16x2 Character LCD Driver for STM32F103 (8-Bit Mode)
 * @author Assiut Robotics Team
 * @date 2026-10-28
 */

#ifndef __LCD_H__
#define __LCD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initializes the LCD in 8-bit mode with cursor off and display cleared.
 * @retval None
 */
void LCD_Init(void);

/**
 * @brief Clears the LCD screen and returns cursor to (0, 0).
 * @retval None
 */
void LCD_Clear(void);

/**
 * @brief Sets the LCD cursor position.
 * @param[in] row Line number (0 for first line, 1 for second line).
 * @param[in] col Column number (0 to 15).
 * @retval None
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief Sends a single character to the LCD.
 * @param[in] ch ASCII character to display.
 * @retval None
 */
void LCD_SendChar(char ch);

/**
 * @brief Sends a null-terminated string to the LCD.
 * @param[in] str Pointer to string.
 * @retval None
 */
void LCD_Print(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H__ */
