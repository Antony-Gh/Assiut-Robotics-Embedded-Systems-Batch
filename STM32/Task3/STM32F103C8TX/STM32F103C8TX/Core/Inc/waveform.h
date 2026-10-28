/**
 * @file waveform.h
 * @brief Periodic Custom Pulse Waveform Generator for Oscilloscope Inspection
 * @author Assiut Robotics Team
 * @date 2026-10-28
 *
 * @details Generates the Task 3 periodic pulse waveform:
 *          - 0 to 10 us: LOW (0V)
 *          - 10 to 30 us: HIGH (3.3V) [Width = 20 us]
 *          - 30 to 40 us: LOW (0V)    [Width = 10 us]
 *          - 40 to 45 us: HIGH (3.3V) [Width = 5 us]
 *          - 45 to 60 us: LOW (0V)    [Rest of period, then repeats]
 *          Driven by hardware Timer 2 (TIM2) interrupt state machine with zero super-loop overhead.
 */

#ifndef __WAVEFORM_H__
#define __WAVEFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Initializes TIM2 and GPIO pins (PA5 and PB8) for periodic pulse waveform generation.
 * @retval None
 */
void Waveform_Init(void);

/**
 * @brief Interrupt handler callback for TIM2 state transitions.
 * @retval None
 */
void Waveform_TIM2_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* __WAVEFORM_H__ */
