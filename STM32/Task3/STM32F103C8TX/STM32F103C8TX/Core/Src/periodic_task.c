/**
 * @file periodic_task.c
 * @brief Implementation of Periodic Timer-Driven Engine
 * @author Assiut Robotics Team
 * @date 2026-10-28
 */

#include "periodic_task.h"
#include <stdio.h>

/**
 * @brief Initializes TIM3 for a 100 ms update period.
 * @details Clock = 8 MHz.
 *          PSC = 7999 -> 1 kHz timer frequency (1 tick = 1 ms).
 *          ARR = 99 -> 100 ms period (10 Hz).
 * @retval None
 */
void PeriodicTask_Init(void)
{
    /* Enable TIM3 clock */
    __HAL_RCC_TIM3_CLK_ENABLE();

    TIM3->PSC = 7999;       /* 8 MHz / 8000 = 1000 Hz (1 ms tick) */
    TIM3->ARR = 100 - 1;    /* 100 ms period */
    TIM3->CNT = 0;

    /* Enable TIM3 update interrupt */
    TIM3->DIER |= TIM_DIER_UIE;

    /* Configure NVIC for TIM3 */
    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    /* Start TIM3 */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief TIM3 Interrupt Handler callback for the 100 ms periodic task.
 * @retval None
 */
void PeriodicTask_TIM3_ISR(void)
{
    if (TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR = ~TIM_SR_UIF; /* Clear flag */

        /* 1. Trigger the Ultrasonic sensor */
        Ultrasonic_Trigger(&g_ultrasonic);

        /* 2. Format and display measured distance on LCD */
        char line1[17];
        char line2[17];
        uint32_t dist = Ultrasonic_GetDistanceCm(&g_ultrasonic);

        if (dist > 400U || dist == 0U)
        {
            snprintf(line1, sizeof(line1), "Dist: OutRange  ");
        }
        else
        {
            snprintf(line1, sizeof(line1), "Dist: %3lu cm    ", (unsigned long)dist);
        }

        snprintf(line2, sizeof(line2), "O-Scope: PA5/PB8");

        /* Overwrite LCD lines directly (no clearing, eliminates flicker) */
        LCD_SetCursor(0, 0);
        LCD_Print(line1);

        LCD_SetCursor(1, 0);
        LCD_Print(line2);
    }
}
