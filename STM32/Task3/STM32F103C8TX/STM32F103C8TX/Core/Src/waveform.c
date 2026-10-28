/**
 * @file waveform.c
 * @brief Implementation of Periodic Custom Pulse Waveform Generator
 * @author Assiut Robotics Team
 * @date 2026-10-28
 *
 * @details Generates an asymmetric periodic pulse waveform on PA5 and PB8:
 *          - Phase 0: 20 us HIGH (10 us to 30 us)
 *          - Phase 1: 10 us LOW  (30 us to 40 us)
 *          - Phase 2:  5 us HIGH (40 us to 45 us)
 *          - Phase 3: 25 us LOW  (15 us end of cycle + 10 us start of next = 25 us)
 *          Total repeating period = 60 us (f = 16.67 kHz).
 */

#include "waveform.h"

/* State machine variable for pulse phases */
static volatile uint8_t g_wave_state = 0;

/**
 * @brief Initializes TIM2 and GPIO output pins (PA5 and PB8) for pulse generation.
 * @retval None
 */
void Waveform_Init(void)
{
    /* Enable GPIOA and GPIOB clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure PA5 and PB8 as High-Speed Push-Pull outputs */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Initially set both pins LOW */
    GPIOA->BSRR = (1U << (5 + 16));
    GPIOB->BSRR = (1U << (8 + 16));

    /* Enable TIM2 peripheral clock */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Configure TIM2:
     * Clock = 8 MHz, Prescaler = 7 -> 1 MHz counter frequency (1 tick = 1 microsecond).
     * Start with 10 microseconds LOW before first pulse
     */
    TIM2->PSC = 7;
    TIM2->ARR = 10 - 1;
    TIM2->CNT = 0;

    /* Enable Update Interrupt */
    TIM2->DIER |= TIM_DIER_UIE;

    /* Configure NVIC for TIM2 with highest priority (0) */
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    /* Start TIM2 counter */
    g_wave_state = 0;
    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Interrupt handler callback for TIM2 state transitions.
 * @details Transitions through the 4 states of the 60 us waveform:
 *          - State 0: 10 us LOW finished -> Drive HIGH for 20 us
 *          - State 1: 20 us HIGH finished -> Drive LOW for 10 us
 *          - State 2: 10 us LOW finished -> Drive HIGH for 5 us
 *          - State 3: 5 us HIGH finished -> Drive LOW for 25 us (15 us + 10 us of next cycle)
 * @retval None
 */
void Waveform_TIM2_ISR(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR = ~TIM_SR_UIF; /* Clear update interrupt flag */

        switch (g_wave_state)
        {
            case 0:
                /* Phase 0 finished (initial 10us LOW) -> Drive HIGH (3.3V) for 20 us */
                GPIOA->BSRR = (1U << 5);
                GPIOB->BSRR = (1U << 8);
                TIM2->ARR = 20 - 1; /* Next interval: 20 us */
                TIM2->CNT = 0;      /* Reset counter immediately to prevent 0xFFFF rollover */
                g_wave_state = 1;
                break;

            case 1:
                /* Phase 1 finished (20us HIGH) -> Drive LOW (0V) for 10 us */
                GPIOA->BSRR = (1U << (5 + 16));
                GPIOB->BSRR = (1U << (8 + 16));
                TIM2->ARR = 10 - 1; /* Next interval: 10 us */
                TIM2->CNT = 0;
                g_wave_state = 2;
                break;

            case 2:
                /* Phase 2 finished (10us LOW) -> Drive HIGH (3.3V) for 5 us */
                GPIOA->BSRR = (1U << 5);
                GPIOB->BSRR = (1U << 8);
                TIM2->ARR = 5 - 1;  /* Next interval: 5 us */
                TIM2->CNT = 0;
                g_wave_state = 3;
                break;

            case 3:
            default:
                /* Phase 3 finished (5us HIGH) -> Drive LOW (0V) for 25 us (15us + 10us = 25us) */
                GPIOA->BSRR = (1U << (5 + 16));
                GPIOB->BSRR = (1U << (8 + 16));
                TIM2->ARR = 25 - 1; /* Next interval: 25 us */
                TIM2->CNT = 0;
                g_wave_state = 0;
                break;
        }
    }
}
