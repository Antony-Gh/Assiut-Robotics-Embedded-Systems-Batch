/**
  ******************************************************************************
  * @file           : task1.h
  * @brief          : Header file for Task 1 Solutions (Assiut Robotics Team)
  *                   - Deliverable 1: Snake Effect (HAL & Register Level)
  *                   - Deliverable 2: 50Hz PWM Signal (Hardware & Software)
  *                   - Deliverable 3: Controlled Counter 0-8 with Switch (HAL & Reg)
  ******************************************************************************
  */

#ifndef __TASK1_H
#define __TASK1_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ========================================================================== */
/* Clock Configuration Options                                                */
/* ========================================================================== */
#define CLOCK_CONFIG_HSI_8MHZ       0  /* Internal 8 MHz RC Oscillator (Standard for Proteus) */
#define CLOCK_CONFIG_HSE_8MHZ       1  /* External Crystal 8 MHz (Direct HSE) */
#define CLOCK_CONFIG_HSE_72MHZ      2  /* External Crystal 8 MHz + PLL -> 72 MHz Max Speed */

#define SELECTED_CLOCK_CONFIG       CLOCK_CONFIG_HSI_8MHZ

/* ========================================================================== */
/* Pin Mapping Definitions                                                    */
/* ========================================================================== */

/* 8 LEDs connected to Port A Pins 0 through 7 */
#define LED_PORT            GPIOA
#define LED0_PIN            GPIO_PIN_0
#define LED1_PIN            GPIO_PIN_1
#define LED2_PIN            GPIO_PIN_2
#define LED3_PIN            GPIO_PIN_3
#define LED4_PIN            GPIO_PIN_4
#define LED5_PIN            GPIO_PIN_5
#define LED6_PIN            GPIO_PIN_6
#define LED7_PIN            GPIO_PIN_7
#define LED_ALL_PINS        (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                             GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)

/* Switch 1: Controlled Counter Increment Button (PB0) */
#define SW_COUNTER_PORT     GPIOB
#define SW_COUNTER_PIN      GPIO_PIN_0

/* Switch 2: Mode Selector Button (PB1) */
#define SW_MODE_PORT        GPIOB
#define SW_MODE_PIN         GPIO_PIN_1

/* Dedicated Hardware PWM Output Pin: PB8 (TIM4 Channel 3) */
#define PWM_TIM4_PORT       GPIOB
#define PWM_TIM4_PIN        GPIO_PIN_8

/* Software PWM Output Pin (Can use PA0 or PB8) */
#define PWM_SW_PORT         GPIOA
#define PWM_SW_PIN          GPIO_PIN_0

/* ========================================================================== */
/* Application Modes                                                          */
/* ========================================================================== */

typedef enum {
    MODE_ALL_IN_ONE = 0,    /* Mode 0: Interactive System - Switch modes via button PB1 / long press */
    MODE_SNAKE_HAL,         /* Mode 1: Snake Effect using STM32 HAL API */
    MODE_SNAKE_REG,         /* Mode 2: Snake Effect using direct Register-Level GPIO */
    MODE_COUNTER_HAL,       /* Mode 3: Controlled Counter (0..8) using STM32 HAL */
    MODE_COUNTER_REG,       /* Mode 4: Controlled Counter (0..8) using direct Register-Level GPIO */
    MODE_PWM_50HZ_DEMO,     /* Mode 5: 50Hz PWM signal generator with LED fading / servo sweep */
    MODE_COUNT_MAX
} Task1_Mode_t;

/* Counter Display Styles */
typedef enum {
    COUNTER_STYLE_BARGRAPH = 0,  /* 0: all off, 1: 1 LED on, ..., 8: 8 LEDs on */
    COUNTER_STYLE_BINARY         /* 0..8 displayed in binary on 8 LEDs */
} Counter_Style_t;

/* ========================================================================== */
/* Function Prototypes                                                        */
/* ========================================================================== */

/* Initialization */
void Task1_Init(void);
void Task1_RegisterLevel_Init(void);
void Task1_HAL_Init(void);
void Task1_PWM_Timer4_RegisterInit(void);

/* Deliverable 1: Snake Effect */
void Task1_SnakeEffect_HAL_Step(void);
void Task1_SnakeEffect_Register_Step(void);
void Task1_SnakeEffect_Reset(void);

/* Deliverable 2: 50Hz PWM Signal */
void Task1_PWM_50Hz_SoftwareToggle(uint32_t high_ms, uint32_t low_ms);
void Task1_PWM_50Hz_SetDuty(float duty_percent);
void Task1_PWM_50Hz_Demo_Step(void);

/* Deliverable 3: Controlled Counter */
void Task1_Counter_Display_HAL(uint8_t count, Counter_Style_t style);
void Task1_Counter_Display_Register(uint8_t count, Counter_Style_t style);
void Task1_Counter_Increment(void);
void Task1_Counter_Reset(void);
uint8_t Task1_Counter_GetCount(void);

/* Main Application Runner */
void Task1_Run(void);
void Task1_SetMode(Task1_Mode_t mode);
Task1_Mode_t Task1_GetMode(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK1_H */
