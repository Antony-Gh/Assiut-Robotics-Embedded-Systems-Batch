/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Comprehensive Solution for Task 3 (Assiut Robotics Team)
  *                   All deliverables fully implemented:
  *                     1. Ultrasonic Distance Measurement displayed on 16x2 LCD
  *                     2. Precise Custom Periodic Waveform (60 us period) on Oscilloscope
  *                     3. Strict Super-Loop Constraint: while(1) is completely empty!
  *                        All operations are driven by Hardware Timers & EXTI Interrupts.
  *                     4. Modular Architecture & Full Doxygen Documentation.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "ultrasonic.h"
#include "waveform.h"
#include "periodic_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/**
 * @brief Global Ultrasonic sensor instance configured for Trig = PA3, Echo = PA4.
 */
Ultrasonic_t g_ultrasonic = {
    .trig = { .port = GPIOA, .pin = GPIO_PIN_3 },
    .echo = { .port = GPIOA, .pin = GPIO_PIN_4 },
    .distance = 0,
    .echo_start = 0,
    .echo_width = 0,
    .new_data_ready = 0
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock (8 MHz HSI) */
  SystemClock_Config();

  /* Initialize all configured GPIO pins */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  /* 1. Initialize 16x2 Character LCD in 8-bit mode (PA0..PA2 ctrl, PB0..PB7 data) */
  LCD_Init();

  /* 2. Initialize HC-SR04 Ultrasonic Sensor (Trig: PA3, Echo: PA4, Timebase: TIM1) */
  Ultrasonic_Init(&g_ultrasonic);

  /* 3. Initialize Oscilloscope Periodic Pulse Waveform Generator on PA5 / PB8 (TIM2) */
  Waveform_Init();

  /* 4. Initialize 100 ms Periodic Task Engine (TIM3) for Sensor Trigger and LCD Refresh */
  PeriodicTask_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /*
     * STRICT TASK 3 REQUIREMENT:
     * "Perform the Prev task without writing any line inside the super Loop."
     *
     * The super-loop is kept completely empty!
     * 1. Waveform Generation: Driven by TIM2 Update Interrupt State Machine (10us/20us/10us/5us/15us).
     * 2. Echo Measurement: Driven by EXTI4 Rising/Falling Interrupts & TIM1 microsecond counter.
     * 3. Periodic Measurement & LCD Updates: Driven by TIM3 100ms Periodic Interrupt.
     */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
