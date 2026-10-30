/**
 * @file lcd.h
 * @brief High-Performance 16x2 Character LCD Driver for STM32F103
 * @author Assiut Robotics Team
 * @date 2026-10-29
 *
 * @details Supports both 4-bit mode (recommended to avoid I2C pin conflicts)
 *          and 8-bit mode. Fully HD44780 compliant.
 *
 * Pin Mapping in 4-Bit Mode (Default):
 *   - Control Pins on GPIOA:
 *       * PA0 -> EN  (Enable strobe)
 *       * PA1 -> RS  (Register Select: 0=Command, 1=Data)
 *       * PA2 -> RW  (Read/Write: 0=Write)
 *   - 4-Bit Data Bus on GPIOB:
 *       * PB0 -> D4
 *       * PB1 -> D5
 *       * PB2 -> D6
 *       * PB3 -> D7
 *   *Note: Leaves PB6 (I2C1_SCL) and PB7 (I2C1_SDA) completely free and isolated!*
 */

#ifndef __LCD_H__
#define __LCD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "mpu6050.h"
#include "bmp280.h"

/**
 * @brief Select 4-bit mode (1) or 8-bit mode (0).
 * 4-Bit mode is recommended because PB6/PB7 are dedicated to I2C1 (MPU6050 & BMP280).
 */
#define LCD_MODE_4BIT               1

/* ========================================================================== */
/* Pin Configuration Definitions                                              */
/* ========================================================================== */

/* Control Pins (Identical to Task 3) */
#define LCD_CTRL_PORT               GPIOA
#define LCD_PIN_EN                  GPIO_PIN_0
#define LCD_PIN_RS                  GPIO_PIN_1
#define LCD_PIN_RW                  GPIO_PIN_2

#if LCD_MODE_4BIT
/* 4-Bit Data Bus on PB0..PB3 (Leaves PB6/PB7 free for I2C) */
#define LCD_DATA_PORT               GPIOB
#define LCD_PIN_D4                  GPIO_PIN_0
#define LCD_PIN_D5                  GPIO_PIN_1
#define LCD_PIN_D6                  GPIO_PIN_2
#define LCD_PIN_D7                  GPIO_PIN_3
#else
/* 8-Bit Data Bus on PA0..PA7 */
#define LCD_DATA_PORT               GPIOA
#endif

/* ========================================================================== */
/* Public Function Prototypes                                                 */
/* ========================================================================== */

/**
 * @brief Initializes the LCD in 4-bit (or 8-bit) mode with cursor off and display cleared.
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

/**
 * @brief High-level helper that displays real-time telemetry from MPU-6050 and BMP280.
 * @details Displays Roll & Pitch on Row 0, and smoothly alternates Row 1 between
 *          (Yaw + Temperature) and (Pressure + Altitude) to prevent LCD flicker.
 * @param[in] mpu Pointer to MPU-6050 handle structure.
 * @param[in] bmp Pointer to BMP handle structure.
 * @retval None
 */
void LCD_DisplaySensors(const MPU6050_t *mpu, const BMP_t *bmp);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H__ */
