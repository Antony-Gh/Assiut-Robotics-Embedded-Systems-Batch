/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ROV Multi-Sensor Telemetry & Driver Suite
  *                   STM32F401RCT6
  *                   Sensors: GY-521 (MPU-6050), GY-87 (10-DOF), MS5540C (Depth)
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
#include "libjpeg.h"
#include "mbedtls.h"
#include "pdm2pcm.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensor_config.h"
#include "mpu6050.h"
#include "hmc5883l.h"
#include "bmp180.h"
#include "gy87.h"
#include "ms5540c.h"
#include "logger.h"
#include "usb_device.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEVICE_FS 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Sensor Handles */
MPU6050_t gy521_sensor;
GY87_t    gy87_sensor;
MS5540C_t ms5540_sensor;

/* Telemetry loop rate */
#define TELEMETRY_INTERVAL_MS 50 /* 20 Hz telemetry streaming rate */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
static void MX_I2C1_InitPins(GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin);
static void AutoDetectAndInitSensors(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    LOGGER_TxCpltCallback(huart);
}

static GPIO_TypeDef *s_active_i2c_port = GPIOB;
static uint16_t      s_active_scl_pin  = GPIO_PIN_8;
static uint16_t      s_active_sda_pin  = GPIO_PIN_9;
static bool          s_i2c_pins_locked = false;

/* Microsecond delay helper for bit-banging */
static void I2C_DelayUs(uint32_t us)
{
    volatile uint32_t count = us * 14; /* ~84MHz Cortex-M4 cycle calibration */
    while (count--) {
        __NOP();
    }
}

/* Fast bit-bang I2C address probe (works on any pin pair, immune to hardware I2C state lockups) */
static bool I2C_ProbePinPair(GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin, uint8_t addr_7bit)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 1. Configure pins as Open-Drain Outputs with internal Pull-Up */
    GPIO_InitStruct.Pin = scl_pin | sda_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    /* Release lines HIGH */
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    I2C_DelayUs(10);

    /* Pulse SCL up to 9 times to unstick any slave holding SDA */
    for (int k = 0; k < 9; k++) {
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
        I2C_DelayUs(5);
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
        I2C_DelayUs(5);
        if (HAL_GPIO_ReadPin(port, sda_pin) == GPIO_PIN_SET) break;
    }

    /* Bit-bang START condition: SDA goes low while SCL is high */
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
    I2C_DelayUs(5);
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
    I2C_DelayUs(5);

    /* Send 8 bits of (addr_7bit << 1) [Write command] */
    uint8_t byte = (addr_7bit << 1);
    for (int b = 7; b >= 0; b--) {
        if ((byte >> b) & 1) {
            HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
        }
        I2C_DelayUs(3);
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
        I2C_DelayUs(5);
        HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
        I2C_DelayUs(3);
    }

    /* Release SDA to read ACK */
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    I2C_DelayUs(3);
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    I2C_DelayUs(5);

    /* Read ACK bit (0 = ACK from chip, 1 = NACK / no chip) */
    bool ack = (HAL_GPIO_ReadPin(port, sda_pin) == GPIO_PIN_RESET);

    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_RESET);
    I2C_DelayUs(3);

    /* Bit-bang STOP condition: SDA goes low, SCL goes high, SDA goes high */
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_RESET);
    I2C_DelayUs(3);
    HAL_GPIO_WritePin(port, scl_pin, GPIO_PIN_SET);
    I2C_DelayUs(5);
    HAL_GPIO_WritePin(port, sda_pin, GPIO_PIN_SET);
    I2C_DelayUs(10);

    return ack;
}

static void MX_I2C1_InitPins(GPIO_TypeDef *port, uint16_t scl_pin, uint16_t sda_pin)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Force reset I2C1 peripheral registers */
    __HAL_RCC_I2C1_FORCE_RESET();
    HAL_Delay(2);
    __HAL_RCC_I2C1_RELEASE_RESET();

    /* Configure selected pins with AF4_I2C1 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = scl_pin | sda_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    /* Initialize hardware I2C1 at 100 kHz */
    __HAL_RCC_I2C1_CLK_ENABLE();
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void AutoDetectAndInitSensors(void)
{
#if (ENABLE_GY521 || ENABLE_GY87)
    uint8_t detected_addr = 0;
    const char *detected_pins = NULL;

    if (!s_i2c_pins_locked) {
#if (I2C1_PIN_SELECTION == 0 || I2C1_PIN_SELECTION == 2)
        /* Test PB8 (SCL) and PB9 (SDA) */
        if (I2C_ProbePinPair(GPIOB, GPIO_PIN_8, GPIO_PIN_9, 0x68)) {
            detected_addr = 0x68;
            s_active_scl_pin = GPIO_PIN_8;
            s_active_sda_pin = GPIO_PIN_9;
            detected_pins = "PB8 (SCL) & PB9 (SDA)";
            s_i2c_pins_locked = true;
        } else if (I2C_ProbePinPair(GPIOB, GPIO_PIN_8, GPIO_PIN_9, 0x69)) {
            detected_addr = 0x69;
            s_active_scl_pin = GPIO_PIN_8;
            s_active_sda_pin = GPIO_PIN_9;
            detected_pins = "PB8 (SCL) & PB9 (SDA)";
            s_i2c_pins_locked = true;
        }
#endif

#if (I2C1_PIN_SELECTION == 1 || I2C1_PIN_SELECTION == 2)
        /* If not found, test PB6 (SCL) and PB7 (SDA) */
        if (!s_i2c_pins_locked) {
            if (I2C_ProbePinPair(GPIOB, GPIO_PIN_6, GPIO_PIN_7, 0x68)) {
                detected_addr = 0x68;
                s_active_scl_pin = GPIO_PIN_6;
                s_active_sda_pin = GPIO_PIN_7;
                detected_pins = "PB6 (SCL) & PB7 (SDA)";
                s_i2c_pins_locked = true;
            } else if (I2C_ProbePinPair(GPIOB, GPIO_PIN_6, GPIO_PIN_7, 0x69)) {
                detected_addr = 0x69;
                s_active_scl_pin = GPIO_PIN_6;
                s_active_sda_pin = GPIO_PIN_7;
                detected_pins = "PB6 (SCL) & PB7 (SDA)";
                s_i2c_pins_locked = true;
            }
        }
#endif

        if (s_i2c_pins_locked) {
            LOG_INFO("==================================================");
            LOG_INFO(">>> SUCCESS: Sensor ACKed on %s! <<<", detected_pins);
            LOG_INFO(">>> Device I2C Address: 0x%02X <<<", detected_addr);
            LOG_INFO("==================================================");
            MX_I2C1_InitPins(s_active_i2c_port, s_active_scl_pin, s_active_sda_pin);
        } else {
            LOG_WARN("[I2C SCAN] No device ACK on PB8/PB9 OR PB6/PB7! Check: 1) VCC to 3.3V/5V, 2) GND, 3) SCL to PB8/PB6, 4) SDA to PB9/PB7.");
            return;
        }
    }

#if ENABLE_GY521
    if (!gy521_sensor.is_initialized && s_i2c_pins_locked) {
        uint8_t addr_to_use = detected_addr ? detected_addr : 0x68;
        LOG_INFO("Initializing GY-521 (MPU-6050 @ 0x%02X)...", addr_to_use);
        if (MPU6050_Init(&gy521_sensor, &hi2c1, (addr_to_use << 1),
                         MPU6050_ACCEL_FS_4G, MPU6050_GYRO_FS_500DPS, MPU6050_DLPF_44HZ) == HAL_OK) {
            LOG_INFO(">>> GY-521 (MPU-6050) ONLINE & CALIBRATED! <<<");
        } else {
            LOG_WARN("MPU6050_Init returned error at 0x%02X.", addr_to_use);
        }
    }
#endif

#if ENABLE_GY87
    if (!gy87_sensor.is_initialized && s_i2c_pins_locked) {
        LOG_INFO("Initializing GY-87 10-DOF (MPU @ 0x68 + Mag + Baro)...");
        if (GY87_Init(&gy87_sensor, &hi2c1, (0x68 << 1), LOCAL_MAGNETIC_DECLINATION_DEG) == HAL_OK) {
            LOG_INFO(">>> GY-87 10-DOF ONLINE! <<<");
        }
    }
#endif
#endif /* ENABLE_GY521 || ENABLE_GY87 */

#if ENABLE_MS5540C
    if (!ms5540_sensor.is_initialized) {
        if (MS5540C_Init(&ms5540_sensor, ROV_WATER_DENSITY) == HAL_OK) {
            LOG_INFO(">>> MS5540C Depth Sensor ONLINE! Baseline: %.1f mbar <<<", ms5540_sensor.surface_pressure_mbar);
        }
    }
#endif
}
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
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_CRC_Init();
  MX_USB_DEVICE_Init();
  MX_LIBJPEG_Init();
  MX_MBEDTLS_Init();
  MX_PDM2PCM_Init();
  /* USER CODE BEGIN 2 */

  /* 1. Initialize DMA Logger */
  LOGGER_Init(&huart2);
  HAL_Delay(100);
  LOG_INFO("========================================");
  LOG_INFO("   AQUASYNC ROV Multi-Sensor Telemetry  ");
  LOG_INFO("   STM32F401RCT6 Navigation Core       ");
  LOG_INFO("   Single-Sensor Verification Mode      ");
  LOG_INFO("========================================");

#if ENABLE_MS5540C
  /* Start 32.768 kHz continuous MCLK on PA0 for MS5540C */
  LOG_INFO("Starting 32.768 kHz MCLK on PA0 for MS5540C...");
  MS5540C_StartMCLK(&htim2, TIM_CHANNEL_1);
#endif

  /* 2. Run Initial Sensor Auto-Detection */
  AutoDetectAndInitSensors();

  LOG_INFO("Telemetry streaming at 20 Hz (interval = %d ms)...", TELEMETRY_INTERVAL_MS);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_telemetry_tick = HAL_GetTick();
  uint32_t last_attitude_tick  = HAL_GetTick();
#if ENABLE_GY87
  uint32_t last_mag_tick       = HAL_GetTick();
#endif
#if ENABLE_MS5540C
  uint32_t last_depth_tick     = HAL_GetTick();
#endif
  uint32_t last_scan_tick      = HAL_GetTick();
  uint32_t last_diag_tick      = HAL_GetTick();

  while (1)
  {
    uint32_t current_tick = HAL_GetTick();

    /* 1. Fast attitude update for GY-521 and GY-87 MPU (50 Hz, every 20ms) */
    if ((current_tick - last_attitude_tick) >= 20) {
        float dt = (float)(current_tick - last_attitude_tick) / 1000.0f;
        last_attitude_tick = current_tick;
#if ENABLE_GY521
        if (gy521_sensor.is_initialized) {
            MPU6050_UpdateAttitude(&gy521_sensor, dt);
        }
#endif
#if ENABLE_GY87
        if (gy87_sensor.is_initialized) {
            if (MPU6050_UpdateAttitude(&gy87_sensor.mpu, dt) == HAL_OK) {
                gy87_sensor.data.ax_g       = gy87_sensor.mpu.data.ax_g;
                gy87_sensor.data.ay_g       = gy87_sensor.mpu.data.ay_g;
                gy87_sensor.data.az_g       = gy87_sensor.mpu.data.az_g;
                gy87_sensor.data.gx_dps     = gy87_sensor.mpu.data.gx_dps;
                gy87_sensor.data.gy_dps     = gy87_sensor.mpu.data.gy_dps;
                gy87_sensor.data.gz_dps     = gy87_sensor.mpu.data.gz_dps;
                gy87_sensor.data.mpu_temp_c = gy87_sensor.mpu.data.temp_c;
                gy87_sensor.data.roll_deg   = gy87_sensor.mpu.data.roll_deg;
                gy87_sensor.data.pitch_deg  = gy87_sensor.mpu.data.pitch_deg;
            }
        }
#endif
    }

#if ENABLE_GY87
    /* 2. Low-frequency GY-87 Magnetometer and Barometer update (10 Hz, every 100ms) */
    if ((current_tick - last_mag_tick) >= 100) {
        last_mag_tick = current_tick;
        if (gy87_sensor.is_initialized) {
            if (HMC5883L_Read(&gy87_sensor.mag) == HAL_OK) {
                gy87_sensor.data.mx_uT       = gy87_sensor.mag.data.mx_uT;
                gy87_sensor.data.my_uT       = gy87_sensor.mag.data.my_uT;
                gy87_sensor.data.mz_uT       = gy87_sensor.mag.data.mz_uT;
                gy87_sensor.data.heading_deg = gy87_sensor.mag.data.heading_deg;
            }
            if (BMP180_Read(&gy87_sensor.baro) == HAL_OK) {
                gy87_sensor.data.pressure_hpa = gy87_sensor.baro.data.pressure_hpa;
                gy87_sensor.data.baro_temp_c  = gy87_sensor.baro.data.temperature_c;
                gy87_sensor.data.altitude_m   = gy87_sensor.baro.data.altitude_m;
            }
        }
    }
#endif

#if ENABLE_MS5540C
    /* 3. MS5540C Depth / Pressure update (4 Hz, every 250ms) */
    if ((current_tick - last_depth_tick) >= 250) {
        last_depth_tick = current_tick;
        if (ms5540_sensor.is_initialized) {
            MS5540C_Read(&ms5540_sensor);
        }
    }
#endif

    /* 4. Periodic Telemetry Stream (20 Hz, every 50ms) */
    if ((current_tick - last_telemetry_tick) >= TELEMETRY_INTERVAL_MS) {
        last_telemetry_tick = current_tick;
#if TELEMETRY_USE_JSON
        LOGGER_SendTelemetryJSON(&gy521_sensor.data, &gy87_sensor.data, &ms5540_sensor.data);
#else
        LOGGER_SendTelemetryCSV(&gy521_sensor.data, &gy87_sensor.data, &ms5540_sensor.data);
#endif
    }

#if SENSOR_LIVE_DIAGNOSTICS
    /* Live Text Diagnostics in Console (every 1 second) */
    if ((current_tick - last_diag_tick) >= 1000) {
        last_diag_tick = current_tick;
#if ENABLE_GY521
        if (gy521_sensor.is_initialized) {
            char s_p[16], s_r[16], s_ax[16], s_ay[16], s_az[16];
            ftoa_custom(s_p, gy521_sensor.data.pitch_deg, 1);
            ftoa_custom(s_r, gy521_sensor.data.roll_deg, 1);
            ftoa_custom(s_ax, gy521_sensor.data.ax_g, 2);
            ftoa_custom(s_ay, gy521_sensor.data.ay_g, 2);
            ftoa_custom(s_az, gy521_sensor.data.az_g, 2);
            LOG_INFO("[GY-521 LIVE] Pitch: %s deg | Roll: %s deg | Accel: X=%s Y=%s Z=%s g",
                     s_p, s_r, s_ax, s_ay, s_az);
        }
#endif
    }
#endif

    /* 5. Auto-retry probe every 2.5 seconds if active sensor not initialized */
    if ((current_tick - last_scan_tick) >= 2500) {
        last_scan_tick = current_tick;
        bool need_probe = false;
#if ENABLE_GY521
        if (!gy521_sensor.is_initialized) need_probe = true;
#endif
#if ENABLE_GY87
        if (!gy87_sensor.is_initialized) need_probe = true;
#endif
#if ENABLE_MS5540C
        if (!ms5540_sensor.is_initialized) need_probe = true;
#endif
        if (need_probe) {
            AutoDetectAndInitSensors();
        }
    }

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_CRC_DR_RESET(&hcrc);
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_ENABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_ENABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 2;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
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
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC0 PC1 PC2
                           PC3 PC4 PC5 PC6
                           PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
