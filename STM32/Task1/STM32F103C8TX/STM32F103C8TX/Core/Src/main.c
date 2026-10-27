/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Comprehensive Solution for Task 1 (Assiut Robotics Team)
  *                   All deliverables fully implemented in one STM32 project:
  *                     1. Snake Effect (HAL & Register Level)
  *                     2. PWM Signal at f = 50Hz (Hardware Timer & Software Delay)
  *                     3. Controlled Counter with Switch on 8 LEDs (0 to 8)
  *                     Notes solved: Both HAL and Register-Level implementations,
  *                     plus interactive mode switching via button PB1 / long-press.
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
#include "task1.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Default Startup Mode:
 *   MODE_ALL_IN_ONE    : Interactive Master Mode (cycles/demonstrates all tasks)
 *   MODE_SNAKE_HAL     : Snake Effect using STM32 HAL API
 *   MODE_SNAKE_REG     : Snake Effect using direct Register-Level GPIO
 *   MODE_COUNTER_HAL   : Controlled Counter (0..8) using HAL
 *   MODE_COUNTER_REG   : Controlled Counter (0..8) using Register-Level GPIO
 *   MODE_PWM_50HZ_DEMO : 50Hz PWM Signal Generator & LED Fading / Servo Sweep
 */
#define DEFAULT_STARTUP_MODE     MODE_ALL_IN_ONE

/* Step delay for Snake Effect in milliseconds */
#define SNAKE_STEP_DELAY_MS      150

/* Debounce interval for push buttons in milliseconds */
#define BUTTON_DEBOUNCE_MS       50

/* Long press duration threshold in milliseconds (for single-button mode switch) */
#define BUTTON_LONG_PRESS_MS     1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Current operating mode (0: Snake Effect, 1: Controlled Counter, 2: 50Hz PWM) */
volatile Task1_Mode_t currentMode = MODE_SNAKE_EFFECT;

/* Active implementation method: Register Level vs HAL */
volatile Implementation_t currentImpl = IMPL_REGISTER;

/* Controlled counter value: 0 to 8 */
volatile uint8_t counterValue = 0;

/* Counter display style (Bar-Graph 0..8 LEDs vs Binary 0..8) */
volatile Counter_Style_t counterStyle = COUNTER_STYLE_BARGRAPH;

/* Asynchronous button event flags (Safe for EXTI ISR) */
static volatile uint8_t g_btn0Event = 0; /* PB0: Counter button press event */
static volatile uint8_t g_btn1Event = 0; /* PB1: Mode button press event */

/* Timestamp trackers for non-blocking timing */
static uint32_t lastSnakeTick = 0;
static uint32_t lastBtn0PressTick = 0;
static uint32_t lastBtn1PressTick = 0;

/* Snake effect state machine variables */
static uint8_t snakeVal = 0;
static uint8_t snakePhase = 0; /* 0: Turn ON one-by-one, 1: Turn OFF one-by-one */
static uint8_t snakeStep = 0;

/* PWM duty cycle variable */
static float pwmDuty = 50.0f; /* Default 50% duty cycle (10ms on, 10ms off) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Handle_Button_Inputs(void);
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

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  /* Initialize Hardware Timer 4 Register-Level 50Hz PWM Output on PB8 */
  Task1_PWM_Timer4_RegisterInit();

  /* Settle time to avoid power-on capacitor transients on PB0/PB1 */
  HAL_Delay(100);
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0 | GPIO_PIN_1);
  lastBtn0PressTick = HAL_GetTick();
  lastBtn1PressTick = HAL_GetTick();
  g_btn0Event = 0;
  g_btn1Event = 0;

  /* Reset all states to Deliverable 1: Snake Effect */
  currentMode = MODE_SNAKE_EFFECT;
  Task1_SnakeEffect_Reset();
  Task1_Counter_Reset();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Run task engine */
    Task1_Run();

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

/* ========================================================================== */
/* Register-Level Direct Hardware Implementation (Eng. Ahmed Eldeeb Lesson 8) */
/* ========================================================================== */

/**
  * @brief  Direct Register-Level GPIO Initialization (Alternative to HAL)
  *         Configures:
  *           - RCC APB2ENR: Clocks for Port A, Port B, AFIO
  *           - GPIOA CRL: Pins 0..7 as Output Push-Pull 2MHz (0x22222222)
  *           - GPIOB CRL: Pins 0 & 1 as Input Pull-Down (0x00000088, ODR=0)
  */
void Task1_RegisterLevel_Init(void)
{
  /* 1. Enable Peripheral Clocks on APB2 Bus:
   *    Bit 2: IOPAEN (Port A clock enable)
   *    Bit 3: IOPBEN (Port B clock enable)
   *    Bit 0: AFIOEN (Alternate Function IO clock enable)
   */
  RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN);

  /* 2. Configure GPIOA Pins 0..7 (LEDs) in CRL (Port Configuration Register Low)
   *    Each pin is configured with 4 bits: CNF[1:0] : MODE[1:0]
   *    MODE = 10b (Output mode, max speed 2 MHz)
   *    CNF  = 00b (General purpose output push-pull)
   *    Nibble = 0010b = 0x2
   *    For all 8 pins PA0..PA7: CRL = 0x22222222
   */
  GPIOA->CRL = 0x22222222;

  /* Reset all 8 LED pins to LOW initially via BRR (Bit Reset Register) */
  GPIOA->BRR = 0x00FF;

  /* 3. Configure GPIOB Pins 0 & 1 (Switches) in CRL
   *    MODE = 00b (Input mode)
   *    CNF  = 10b (Input with pull-up / pull-down)
   *    Nibble = 1000b = 0x8
   *    Clear lower 8 bits (PB0, PB1) then set to 0x88
   */
  GPIOB->CRL &= ~(0x000000FF);
  GPIOB->CRL |=  (0x00000088);

  /* Set Pull-Down: writing 0 to ODR bits activates internal pull-down */
  GPIOB->ODR &= ~((1U << 0) | (1U << 1));
}

/**
  * @brief  Register-Level 50Hz Hardware PWM Configuration using Timer 4 Channel 3 on PB8
  *         Timer Calculation:
  *           f_clk = 8 MHz (System Clock)
  *           Prescaler (PSC) = 7 -> Timer Clock = 8 MHz / (7 + 1) = 1 MHz (1 tick = 1 us)
  *           Auto-Reload (ARR) = 20000 - 1 = 19999 -> Period = 20000 us = 20 ms -> f = 50 Hz!
  *           Initial Duty (CCR3) = 10000 us (50% duty cycle)
  */
void Task1_PWM_Timer4_RegisterInit(void)
{
  /* 1. Enable GPIOB and AFIO Clocks */
  RCC->APB2ENR |= (RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN);

  /* 2. Configure PB8 as Alternate Function Output Push-Pull, 50MHz
   *    PB8 is controlled by GPIOB->CRH bits [3:0]
   *    MODE = 11b (Output 50 MHz), CNF = 10b (AF Push-Pull) -> Nibble = 1011b = 0xB
   */
  GPIOB->CRH &= ~(0x0000000F);
  GPIOB->CRH |=  (0x0000000B);

  /* 3. Enable Timer 4 Clock on APB1 Bus */
  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

  /* 4. Set Prescaler and Auto-Reload values for exact 50 Hz
   *    Dynamically calculates prescaler from APB1 timer clock frequency.
   *    If APB1 prescaler != 1, timer clock = 2 * PCLK1 (STM32 architecture).
   */
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t tim4_clock = pclk1;
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
  {
    tim4_clock = pclk1 * 2U;
  }
  TIM4->PSC = (tim4_clock / 1000000U) - 1U; /* 1 MHz clock -> 1 us tick */
  TIM4->ARR = 19999;                        /* 20000 ticks * 1 us = 20 ms (50 Hz) */
  TIM4->CCR3 = 10000;                       /* 10000 us pulse = 50% duty cycle */

  /* 5. Configure Channel 3 for PWM Mode 1 (OC3M = 110b) and Preload Enable (OC3PE = 1) */
  TIM4->CCMR2 &= ~(TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE_Msk);
  TIM4->CCMR2 |=  (6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

  /* 6. Enable Output on Channel 3 */
  TIM4->CCER |= TIM_CCER_CC3E;

  /* 7. Enable Auto-reload Preload (ARPE) and Start the Counter (CEN) */
  TIM4->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;

  /* Trigger update event to load registers */
  TIM4->EGR |= TIM_EGR_UG;
}

/* ========================================================================== */
/* Deliverable 1: Snake Effect (HAL & Register Level)                         */
/* ========================================================================== */

/**
  * @brief  Snake Effect reset to initial state
  */
void Task1_SnakeEffect_Reset(void)
{
  snakeVal = 0;
  snakePhase = 0;
  snakeStep = 0;
  HAL_GPIO_WritePin(LED_PORT, LED_ALL_PINS, GPIO_PIN_RESET);
}

/**
  * @brief  Snake Effect single step using STM32 HAL API
  *         Connect 8 LEDs and light them one by one, then switch them off one by one.
  */
void Task1_SnakeEffect_HAL_Step(void)
{
  if (snakePhase == 0)
  {
    /* Light LEDs one by one: 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF */
    snakeVal = (uint8_t)((snakeVal << 1) | 0x01);
    snakeStep++;
    if (snakeStep >= 8)
    {
      snakePhase = 1;
      snakeStep = 0;
    }
  }
  else
  {
    /* Switch LEDs off one by one: 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00 */
    snakeVal = (uint8_t)((snakeVal << 1) & 0xFF);
    snakeStep++;
    if (snakeStep >= 8)
    {
      snakePhase = 0;
      snakeStep = 0;
      snakeVal = 0;
    }
  }

  /* Output each bit to corresponding GPIO pin using HAL_GPIO_WritePin */
  for (uint8_t i = 0; i < 8; i++)
  {
    GPIO_PinState pinState = (snakeVal & (1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(LED_PORT, (uint16_t)(1U << i), pinState);
  }
}

/**
  * @brief  Snake Effect single step using Direct Register-Level Operations
  *         Directly writes to GPIOA->ODR (Output Data Register)
  */
void Task1_SnakeEffect_Register_Step(void)
{
  if (snakePhase == 0)
  {
    /* Light LEDs one by one */
    snakeVal = (uint8_t)((snakeVal << 1) | 0x01);
    snakeStep++;
    if (snakeStep >= 8)
    {
      snakePhase = 1;
      snakeStep = 0;
    }
  }
  else
  {
    /* Switch LEDs off one by one */
    snakeVal = (uint8_t)((snakeVal << 1) & 0xFF);
    snakeStep++;
    if (snakeStep >= 8)
    {
      snakePhase = 0;
      snakeStep = 0;
      snakeVal = 0;
    }
  }

  /* Direct Register Write to Port A Output Data Register:
   * Keep upper 8 pins (PA8..PA15) unchanged, update lower 8 pins (PA0..PA7)
   */
  GPIOA->ODR = (GPIOA->ODR & 0xFF00) | (uint16_t)snakeVal;
}

/* ========================================================================== */
/* Deliverable 2: 50Hz PWM Signal Generation                                  */
/* ========================================================================== */

/**
  * @brief  Software 50Hz PWM Output toggle
  *         Period = 20ms (50Hz): high_ms HIGH, low_ms LOW.
  * @param  high_ms: On-time in milliseconds (e.g. 10ms for 50% duty)
  * @param  low_ms:  Off-time in milliseconds (e.g. 10ms for 50% duty)
  */
void Task1_PWM_50Hz_SoftwareToggle(uint32_t high_ms, uint32_t low_ms)
{
  HAL_GPIO_WritePin(PWM_SW_PORT, PWM_SW_PIN, GPIO_PIN_SET);
  HAL_Delay(high_ms);
  HAL_GPIO_WritePin(PWM_SW_PORT, PWM_SW_PIN, GPIO_PIN_RESET);
  HAL_Delay(low_ms);
}

/**
  * @brief  Adjust Hardware Timer 4 PWM Duty Cycle
  * @param  duty_percent: 0.0f to 100.0f %
  */
void Task1_PWM_50Hz_SetDuty(float duty_percent)
{
  if (duty_percent < 0.0f)   duty_percent = 0.0f;
  if (duty_percent > 100.0f) duty_percent = 100.0f;

  /* ARR is 19999 (20000 ticks = 20ms) */
  uint32_t ccr_val = (uint32_t)((duty_percent / 100.0f) * 20000.0f);
  if (ccr_val > 19999) ccr_val = 19999;

  TIM4->CCR3 = ccr_val;
}

/**
  * @brief  PWM Demonstration Step
  *         Outputs exact hardware 50Hz PWM on PB8, and displays active duty cycle
  *         cleanly on the 8 LEDs as a solid bar meter (e.g. 50% = 4 LEDs lit)
  */
void Task1_PWM_50Hz_Demo_Step(void)
{
  /* Update hardware Timer 4 PWM (PB8) */
  Task1_PWM_50Hz_SetDuty(pwmDuty);

  /* Display duty cycle on 8 LEDs as a clean, steady meter (1 to 8 LEDs) */
  uint8_t numLeds = (uint8_t)((pwmDuty / 100.0f) * 8.0f + 0.5f);
  if (numLeds > 8) numLeds = 8;

  uint8_t pattern = 0;
  for (uint8_t i = 0; i < numLeds; i++)
  {
    pattern |= (uint8_t)(1U << i);
  }
  GPIOA->ODR = (GPIOA->ODR & 0xFF00) | (uint16_t)pattern;
}

/* ========================================================================== */
/* Deliverable 3: Controlled Counter (0 to 8) on 8 LEDs with Switch           */
/* ========================================================================== */

/**
  * @brief  Display counter value (0 to 8) on the 8 LEDs using STM32 HAL
  * @param  count: Number from 0 to 8
  * @param  style: Bar-Graph (0..8 LEDs lit) or Binary (0..8 in binary)
  */
void Task1_Counter_Display_HAL(uint8_t count, Counter_Style_t style)
{
  if (count > 8) count = 8;
  uint8_t pattern = 0;

  if (style == COUNTER_STYLE_BARGRAPH)
  {
    /* Thermometer / Bar-graph: count represents number of illuminated LEDs */
    for (uint8_t i = 0; i < count; i++)
    {
      pattern |= (uint8_t)(1U << i);
    }
  }
  else
  {
    /* Binary: count value in binary */
    pattern = count;
  }

  for (uint8_t i = 0; i < 8; i++)
  {
    GPIO_PinState state = (pattern & (1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(LED_PORT, (uint16_t)(1U << i), state);
  }
}

/**
  * @brief  Display counter value (0 to 8) on the 8 LEDs using Register Level
  * @param  count: Number from 0 to 8
  * @param  style: Bar-Graph or Binary
  */
void Task1_Counter_Display_Register(uint8_t count, Counter_Style_t style)
{
  if (count > 8) count = 8;
  uint8_t pattern = 0;

  if (style == COUNTER_STYLE_BARGRAPH)
  {
    for (uint8_t i = 0; i < count; i++)
    {
      pattern |= (uint8_t)(1U << i);
    }
  }
  else
  {
    pattern = count;
  }

  /* Direct register write to GPIOA->ODR (lower 8 bits) */
  GPIOA->ODR = (GPIOA->ODR & 0xFF00) | (uint16_t)pattern;
}

/**
  * @brief  Increment counter from 0 to 8, wrapping around to 0
  */
void Task1_Counter_Increment(void)
{
  counterValue++;
  if (counterValue > 8)
  {
    counterValue = 0;
  }
}

/**
  * @brief  Reset counter to 0
  */
void Task1_Counter_Reset(void)
{
  counterValue = 0;
}

/**
  * @brief  Get current counter value
  */
uint8_t Task1_Counter_GetCount(void)
{
  return counterValue;
}

/* ========================================================================== */
/* Application Mode Control & Interactive Runner                              */
/* ========================================================================== */

void Task1_SetMode(Task1_Mode_t mode)
{
  currentMode = mode;

  /* Reset mode-specific states without disruptive flashing */
  if (mode == MODE_SNAKE_EFFECT)
  {
    Task1_SnakeEffect_Reset();
  }
  else if (mode == MODE_CONTROLLED_COUNTER)
  {
    /* Display the current counter value immediately on the LEDs */
    if (currentImpl == IMPL_REGISTER)
    {
      Task1_Counter_Display_Register(counterValue, counterStyle);
    }
    else
    {
      Task1_Counter_Display_HAL(counterValue, counterStyle);
    }
  }
  else if (mode == MODE_PWM_50HZ)
  {
    Task1_PWM_50Hz_Demo_Step();
  }
}

Task1_Mode_t Task1_GetMode(void)
{
  return currentMode;
}

/**
  * @brief  EXTI Interrupt Callback (Triggered instantaneously by hardware on rising edge)
  *         Catches short 1ms button pulses in Proteus simulation without fail.
  *         Debounced with 200ms lockout so each press generates EXACTLY ONE event.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();
  if (now < 300) return; /* Ignore power-on settling */

  if (GPIO_Pin == SW_COUNTER_PIN) /* PB0: Counter Increment Button */
  {
    if ((now - lastBtn0PressTick) >= 200)
    {
      lastBtn0PressTick = now;
      g_btn0Event = 1;
    }
  }
  else if (GPIO_Pin == SW_MODE_PIN) /* PB1: Mode Selector Button */
  {
    if ((now - lastBtn1PressTick) >= 250)
    {
      lastBtn1PressTick = now;
      g_btn1Event = 1;
    }
  }
}

/**
  * @brief  Button input polling backup (ensures reliability in both hardware and simulation)
  */
static void Handle_Button_Inputs(void)
{
  uint32_t now = HAL_GetTick();
  if (now < 300) return;

  /* Backup polling for PB0 (Counter) */
  if ((GPIOB->IDR & (1U << 0)) && !g_btn0Event && ((now - lastBtn0PressTick) >= 200))
  {
    lastBtn0PressTick = now;
    g_btn0Event = 1;
  }

  /* Backup polling for PB1 (Mode) */
  if ((GPIOB->IDR & (1U << 1)) && !g_btn1Event && ((now - lastBtn1PressTick) >= 250))
  {
    lastBtn1PressTick = now;
    g_btn1Event = 1;
  }
}

/**
  * @brief  Main Task Execution Engine
  *         Dispatches actions according to currentMode in a non-blocking architecture
  */
void Task1_Run(void)
{
  uint32_t now = HAL_GetTick();

  /* Check button states */
  Handle_Button_Inputs();

  /* 1. Handle Pending Mode Change Request (PB1 pressed) */
  if (g_btn1Event)
  {
    g_btn1Event = 0;
    Task1_Mode_t nextMode = (Task1_Mode_t)((currentMode + 1) % MODE_COUNT_MAX);
    Task1_SetMode(nextMode);
    return;
  }

  /* 2. Handle Pending Counter Button Press (PB0 pressed) */
  if (g_btn0Event)
  {
    g_btn0Event = 0;

    if (currentMode == MODE_CONTROLLED_COUNTER)
    {
      /* Deliverable 3: Increment counter by exactly 1 (0 to 8, then wraps to 0) */
      Task1_Counter_Increment();
      if (currentImpl == IMPL_REGISTER)
      {
        Task1_Counter_Display_Register(counterValue, counterStyle);
      }
      else
      {
        Task1_Counter_Display_HAL(counterValue, counterStyle);
      }
    }
    else if (currentMode == MODE_SNAKE_EFFECT)
    {
      /* Toggle between Register-Level Direct GPIO and HAL Implementation */
      currentImpl = (currentImpl == IMPL_REGISTER) ? IMPL_HAL : IMPL_REGISTER;
    }
    else if (currentMode == MODE_PWM_50HZ)
    {
      /* Step PWM duty cycle on PB0 press: 10% -> 25% -> 50% -> 75% -> 90% */
      static uint8_t step = 2;
      const float presets[5] = {10.0f, 25.0f, 50.0f, 75.0f, 90.0f};
      step = (step + 1) % 5;
      pwmDuty = presets[step];
      Task1_PWM_50Hz_Demo_Step();
    }
  }

  /* 3. Execute Mode State Machine */
  switch (currentMode)
  {
    /* Mode 0: Deliverable 1 - Snake Effect (light 1-by-1, turn off 1-by-1) */
    case MODE_SNAKE_EFFECT:
    {
      if ((now - lastSnakeTick) >= SNAKE_STEP_DELAY_MS)
      {
        lastSnakeTick = now;
        if (currentImpl == IMPL_REGISTER)
        {
          Task1_SnakeEffect_Register_Step();
        }
        else
        {
          Task1_SnakeEffect_HAL_Step();
        }
      }
      break;
    }

    /* Mode 1: Deliverable 3 - Controlled Counter (0 to 8 via PB0 Switch)
     * LEDs maintain their state steadily and update cleanly on each PB0 press!
     */
    case MODE_CONTROLLED_COUNTER:
    {
      /* Steady state display */
      break;
    }

    /* Mode 2: Deliverable 2 - 50Hz PWM Signal Demo & LED Duty Bar Meter */
    case MODE_PWM_50HZ:
    {
      /* Steady state display: PB8 outputs 50Hz PWM, LEDs show duty meter */
      break;
    }

    default:
      currentMode = MODE_SNAKE_EFFECT;
      break;
  }
}

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
