/**
  ******************************************************************************
  * @file           : lcd.h
  * @brief          : Header file for 16x2 Character LCD Driver (HD44780 / LM016L)
  *                   Assiut Robotics Embedded Systems Team - Task 4
  *                   STM32F401RCT6 (Blackpill)
  ******************************************************************************
  * @details
  * This driver implements an 8-bit parallel interface to an alphanumeric LCD
  * display compatible with the Hitachi HD44780 controller (e.g. LM016L 16x2).
  *
  * Pin Connections (STM32F401RCT6):
  *   - Control Pins:
  *       * PA4 -> LCD_EN (Enable Signal, Active High pulse)
  *       * PA5 -> LCD_RS (Register Select: 0 = Command, 1 = Data)
  *       * PA6 -> LCD_RW (Read/Write: 0 = Write, 1 = Read)
  *   - Data Bus (8-bit):
  *       * PC0 -> LCD_D0
  *       * PC1 -> LCD_D1
  *       * PC2 -> LCD_D2
  *       * PC3 -> LCD_D3
  *       * PC4 -> LCD_D4
  *       * PC5 -> LCD_D5
  *       * PC6 -> LCD_D6
  *       * PC7 -> LCD_D7
  ******************************************************************************
  */

#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
  * @defgroup LCD_Hardware_Configuration LCD Hardware Pin Assignments
  * @{
  */
#define LCD_CTRL_PORT       GPIOA
#define LCD_PIN_EN          GPIO_PIN_4
#define LCD_PIN_RS          GPIO_PIN_5
#define LCD_PIN_RW          GPIO_PIN_6

#define LCD_DATA_PORT       GPIOC
#define LCD_DATA_PINS       (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                             GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
/**
  * @}
  */

/**
  * @defgroup LCD_Commands HD44780 Instruction Set Definitions
  * @{
  */
#define LCD_CMD_CLEAR_DISPLAY          0x01U  /*!< Clear display screen and reset DDRAM address to 0 */
#define LCD_CMD_RETURN_HOME            0x02U  /*!< Return cursor to home position (Row 0, Col 0)     */
#define LCD_CMD_ENTRY_MODE_SET         0x06U  /*!< Increment address, shift cursor right, no display shift */
#define LCD_CMD_DISPLAY_OFF            0x08U  /*!< Turn display off                                 */
#define LCD_CMD_DISPLAY_ON_NO_CURSOR   0x0CU  /*!< Display ON, Cursor OFF, Blink OFF                 */
#define LCD_CMD_DISPLAY_ON_CURSOR_ON   0x0EU  /*!< Display ON, Cursor ON, Blink OFF                  */
#define LCD_CMD_DISPLAY_ON_BLINK_ON    0x0FU  /*!< Display ON, Cursor ON, Blink ON                   */
#define LCD_CMD_FUNCTION_SET_8BIT_2L   0x38U  /*!< 8-bit bus, 2 display lines, 5x8 dot font matrix   */
#define LCD_CMD_SET_DDRAM_ADDR         0x80U  /*!< Base command to set Display Data RAM (DDRAM) address */

#define LCD_LINE1_BASE_ADDR            0x00U  /*!< DDRAM start address for Row 1 (Index 0)           */
#define LCD_LINE2_BASE_ADDR            0x40U  /*!< DDRAM start address for Row 2 (Index 1)           */
/**
  * @}
  */

/* Public Function Prototypes -----------------------------------------------*/

/**
  * @brief  Initializes the LCD hardware pins and executes the HD44780 power-on sequence.
  * @note   Configures control pins (PA0..PA2) and data pins (PB0..PB7) as Output Push-Pull,
  *         and sends standard 8-bit initialization commands (Function Set, Display Control,
  *         Clear Screen, Entry Mode).
  * @retval None
  */
void LCD_Init(void);

/**
  * @brief  Sends a command byte to the LCD instruction register (IR).
  * @param  cmd: Command code to execute (e.g., LCD_CMD_CLEAR_DISPLAY).
  * @retval None
  */
void LCD_SendCommand(uint8_t cmd);

/**
  * @brief  Sends a data byte to the LCD data register (DR) to be displayed.
  * @param  data: ASCII character code or custom character index.
  * @retval None
  */
void LCD_SendData(uint8_t data);

/**
  * @brief  Prints a single ASCII character at current cursor position.
  * @param  ch: ASCII character to print.
  * @retval None
  */
void LCD_SendChar(char ch);

/**
  * @brief  Prints a null-terminated string onto the LCD.
  * @param  str: Pointer to null-terminated C string.
  * @retval None
  */
void LCD_SendString(const char *str);
void LCD_Print(const char *str);

/**
  * @brief  Positions the cursor at specified row and column.
  * @param  row: Row index (0 for Row 1, 1 for Row 2).
  * @param  col: Column index (0 to 15 for 16-column display).
  * @retval None
  */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
  * @brief  Clears the entire LCD screen and resets cursor position to (0, 0).
  * @retval None
  */
void LCD_Clear(void);

/**
  * @brief  Prints an integer number onto the LCD.
  * @param  num: 32-bit signed integer value to display.
  * @retval None
  */
void LCD_PrintNumber(int32_t num);

/**
  * @brief  Prints a floating point number with specified decimal precision.
  * @param  val: Float value to display.
  * @param  decimal_places: Number of digits to display after the decimal point.
  * @retval None
  */
void LCD_PrintFloat(float val, uint8_t decimal_places);

/**
  * @brief  Displays real-time telemetry from MPU-6050 and BMP280 on 16x2 LCD.
  * @param  roll: Roll angle in degrees.
  * @param  pitch: Pitch angle in degrees.
  * @param  yaw: Yaw angle in degrees.
  * @param  temp: Temperature in Celsius.
  * @param  press: Pressure in hPa.
  * @param  alt: Altitude in meters.
  * @param  page: Display page (0: Roll/Pitch + Yaw/Temp; 1: Roll/Pitch + Press/Alt).
  */
void LCD_DisplaySensors(float roll, float pitch, float yaw, float temp, float press, float alt, uint8_t page);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
