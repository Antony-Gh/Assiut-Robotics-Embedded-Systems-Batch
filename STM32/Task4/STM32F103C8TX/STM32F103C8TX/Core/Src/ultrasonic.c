/**
 * @file ultrasonic.c
 * @brief Implementation of HC-SR04 Ultrasonic Distance Sensor Driver
 * @author Assiut Robotics Team
 * @date 2026-10-28
 *
 * @details Implements non-blocking pulse measurement using STM32 hardware
 *          timer TIM1 configured with 1 us resolution and EXTI edge detection.
 */

#include "ultrasonic.h"

/**
 * @brief Microsecond delay helper using inline CPU loop
 * @param[in] us Duration in microseconds to delay
 * @retval None
 */
static void Ultrasonic_DelayUs(uint32_t us)
{
    /* At 8 MHz SYSCLK, 1 us is ~8 cycles. Each loop iteration takes ~3-4 cycles. */
    volatile uint32_t count = us * 2;
    while (count--)
    {
        __NOP();
    }
}

/**
 * @brief Initializes the Ultrasonic Sensor GPIO pins and microsecond timer.
 * @param[in,out] us Pointer to an Ultrasonic_t configuration structure.
 * @retval None
 */
void Ultrasonic_Init(Ultrasonic_t *us)
{
    if (us == NULL) return;

    /* Enable GPIO Clock for Trig and Echo ports */
    if (us->trig.port == GPIOA || us->echo.port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    if (us->trig.port == GPIOB || us->echo.port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure Trigger Pin as Output Push-Pull, High-Speed */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = us->trig.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(us->trig.port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(us->trig.port, us->trig.pin, GPIO_PIN_RESET);

    /* Configure Echo Pin as Input with EXTI Interrupt on Both Rising & Falling Edges */
    GPIO_InitStruct.Pin = us->echo.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(us->echo.port, &GPIO_InitStruct);

    /* Configure and enable EXTI interrupt in NVIC for the Echo pin */
    if (us->echo.pin == GPIO_PIN_4)
    {
        HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    }

    /* Configure TIM1 as a 1-microsecond free-running timebase counter */
    __HAL_RCC_TIM1_CLK_ENABLE();
    TIM1->PSC = 7;          /* 8 MHz / (7 + 1) = 1 MHz -> 1 us per tick */
    TIM1->ARR = 0xFFFF;     /* 16-bit maximum auto-reload */
    TIM1->CR1 = TIM_CR1_CEN;/* Enable Timer 1 counter */

    /* Initialize runtime state variables */
    us->distance = 0;
    us->echo_start = 0;
    us->echo_width = 0;
    us->new_data_ready = 0;
}

/**
 * @brief Sends a 10-microsecond trigger pulse on the TRIG pin.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval None
 */
void Ultrasonic_Trigger(Ultrasonic_t *us)
{
    if (us == NULL) return;

    /* Pull TRIG HIGH for 10 microseconds */
    HAL_GPIO_WritePin(us->trig.port, us->trig.pin, GPIO_PIN_SET);
    Ultrasonic_DelayUs(10);
    HAL_GPIO_WritePin(us->trig.port, us->trig.pin, GPIO_PIN_RESET);
}

/**
 * @brief Interrupt Service Routine callback for Echo pin edge transitions.
 * @param[in,out] us Pointer to an Ultrasonic_t configuration structure.
 * @param[in] GPIO_Pin Pin number that triggered the EXTI interrupt.
 * @retval None
 */
void Ultrasonic_Echo_ISR(Ultrasonic_t *us, uint16_t GPIO_Pin)
{
    if (us == NULL || GPIO_Pin != us->echo.pin) return;

    if (HAL_GPIO_ReadPin(us->echo.port, us->echo.pin) == GPIO_PIN_SET)
    {
        /* Rising Edge: Echo pulse has begun, record timestamp from TIM1 */
        us->echo_start = TIM1->CNT;
    }
    else
    {
        /* Falling Edge: Echo pulse has ended, calculate elapsed microseconds */
        uint32_t echo_end = TIM1->CNT;
        uint32_t pulse_width;

        if (echo_end >= us->echo_start)
        {
            pulse_width = echo_end - us->echo_start;
        }
        else
        {
            /* Timer overflow handling across 0xFFFF */
            pulse_width = (0x10000UL - us->echo_start) + echo_end;
        }

        us->echo_width = pulse_width;
        
        /* Distance in cm = pulse_width (us) / 58 (Speed of sound = 340 m/s = 0.034 cm/us) */
        us->distance = pulse_width / 58U;
        us->new_data_ready = 1;
    }
}

/**
 * @brief Retrieves the latest distance measurement in centimeters.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval uint32_t Distance in centimeters.
 */
uint32_t Ultrasonic_GetDistanceCm(const Ultrasonic_t *us)
{
    if (us == NULL) return 0;
    return us->distance;
}

/**
 * @brief Retrieves the latest distance measurement in inches.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval uint32_t Distance in inches.
 */
uint32_t Ultrasonic_GetDistanceInch(const Ultrasonic_t *us)
{
    if (us == NULL) return 0;
    return us->echo_width / 148U;
}
