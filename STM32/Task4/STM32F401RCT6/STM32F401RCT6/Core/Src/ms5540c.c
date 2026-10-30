/**
  ******************************************************************************
  * @file           : ms5540c.c
  * @brief          : High-Precision Depth & Pressure Driver implementation for MS5540C
  *                   STM32F401 HAL Driver with 2nd-Order Temperature Compensation
  ******************************************************************************
  */

#include "ms5540c.h"

/* Fast microsecond delay using DWT cycle counter */
static void MS5540C_DelayUs(uint32_t us) {
    /* STM32F401 Core Clock typically 16MHz to 84MHz.
     * We use simple loop or SysTick/DWT to ensure at least 2us pulse width.
     */
    volatile uint32_t count = us * (SystemCoreClock / 1000000) / 5;
    while (count--) {
        __NOP();
    }
}

static void MS5540C_ClockPulse(MS5540C_t *dev) {
    HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_SET);
    MS5540C_DelayUs(3);
    HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_RESET);
    MS5540C_DelayUs(3);
}

static void MS5540C_SendCommand(MS5540C_t *dev, uint32_t cmd, uint8_t bit_count) {
    for (int8_t i = bit_count - 1; i >= 0; i--) {
        if ((cmd >> i) & 0x01) {
            HAL_GPIO_WritePin(dev->pins.din_port, dev->pins.din_pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(dev->pins.din_port, dev->pins.din_pin, GPIO_PIN_RESET);
        }
        MS5540C_DelayUs(2);
        MS5540C_ClockPulse(dev);
    }
    HAL_GPIO_WritePin(dev->pins.din_port, dev->pins.din_pin, GPIO_PIN_RESET);
}

static uint16_t MS5540C_ReadWord(MS5540C_t *dev) {
    uint16_t result = 0;
    for (uint8_t i = 0; i < 16; i++) {
        HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_SET);
        MS5540C_DelayUs(3);
        result <<= 1;
        if (HAL_GPIO_ReadPin(dev->pins.dout_port, dev->pins.dout_pin) == GPIO_PIN_SET) {
            result |= 0x01;
        }
        HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_RESET);
        MS5540C_DelayUs(3);
    }
    return result;
}

static void MS5540C_Reset(MS5540C_t *dev) {
    /* Reset sequence: 21 pulses of alternating 1 and 0 (pattern 0x155555) */
    MS5540C_SendCommand(dev, 0x155555, 21);
    HAL_GPIO_WritePin(dev->pins.din_port, dev->pins.din_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_RESET);
    HAL_Delay(5); /* Sensor internal reset time */
}

static uint16_t MS5540C_ReadPROM(MS5540C_t *dev, uint8_t word_idx) {
    /* PROM Read Commands:
     * W1: 111 10100100 000 (0x1D20, 16-bit or 13-bit command + 3 zeros)
     * Base: 111 101001xx 000
     */
    uint16_t cmd = 0x1D20 | ((uint16_t)(word_idx & 0x03) << 4);
    MS5540C_SendCommand(dev, cmd, 16);
    return MS5540C_ReadWord(dev);
}

HAL_StatusTypeDef MS5540C_StartMCLK(TIM_HandleTypeDef *htim, uint32_t channel) {
    /* Start Timer PWM for 32.768 kHz continuous clock */
    return HAL_TIM_PWM_Start(htim, channel);
}

HAL_StatusTypeDef MS5540C_Init(MS5540C_t *dev, float water_density) {
    dev->pins.sclk_port = MS5540C_SCLK_PORT;
    dev->pins.sclk_pin  = MS5540C_SCLK_PIN;
    dev->pins.din_port  = MS5540C_DIN_PORT;
    dev->pins.din_pin   = MS5540C_DIN_PIN;
    dev->pins.dout_port = MS5540C_DOUT_PORT;
    dev->pins.dout_pin  = MS5540C_DOUT_PIN;

    dev->water_density = water_density;
    dev->surface_pressure_mbar = 1013.25f;
    dev->is_initialized = false;

    /* Initialize GPIOs: SCLK and DIN as Output, DOUT as Input */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SCLK and DIN (Output Push-Pull) */
    GPIO_InitStruct.Pin = dev->pins.sclk_pin | dev->pins.din_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(dev->pins.sclk_port, &GPIO_InitStruct);

    /* DOUT (Input with Pull-up) */
    GPIO_InitStruct.Pin = dev->pins.dout_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(dev->pins.dout_port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(dev->pins.sclk_port, dev->pins.sclk_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dev->pins.din_port, dev->pins.din_pin, GPIO_PIN_RESET);

    /* Send 21-bit Reset sequence */
    MS5540C_Reset(dev);

    /* Read PROM Calibration Words W1 to W4 */
    uint16_t w1 = MS5540C_ReadPROM(dev, 0);
    uint16_t w2 = MS5540C_ReadPROM(dev, 1);
    uint16_t w3 = MS5540C_ReadPROM(dev, 2);
    uint16_t w4 = MS5540C_ReadPROM(dev, 3);

    /* Extract C1 to C6 per MS5540C specification */
    dev->calib.c1 = (w1 >> 3) & 0x1FFF;
    dev->calib.c2 = ((w1 & 0x07) << 10) | ((w2 >> 6) & 0x03FF);
    dev->calib.c3 = (w3 >> 6) & 0x03FF;
    dev->calib.c4 = (w4 >> 7) & 0x01FF;
    dev->calib.c5 = ((w2 & 0x3F) << 6) | (w3 & 0x3F);
    dev->calib.c6 = w4 & 0x7F;

    /* Check if calibration values are valid (not all zero or 0xFFFF) */
    if (dev->calib.c1 == 0 || dev->calib.c1 == 0x1FFF) {
        return HAL_ERROR;
    }

    dev->is_initialized = true;

    /* Perform initial reading and tare surface pressure */
    if (MS5540C_Read(dev) == HAL_OK) {
        dev->surface_pressure_mbar = dev->data.pressure_mbar;
    }

    return HAL_OK;
}

HAL_StatusTypeDef MS5540C_Read(MS5540C_t *dev) {
    if (!dev->is_initialized) return HAL_ERROR;

    /* 1. Request D1 (Digital Pressure Conversion)
     * Command: 111 10100000 000 = 0x1D00 (16-bit)
     */
    MS5540C_SendCommand(dev, 0x1D00, 16);

    /* Wait for conversion completion (typically 35ms, DOUT pin pulled LOW by sensor when done) */
    uint32_t timeout = HAL_GetTick() + 45;
    while (HAL_GPIO_ReadPin(dev->pins.dout_port, dev->pins.dout_pin) == GPIO_PIN_SET) {
        if (HAL_GetTick() > timeout) break;
    }
    uint16_t d1 = MS5540C_ReadWord(dev);
    dev->data.raw_d1 = d1;

    /* 2. Request D2 (Digital Temperature Conversion)
     * Command: 111 10101000 000 = 0x1D40 (16-bit)
     */
    MS5540C_SendCommand(dev, 0x1D40, 16);

    timeout = HAL_GetTick() + 45;
    while (HAL_GPIO_ReadPin(dev->pins.dout_port, dev->pins.dout_pin) == GPIO_PIN_SET) {
        if (HAL_GetTick() > timeout) break;
    }
    uint16_t d2 = MS5540C_ReadWord(dev);
    dev->data.raw_d2 = d2;

    /* 3. Execute 2nd-order Temperature & Pressure Compensation Formulas */
    int64_t c1 = dev->calib.c1;
    int64_t c2 = dev->calib.c2;
    int64_t c3 = dev->calib.c3;
    int64_t c4 = dev->calib.c4;
    int64_t c5 = dev->calib.c5;
    int64_t c6 = dev->calib.c6;

    int64_t ut1 = (c5 << 3) + 20224;
    int64_t dt = (int64_t)d2 - ut1;

    /* Temperature in 0.1 deg C: TEMP = 200 + (dT * (C6 + 50)) / 1024 */
    int64_t temp = 200 + ((dt * (c6 + 50)) >> 10);

    /* Offset and Sensitivity calculations */
    int64_t off = (c2 << 2) + (((c4 - 512) * dt) >> 12);
    int64_t sens = c1 + ((c3 * dt) >> 10) + 24576;

    int64_t x = ((sens * ((int64_t)d1 - 7168)) >> 14) - off;
    /* Pressure in 0.1 mbar */
    int64_t p = ((x * 10) >> 5) + 25000;

    /* Second-Order Low-Temperature Correction (< 20.0 °C / temp < 200) */
    if (temp < 200) {
        int64_t t2 = (11 * (c6 + 65) * (200 - temp) * (200 - temp)) >> 11;
        int64_t p2 = (t2 * (temp - 200)) >> 4;
        temp = temp - t2;
        p = p - p2;
    }

    dev->data.temperature_c = (float)temp / 10.0f;
    dev->data.pressure_mbar = (float)p / 10.0f;

    /* 4. Calculate Depth in meters */
    /* Depth = (P_current - P_surface) * 100 / (density * g) */
    float delta_p_pa = (dev->data.pressure_mbar - dev->surface_pressure_mbar) * 100.0f;
    if (delta_p_pa < 0.0f) delta_p_pa = 0.0f; /* Ignore small negative noise on surface */

    dev->data.depth_meters = delta_p_pa / (dev->water_density * STANDARD_GRAVITY);

    return HAL_OK;
}

void MS5540C_TareSurface(MS5540C_t *dev) {
    if (dev->data.pressure_mbar > 800.0f && dev->data.pressure_mbar < 1200.0f) {
        dev->surface_pressure_mbar = dev->data.pressure_mbar;
    }
}

void MS5540C_SetWaterDensity(MS5540C_t *dev, float density_kg_m3) {
    if (density_kg_m3 > 900.0f && density_kg_m3 < 1100.0f) {
        dev->water_density = density_kg_m3;
    }
}
