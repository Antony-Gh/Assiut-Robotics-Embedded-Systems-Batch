/**
 * @file lcd.c
 * @brief Implementation of 16x2 Character LCD Driver for STM32F103 (Task 4)
 * @author Assiut Robotics Team
 * @date 2026-10-29
 */

#include "lcd.h"
#include <stdio.h>
#include <string.h>

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
 * @brief Pulses the LCD Enable pin to latch data/command
 * @retval None
 */
static void LCD_PulseEnable(void)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN, GPIO_PIN_SET);
    LCD_DelayUs(10);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN, GPIO_PIN_RESET);
    LCD_DelayUs(50);
}

#if LCD_MODE_4BIT

/**
 * @brief Sends a 4-bit nibble to the LCD data lines (PB0..PB3)
 * @details Uses atomic BSRR bit-set/reset to avoid disturbing other GPIOB pins
 *          such as PB6 (I2C1_SCL) and PB7 (I2C1_SDA).
 * @param[in] nibble Lower 4 bits contain the data nibble
 * @retval None
 */
static void LCD_SendNibble(uint8_t nibble)
{
    /* Mask to lower 4 bits */
    uint32_t n = (uint32_t)(nibble & 0x0FU);

    /* Atomic bit manipulation:
     * Lower 16 bits of BSRR set pins HIGH (where bit is 1)
     * Upper 16 bits of BSRR reset pins LOW (where bit was 0)
     * Mask covers only PB0..PB3 (0x000F)
     */
    LCD_DATA_PORT->BSRR = (((~n) & 0x000FU) << 16) | n;

    LCD_PulseEnable();
}

/**
 * @brief Sends an 8-bit command byte as two 4-bit nibbles
 * @param[in] cmd Command byte
 * @retval None
 */
static void LCD_SendCommand(uint8_t cmd)
{
    /* RS = 0 (Command), RW = 0 (Write) */
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

    /* Send upper nibble, then lower nibble */
    LCD_SendNibble((cmd >> 4) & 0x0F);
    LCD_SendNibble(cmd & 0x0F);

    if (cmd == 0x01 || cmd == 0x02)
    {
        HAL_Delay(2); /* Clear Display and Return Home require ~1.52 ms execution */
    }
}

/**
 * @brief Sends an 8-bit data byte as two 4-bit nibbles
 * @param[in] data Character ASCII byte
 * @retval None
 */
static void LCD_SendData(uint8_t data)
{
    /* RS = 1 (Data), RW = 0 (Write) */
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

    /* Send upper nibble, then lower nibble */
    LCD_SendNibble((data >> 4) & 0x0F);
    LCD_SendNibble(data & 0x0F);
}

#else /* 8-Bit Mode */

static void LCD_SendCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00U) | (uint16_t)cmd;
    LCD_PulseEnable();

    if (cmd == 0x01 || cmd == 0x02)
    {
        HAL_Delay(2);
    }
}

static void LCD_SendData(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

    LCD_DATA_PORT->ODR = (LCD_DATA_PORT->ODR & 0xFF00U) | (uint16_t)data;
    LCD_PulseEnable();
}

#endif

/**
 * @brief Initializes the LCD in 4-bit (or 8-bit) mode with cursor off and display cleared.
 * @retval None
 */
void LCD_Init(void)
{
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure Control Pins: PA0 (EN), PA1 (RS), PA2 (RW) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LCD_PIN_EN | LCD_PIN_RS | LCD_PIN_RW;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LCD_CTRL_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_EN | LCD_PIN_RS | LCD_PIN_RW, GPIO_PIN_RESET);

#if LCD_MODE_4BIT
    /* Configure 4-Bit Data Bus Pins: PB0..PB3 (Preserving PB6 and PB7 for I2C!) */
    GPIO_InitStruct.Pin = LCD_PIN_D4 | LCD_PIN_D5 | LCD_PIN_D6 | LCD_PIN_D7;
    HAL_GPIO_Init(LCD_DATA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LCD_DATA_PORT, LCD_PIN_D4 | LCD_PIN_D5 | LCD_PIN_D6 | LCD_PIN_D7, GPIO_PIN_RESET);

    /* Power-on delay (> 40 ms required by HD44780 at 4.5V) */
    HAL_Delay(50);

    /* 4-Bit Initialization sequence specified by HD44780 datasheet:
     * 1. Send 0x03 nibble -> wait > 4.1 ms
     * 2. Send 0x03 nibble -> wait > 100 us
     * 3. Send 0x03 nibble -> wait > 100 us
     * 4. Send 0x02 nibble -> switch controller to 4-bit mode!
     */
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CTRL_PORT, LCD_PIN_RW, GPIO_PIN_RESET);

    LCD_SendNibble(0x03);
    HAL_Delay(5);

    LCD_SendNibble(0x03);
    LCD_DelayUs(150);

    LCD_SendNibble(0x03);
    LCD_DelayUs(150);

    LCD_SendNibble(0x02); /* Switch to 4-Bit Mode */
    LCD_DelayUs(150);

    /* Now in 4-bit mode: 2 lines, 5x8 font */
    LCD_SendCommand(0x28);
    HAL_Delay(1);

#else
    /* 8-Bit Data Bus Pins */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(LCD_DATA_PORT, &GPIO_InitStruct);

    HAL_Delay(50);

    LCD_SendCommand(0x38);
    HAL_Delay(5);
    LCD_SendCommand(0x38);
    HAL_Delay(1);
    LCD_SendCommand(0x38);
    HAL_Delay(1);
#endif

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
    HAL_Delay(2);
}

/**
 * @brief Sets the LCD cursor position.
 * @param[in] row Line number (0 for first line, 1 for second line).
 * @param[in] col Column number (0 to 15).
 * @retval None
 */
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    /* Line 1 base = 0x80, Line 2 base = 0xC0 */
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

/**
 * @brief Helper to format a signed float into a fixed "+00.0" or "-00.0" 5-character string
 */
static void FormatSignedAngle(char *out, size_t len, float val)
{
    char sign = (val < 0.0f) ? '-' : '+';
    if (val < 0.0f) val = -val;

    int ipart = (int)val;
    int fpart = (int)((val - (float)ipart) * 10.0f + 0.5f);
    if (fpart >= 10)
    {
        ipart++;
        fpart = 0;
    }
    if (ipart > 99) ipart = 99; /* clamp to 2 digits for neat alignment */

    snprintf(out, len, "%c%02d.%d", sign, ipart, fpart);
}

/**
 * @brief Helper to format an unsigned float with 1 decimal place (e.g. "25.3")
 */
static void FormatTemp(char *out, size_t len, float val)
{
    int ipart = (int)val;
    int fpart = (int)((val - (float)ipart) * 10.0f + 0.5f);
    if (fpart >= 10)
    {
        ipart++;
        fpart = 0;
    }
    snprintf(out, len, "%2d.%d", ipart, fpart);
}

/**
 * @brief High-level helper that displays real-time telemetry from MPU-6050 and BMP280.
 * @details Row 0: Roll and Pitch angles (always updated in real-time).
 *          Row 1: Alternates smoothly every ~2 seconds between:
 *                 - Phase 0: Yaw angle and Ambient Temperature
 *                 - Phase 1: Barometric Pressure and Altitude
 * @param[in] mpu Pointer to MPU-6050 handle structure.
 * @param[in] bmp Pointer to BMP handle structure.
 * @retval None
 */
void LCD_DisplaySensors(const MPU6050_t *mpu, const BMP_t *bmp)
{
    static uint8_t view_counter = 0;
    char line1[32];
    char line2[32];
    char roll_buf[8];
    char pitch_buf[8];
    char yaw_buf[8];
    char temp_buf[8];

    /* Format Roll and Pitch for Row 0 */
    FormatSignedAngle(roll_buf, sizeof(roll_buf), mpu->roll);
    FormatSignedAngle(pitch_buf, sizeof(pitch_buf), mpu->pitch);

    /* Row 0 format: "R:+12.4  P:-05.1" -> exactly 16 characters */
    snprintf(line1, sizeof(line1), "R:%s  P:%s", roll_buf, pitch_buf);
    line1[16] = '\0';

    /* Row 1 alternate view every 8 updates (~2 seconds at 4 Hz) */
    if (view_counter < 8)
    {
        /* View A: Yaw and Temperature */
        FormatSignedAngle(yaw_buf, sizeof(yaw_buf), mpu->yaw);
        FormatTemp(temp_buf, sizeof(temp_buf), bmp->temperature);

        /* Format: "Y:+00.0  T:25.0C" -> exactly 16 characters */
        snprintf(line2, sizeof(line2), "Y:%s  T:%sC", yaw_buf, temp_buf);
    }
    else
    {
        /* View B: Pressure and Altitude */
        int press_int = (int)(bmp->pressure_hpa + 0.5f);
        int alt_int   = (int)(bmp->altitude + 0.5f);

        /* Format: "P:%4dhPa A:%4dm" -> exactly 16 characters */
        snprintf(line2, sizeof(line2), "P:%4dhPa A:%4dm", press_int, alt_int);
    }
    line2[16] = '\0';

    view_counter = (view_counter + 1) % 16;

    /* Write directly to LCD lines without clearing to prevent display flicker */
    LCD_SetCursor(0, 0);
    LCD_Print(line1);

    LCD_SetCursor(1, 0);
    LCD_Print(line2);
}
