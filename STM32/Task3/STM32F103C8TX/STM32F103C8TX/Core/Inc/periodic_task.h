/**
 * @file periodic_task.h
 * @brief Periodic Timer-Driven Engine for Ultrasonic Trigger and LCD Display
 * @author Assiut Robotics Team
 * @date 2026-10-28
 *
 * @details Runs periodic Ultrasonic measurements and LCD updates via hardware Timer 3 (TIM3)
 *          without executing any code inside the main() while(1) super loop.
 */

#ifndef __PERIODIC_TASK_H__
#define __PERIODIC_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "ultrasonic.h"
#include "lcd.h"

extern Ultrasonic_t g_ultrasonic;

/**
 * @brief Initializes TIM3 to fire an interrupt every 100 ms for periodic measurements and display.
 * @retval None
 */
void PeriodicTask_Init(void);

/**
 * @brief TIM3 Interrupt Handler callback for the 100 ms periodic task.
 * @retval None
 */
void PeriodicTask_TIM3_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* __PERIODIC_TASK_H__ */
