/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for Task 2 (Assiut Robotics Team)
  *                   Read 4 Potentiometers using ADC + DMA and display on 16x2 LCD
  ******************************************************************************
  * @attention
  *
  * Assiut Robotics Embedded Systems Batch - Task 2
  * Deliverables:
  *   - Read 4 Potentiometer analog inputs (PA3, PA4, PA5, PA6).
  *   - Store sampled values in memory buffer using DMA1 Channel 1 (Circular Mode).
  *   - Print all 4 values on a 16x2 Character LCD (HD44780 / LM016L).
  *   - LCD Driver modularized in external files (lcd.h / lcd.c) with Doxygen documentation.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "lcd.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CHANNELS_COUNT    4U

/**
  * @brief Configurable Nominal Maximum Resistances for Potentiometers:
  *        P1: 1k Ohm   (PA3 - ADC1_IN3)
  *        P2: 10k Ohm  (PA4 - ADC1_IN4)
  *        P3: 100k Ohm (PA5 - ADC1_IN5)
  *        P4: 1M Ohm   (PA6 - ADC1_IN6)
  */
#define POT1_MAX_RESISTANCE   1000.0f      /*!< 1 kOhm   */
#define POT2_MAX_RESISTANCE   10000.0f     /*!< 10 kOhm  */
#define POT3_MAX_RESISTANCE   100000.0f    /*!< 100 kOhm */
#define POT4_MAX_RESISTANCE   1000000.0f   /*!< 1 MOhm   */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* USER CODE BEGIN PV */
/**
  * @brief  DMA Destination Buffer: Stores 12-bit ADC conversion values (0 to 4095)
  *         Index 0: PA3 (ADC1_IN3 - Potentiometer 1)
  *         Index 1: PA4 (ADC1_IN4 - Potentiometer 2)
  *         Index 2: PA5 (ADC1_IN5 - Potentiometer 3)
  *         Index 3: PA6 (ADC1_IN6 - Potentiometer 4)
  */
volatile uint16_t adc_dma_buffer[ADC_CHANNELS_COUNT] = {0};

/**
  * @brief Array of maximum nominal resistances corresponding to each ADC channel
  */
static const float POT_MAX_OHMS[ADC_CHANNELS_COUNT] = {
  POT1_MAX_RESISTANCE,
  POT2_MAX_RESISTANCE,
  POT3_MAX_RESISTANCE,
  POT4_MAX_RESISTANCE
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Converts a 12-bit ADC reading (0..4095) into the actual potentiometer resistance,
  *         formatted as an engineering-unit string strictly truncated to at most 4 characters.
  * @param  adc_val: Raw 12-bit ADC conversion value (0..4095)
  * @param  r_max: Nominal maximum resistance in Ohms (e.g. 1000, 10000, 100000, 1000000)
  * @param  out_str: Output character buffer (minimum 5 bytes for 4 chars + null terminator)
  * @retval None
  */
static void Format_Resistance(uint16_t adc_val, float r_max, char *out_str)
{
  float r = ((float)(adc_val & 0x0FFFU) / 4095.0f) * r_max;

  if (r < 0.5f)
  {
    /* Minimum / Zero position */
    snprintf(out_str, 5, "0");
  }
  else if (r_max <= 1000.0f && r <= 1000.5f)
  {
    /* 1k Potentiometer: 0 to 1000 Ohms fit directly in 1 to 4 digits (e.g. "520", "1000") */
    snprintf(out_str, 5, "%u", (unsigned int)(r + 0.5f));
  }
  else if (r < 995.0f)
  {
    /* Under 1k Ohms on larger pots: display integer ohms */
    snprintf(out_str, 5, "%u", (unsigned int)(r + 0.5f));
  }
  else if (r < 9950.0f)
  {
    /* 1k to 9.9k: 1 decimal place with 'K' suffix (e.g. "1.0K", "5.2K", "9.9K") */
    float k_val = r / 1000.0f;
    if (k_val >= 9.95f)
    {
      snprintf(out_str, 5, "%uK", (unsigned int)(k_val + 0.5f));
    }
    else
    {
      snprintf(out_str, 5, "%.1fK", (double)k_val);
    }
  }
  else if (r < 999500.0f)
  {
    /* 10k to 999k: integer k with 'K' suffix (e.g. "10K", "52K", "100K", "520K") */
    snprintf(out_str, 5, "%uK", (unsigned int)((r / 1000.0f) + 0.5f));
  }
  else
  {
    /* 1M and above: 1 decimal place with 'M' suffix (e.g. "1.0M") */
    snprintf(out_str, 5, "%.1fM", (double)(r / 1000000.0f));
  }

  /* Explicitly enforce truncation if string exceeds 4 characters */
  out_str[4] = '\0';
}
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

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */

  /* 1. Initialize 16x2 Character LCD */
  LCD_Init();

  /* 2. Display Welcome Screen */
  LCD_SetCursor(0, 0);
  LCD_SendString("Assiut Robotics ");
  LCD_SetCursor(1, 0);
  LCD_SendString("Task 2: ADC+DMA ");
  HAL_Delay(1200);
  LCD_Clear();

  /* 3. Run ADC Calibration */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_Delay(50);

  /* 4. Start ADC Multi-Channel Conversions with DMA in Circular Mode */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_CHANNELS_COUNT) != HAL_OK)
  {
    Error_Handler();
  }

  /* In Circular DMA mode, the DMA controller transfers ADC samples into SRAM
   * autonomously without CPU intervention. Disabling the DMA interrupt eliminates
   * Proteus simulation CPU load warnings caused by thousands of ISRs per second.
   */
  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);

  /* Individual 4-character resistance string buffers */
  char p1_str[8];
  char p2_str[8];
  char p3_str[8];
  char p4_str[8];

  /* Buffers for formatted LCD string lines (16 characters + null terminator) */
  char line1[32];
  char line2[32];

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Convert raw ADC values to formatted resistance strings (strictly <= 4 chars each) */
    Format_Resistance(adc_dma_buffer[0], POT_MAX_OHMS[0], p1_str);
    Format_Resistance(adc_dma_buffer[1], POT_MAX_OHMS[1], p2_str);
    Format_Resistance(adc_dma_buffer[2], POT_MAX_OHMS[2], p3_str);
    Format_Resistance(adc_dma_buffer[3], POT_MAX_OHMS[3], p4_str);

    /* Format Line 1: P1 and P2 (exactly 16 characters: "P1:xxxx  P2:xxxx") */
    snprintf(line1, sizeof(line1), "P1:%-4s  P2:%-4s", p1_str, p2_str);

    /* Format Line 2: P3 and P4 (exactly 16 characters: "P3:xxxx  P4:xxxx") */
    snprintf(line2, sizeof(line2), "P3:%-4s  P4:%-4s", p3_str, p4_str);

    /* Update LCD screen without clearing to eliminate screen flickering */
    LCD_SetCursor(0, 0);
    LCD_SendString(line1);

    LCD_SetCursor(1, 0);
    LCD_SendString(line2);

    /* Refresh rate: 100 ms */
    HAL_Delay(100);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  *        Configures the system clock to 8 MHz (HSI standard for Proteus)
  *        and sets ADC clock prescaler to DIV2 (4 MHz).
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure ADC clock: 8 MHz / 6 = 1.33 MHz (Valid range: 0.6 MHz - 14 MHz) */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  *        Configures 4 regular channels (PA3, PA4, PA5, PA6) in scan continuous mode.
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Common ADC1 Configuration
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = ADC_CHANNELS_COUNT;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel Rank 1: Channel 3 (PA3 - Potentiometer 1)
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel Rank 2: Channel 4 (PA4 - Potentiometer 2)
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel Rank 3: Channel 5 (PA5 - Potentiometer 3)
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel Rank 4: Channel 6 (PA6 - Potentiometer 4)
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Enable DMA controller clock and configure NVIC interrupt
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA1_Channel1_IRQn: In circular mode, DMA runs autonomously in hardware.
   * Disabling IRQ eliminates thousands of interrupts per second in simulation.
   */
  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
}

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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
