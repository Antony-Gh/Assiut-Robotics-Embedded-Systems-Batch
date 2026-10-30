/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Comprehensive Solution for Task 4 (Assiut Robotics Team)
  *                   All deliverables fully implemented:
  *                     1. Task 1: I2C Communication with MPU-6050 6-DOF IMU
  *                        (Acquires Roll, Pitch, Yaw via Complementary Filter)
  *                     2. Task 2: I2C Communication with BMP280 / BMP180
  *                        (Acquires Ambient Temperature & Barometric Pressure)
  *                     3. Serial Output: Continuous Real-Time Telemetry to
  *                        Proteus Virtual Terminal via USART1 (9600 baud, 8-N-1).
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
#include "mpu6050.h"
#include "bmp280.h"
#include "uart_comm.h"
#include "lcd.h"
#include <stdio.h>
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
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/**
 * @brief Global sensor device handles
 */
MPU6050_t g_mpu;
BMP_t     g_bmp;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
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

  /* Initialize I2C1 peripheral (PB6: SCL, PB7: SDA, 100 kHz Standard Mode) */
  MX_I2C1_Init();

  /* Initialize USART1 peripheral (PA9: TX, PA10: RX, 9600 Baud, 8-N-1) */
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

  /* 1. Initialize UART Telemetry Subsystem */
  UART_Comm_Init(&huart1);
  HAL_Delay(100);

  UART_Print("\r\n=======================================================\r\n");
  UART_Print("   Assiut Robotics Team - Embedded Systems Batch\r\n");
  UART_Print("   Task 4: I2C (MPU6050 + BMP280) & UART Telemetry\r\n");
  UART_Print("=======================================================\r\n");
  UART_Print("[SYS] Initializing I2C Sensors on Bus (PB6/PB7)...\r\n");

  /* 2. Initialize MPU6050 (Task 1 Deliverable: Roll, Pitch, Yaw) */
  HAL_StatusTypeDef mpu_status = MPU6050_Init(&g_mpu, &hi2c1, 0x68);
  if (mpu_status == HAL_OK)
  {
      UART_Print("[OK]  MPU6050 Motion Sensor (GY-521) detected & initialized.\r\n");
  }
  else
  {
      UART_Print("[WARN] MPU6050 not acknowledging on 0x68. Ready for live input.\r\n");
  }

  /* 3. Initialize BMP280 / BMP180 (Task 2 Deliverable: Pressure & Temperature) */
  HAL_StatusTypeDef bmp_status = BMP_Init(&g_bmp, &hi2c1);
  if (bmp_status == HAL_OK)
  {
      if (g_bmp.sensor_type == BMP_TYPE_BMP180)
      {
          UART_Print("[OK]  BMP180 Barometric Pressure Sensor initialized.\r\n");
      }
      else
      {
          UART_Print("[OK]  BMP280 Barometric Pressure Sensor initialized.\r\n");
      }
  }
  else
  {
      UART_Print("[WARN] BMP sensor initialized with fallback standard pressure.\r\n");
  }

  UART_Print("[SYS] Streaming real-time telemetry to Virtual Terminal...\r\n");
  HAL_Delay(500);

  /* 4. Initialize 16x2 Character LCD (PA0..PA2 ctrl, PB0..PB3 4-bit data) */
  LCD_Init();
  LCD_SetCursor(0, 0);
  LCD_Print("Task 4: IMU &BMP");
  LCD_SetCursor(1, 0);
  LCD_Print("Live Telemetry  ");
  HAL_Delay(500);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* 1. Read MPU-6050 data and update orientation angles (Roll, Pitch, Yaw) */
    MPU6050_Read(&g_mpu);

    /* 2. Read BMP barometric pressure and temperature */
    BMP_Read(&g_bmp);

    /* 3. Transmit formatted telemetry block to Virtual Terminal via UART */
    UART_SendTelemetry(&g_mpu, &g_bmp);

    /* 4. Display live sensor values on 16x2 LCD (just as Task 3) */
    LCD_DisplaySensors(&g_mpu, &g_bmp);

    /* 5. Toggle Heartbeat LED (PC13) */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    /* 6. Telemetry update rate (~4 Hz = 250 ms period) */
    HAL_Delay(250);
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

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();
  /* USER CODE END I2C1_Init 0 */

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
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
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
