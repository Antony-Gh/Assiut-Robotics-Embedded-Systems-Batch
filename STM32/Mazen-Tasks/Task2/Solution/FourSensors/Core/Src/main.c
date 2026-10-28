/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* USER CODE BEGIN PV */
#define DISPLAY_RESISTANCE 1  /* 1 = display resistance (Ohms/K/M), 0 = display raw ADC (0-4095) */

/* DMA circular buffer for 4 ADC channels - must be volatile and in static/global RAM */
volatile uint16_t adc_data[4] = {0};

/* Nominal maximum resistance for potentiometers: P1=1K, P2=10K, P3=100K, P4=1M */
static const float POT_NOMINAL_OHMS[4] = {1000.0f, 10000.0f, 100000.0f, 1000000.0f};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Formats ADC measurement into a resistance string strictly <= 4 chars
  * @param  adc_val: Raw 12-bit ADC value (0-4095)
  * @param  r_max: Nominal potentiometer maximum resistance in Ohms
  * @param  out_str: Output character buffer (minimum 5 bytes for 4 chars + null terminator)
  */
static void Format_Resistance(uint16_t adc_val, float r_max, char *out_str)
{
  char temp[16];
  uint32_t raw_adc = (uint32_t)(adc_val & 0x0FFFU);
  uint32_t r = (uint32_t)(((uint64_t)raw_adc * (uint64_t)(r_max + 0.5f) + 2047ULL) / 4095ULL);

  if (r == 0U)
  {
    snprintf(temp, sizeof(temp), "0");
  }
  else if (r_max <= 1000.0f && r <= 1000U)
  {
    snprintf(temp, sizeof(temp), "%lu", (unsigned long)r);
  }
  else if (r < 1000U)
  {
    snprintf(temp, sizeof(temp), "%lu", (unsigned long)r);
  }
  else if (r < 9950U)
  {
    uint32_t val_x10 = (r + 50U) / 100U;
    if (val_x10 >= 100U)
    {
      snprintf(temp, sizeof(temp), "10K");
    }
    else
    {
      uint32_t whole = val_x10 / 10U;
      uint32_t frac = val_x10 % 10U;
      snprintf(temp, sizeof(temp), "%lu.%luK", (unsigned long)whole, (unsigned long)frac);
    }
  }
  else if (r < 999500U)
  {
    uint32_t k_val = (r + 500U) / 1000U;
    snprintf(temp, sizeof(temp), "%luK", (unsigned long)k_val);
  }
  else
  {
    uint32_t val_x10 = (r + 50000U) / 100000U;
    uint32_t whole = val_x10 / 10U;
    uint32_t frac = val_x10 % 10U;
    snprintf(temp, sizeof(temp), "%lu.%luM", (unsigned long)whole, (unsigned long)frac);
  }

  strncpy(out_str, temp, 4);
  out_str[4] = '\0';
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	// 2.5/5  12bit 011111111101
	// 111111111010



  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  LCD_8 l = {
  		  .EN  = { .port = GPIOA, .pin = GPIO_PIN_0 },
  		  .RS  = { .port = GPIOA, .pin = GPIO_PIN_1 },
  		  .RW  = { .port = GPIOA, .pin = GPIO_PIN_2 },
  		  .DB7 = { .port = GPIOB, .pin = GPIO_PIN_7 },
  		  .DB6 = { .port = GPIOB, .pin = GPIO_PIN_6 },
  		  .DB5 = { .port = GPIOB, .pin = GPIO_PIN_5 },
  		  .DB4 = { .port = GPIOB, .pin = GPIO_PIN_4 },
  		  .DB3 = { .port = GPIOB, .pin = GPIO_PIN_3 },
  		  .DB2 = { .port = GPIOB, .pin = GPIO_PIN_2 },
  		  .DB1 = { .port = GPIOB, .pin = GPIO_PIN_1 },
  		  .DB0 = { .port = GPIOB, .pin = GPIO_PIN_0 }
    };
    vidLCDInit(l);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  /* Display initial welcome screen */
  vidLCDScreenClr(l);
  vidLCDSetCursor(l, R1, C1);
  vidLCDWriteStr(l, (uint8_t *)"Assiut Robotics");
  vidLCDSetCursor(l, R2, C1);
  vidLCDWriteStr(l, (uint8_t *)"Task 2: ADC+DMA");
  HAL_Delay(1000);

  /* Calibrate ADC */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_Delay(50);

  /* Start ADC Conversions with DMA in Circular Mode */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_data, 4) != HAL_OK)
  {
    Error_Handler();
  }

  /* Disable DMA Interrupt in NVIC to eliminate Proteus CPU load freeze (~4000 ISRs/sec).
   * Circular DMA transfers data into adc_data in SRAM continuously in hardware. */
  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);

  vidLCDScreenClr(l);

  char p1_str[8], p2_str[8], p3_str[8], p4_str[8];
  char line1[32];
  char line2[32];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if DISPLAY_RESISTANCE
    /* Convert raw ADC values to formatted resistance strings (max 4 chars each) */
    Format_Resistance(adc_data[0], POT_NOMINAL_OHMS[0], p1_str);
    Format_Resistance(adc_data[1], POT_NOMINAL_OHMS[1], p2_str);
    Format_Resistance(adc_data[2], POT_NOMINAL_OHMS[2], p3_str);
    Format_Resistance(adc_data[3], POT_NOMINAL_OHMS[3], p4_str);

    /* Exactly 16 characters per line: "P1:xxxx  P2:xxxx" */
    snprintf(line1, sizeof(line1), "P1:%-4s  P2:%-4s", p1_str, p2_str);
    snprintf(line2, sizeof(line2), "P3:%-4s  P4:%-4s", p3_str, p4_str);
#else
    /* Raw ADC values (0-4095) formatted to 4 chars */
    snprintf(line1, sizeof(line1), "P1:%-4u  P2:%-4u", adc_data[0], adc_data[1]);
    snprintf(line2, sizeof(line2), "P3:%-4u  P4:%-4u", adc_data[2], adc_data[3]);
#endif

    /* Overwrite LCD lines directly without clearing (eliminates screen flicker) */
    vidLCDSetCursor(l, R1, C1);
    vidLCDWriteStr(l, (uint8_t *)line1);

    vidLCDSetCursor(l, R2, C1);
    vidLCDWriteStr(l, (uint8_t *)line2);

    HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
