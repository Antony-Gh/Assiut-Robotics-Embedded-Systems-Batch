# 10. Integrated Robotics Case Studies

The preceding chapters analyzed internal Flash programming, hardware quadrature decoding, low-power duty cycling, linker script memory maps, actuator electronics, watchdog timers, custom bootloaders, and DAC synthesis as distinct technical modules.

In real-world robotics engineering, **these systems must operate concurrently inside a unified firmware architecture**.

This chapter presents five comprehensive, end-to-end case studies demonstrating how these advanced concepts integrate to solve complex robotics challenges.

---

## 10.1 Case Study 1: Differential-Drive Robot Odometry & Closed-Loop Velocity PID

A mobile autonomous robot must track its trajectory accurately and maintain constant wheel speed regardless of floor friction, slope inclinations, or battery voltage sag.

```text
       Closed-Loop Velocity Control Architecture
 Desired Velocity ──► (+) ──► [ PID Controller ] ──► [ TIM1 PWM (20 kHz) ] ──► TB6612FNG Driver
 (m/s)                 ▲                                                            │
                       │ (-) Error                                                  ▼
                       │                                                       DC Gearmotor
                       └────── [ TIM2 / TIM3 Encoders (50 Hz) ] ◄──────────── Wheel & Optical Disc
                               (Counts -> RPM -> Linear Speed)
```

### 10.1.1 Hardware Connection Map

**Table 10.1 — Differential-drive robot hardware pin assignment**

| Subsystem | Hardware Component | STM32 Pin Assignment | Peripheral & Operating Mode |
|---|---|---|---|
| **Left Encoder** | 500 PPR Optical Quadrature | `PA0` (Ch A) / `PA1` (Ch B) | `TIM2`: Encoder Mode TI1 and TI2 ($4\times$) |
| **Right Encoder**| 500 PPR Optical Quadrature | `PA6` (Ch A) / `PA7` (Ch B) | `TIM3`: Encoder Mode TI1 and TI2 ($4\times$) |
| **Left Motor PWM**| TB6612FNG PWMA | `PA8` | `TIM1_CH1`: 20 kHz PWM Mode 1 |
| **Right Motor PWM**| TB6612FNG PWMB | `PA9` | `TIM1_CH2`: 20 kHz PWM Mode 1 |
| **Left Motor Direction**| TB6612FNG AIN1 / AIN2 | `PB12` / `PB13` | GPIO Push-Pull Output |
| **Right Motor Direction**| TB6612FNG BIN1 / BIN2 | `PB14` / `PB15` | GPIO Push-Pull Output |
| **Driver Standby** | TB6612FNG STBY | `PC13` | GPIO Push-Pull Output (Pulled HIGH) |
| **Control Loop Timer** | 50 Hz Periodic Interrupt | Internal Silicon | `TIM4`: Update Interrupt every $20.0\text{ ms}$ |

---

### 10.1.2 Verified Closed-Loop PID Velocity Implementation

**Listing 10.1 — 50 Hz closed-loop motor PID velocity control loop**

```c
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float max_output;
} PID_Controller_t;

static PID_Controller_t left_pid  = { .kp = 2500.0f, .ki = 350.0f, .kd = 45.0f, .integral = 0.0f, .prev_error = 0.0f, .max_output = 3600.0f };
static PID_Controller_t right_pid = { .kp = 2500.0f, .ki = 350.0f, .kd = 45.0f, .integral = 0.0f, .prev_error = 0.0f, .max_output = 3600.0f };

/* Target setpoints in meters per second */
volatile float target_vel_left_mps  = 0.50f; /* 0.5 m/s */
volatile float target_vel_right_mps = 0.50f;

/* Measured state */
volatile float measured_vel_left_mps  = 0.0f;
volatile float measured_vel_right_mps = 0.0f;

static uint16_t prev_cnt_left  = 0;
static uint16_t prev_cnt_right = 0;

#define WHEEL_CIRCUMFERENCE_M   (3.14159265f * 0.065f) /* 65 mm diameter */
#define ENCODER_CPR             2000.0f                /* 500 PPR * 4 */
#define DT_SECONDS              0.020f                 /* 50 Hz = 20 ms */

static float PID_Compute(PID_Controller_t *pid, float setpoint, float measured)
{
    float error = setpoint - measured;
    
    /* Proportional term */
    float p_out = pid->kp * error;
    
    /* Integral term with anti-windup clamping */
    pid->integral += error * DT_SECONDS;
    if (pid->integral > 5.0f)  pid->integral = 5.0f;
    if (pid->integral < -5.0f) pid->integral = -5.0f;
    float i_out = pid->ki * pid->integral;
    
    /* Derivative term */
    float derivative = (error - pid->prev_error) / DT_SECONDS;
    float d_out = pid->kd * derivative;
    pid->prev_error = error;
    
    float total_out = p_out + i_out + d_out;
    if (total_out > pid->max_output)  total_out = pid->max_output;
    if (total_out < -pid->max_output) total_out = -pid->max_output;
    
    return total_out;
}

/* 50 Hz Hardware Timer Interrupt Callback (TIM4) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        /* 1. Read hardware encoder counters */
        uint16_t curr_cnt_left  = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
        uint16_t curr_cnt_right = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
        
        /* 2. Compute signed delta ticks using 16-bit modular trick */
        int16_t delta_left  = (int16_t)(curr_cnt_left - prev_cnt_left);
        int16_t delta_right = (int16_t)(curr_cnt_right - prev_cnt_right);
        
        prev_cnt_left  = curr_cnt_left;
        prev_cnt_right = curr_cnt_right;
        
        /* 3. Convert ticks to linear wheel velocity (m/s) */
        measured_vel_left_mps  = ((float)delta_left / ENCODER_CPR) * (WHEEL_CIRCUMFERENCE_M / DT_SECONDS);
        measured_vel_right_mps = ((float)delta_right / ENCODER_CPR) * (WHEEL_CIRCUMFERENCE_M / DT_SECONDS);
        
        /* 4. Execute PID Control */
        float left_effort  = PID_Compute(&left_pid, target_vel_left_mps, measured_vel_left_mps);
        float right_effort = PID_Compute(&right_pid, target_vel_right_mps, measured_vel_right_mps);
        
        /* 5. Command Left Motor H-Bridge */
        if (left_effort >= 0.0f) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)left_effort);
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)(-left_effort));
        }

        /* 6. Command Right Motor H-Bridge */
        if (right_effort >= 0.0f) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)right_effort);
        } else {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)(-right_effort));
        }
    }
}
```

---

## 10.2 Case Study 2: Persistent Robot Configuration & Calibration Storage

Combining the **MPU6050 Driver** (Chapter 6 of the baseline reference) with **Internal Flash EEPROM Emulation** (Chapter 2 of this guide).

### Operational Workflow
1. At power-on, firmware reads Flash Bank A.
2. If the Magic Word (`0x55AA55AA`) and CRC32 match: IMU calibration offsets and PID constants are loaded into active RAM.
3. If Flash is blank (or corrupted by power cut): firmware executes an automated bench calibration routine:
   - Robot sits stationary on a level surface for $2\text{ seconds}$.
   - Samples 500 IMU readings, calculates average gyro and accelerometer zero-g biases.
   - Saves new calibration struct to Flash with fresh CRC32 checksum.
   - Future boots load calibration instantly, eliminating startup delays!

---

## 10.3 Case Study 3: Robust Multitasking Controller with Watchdog Supervision

Demonstrating the **FreeRTOS Multi-Task Watchdog Supervisor Pattern** (Chapter 7.4) running on an STM32F405 robotic platform.

**Listing 10.2 — FreeRTOS task supervision implementation**

```c
#include "main.h"
#include "cmsis_os.h"

extern IWDG_HandleTypeDef hiwdg;

#define TASK_SENSOR_BIT   (1 << 0) /* 0x01 */
#define TASK_MOTOR_BIT    (1 << 1) /* 0x02 */
#define TASK_TELEMETRY_BIT (1 << 2) /* 0x04 */
#define ALL_TASKS_HEALTHY (TASK_SENSOR_BIT | TASK_MOTOR_BIT | TASK_TELEMETRY_BIT)

EventGroupHandle_t xWatchdogEventGroup;

/* Task 1: Sensor Task (50 Hz) */
void SensorTask(void *argument)
{
    for (;;) {
        App_ReadSensors();
        
        /* Report healthy completion */
        xEventGroupSetBits(xWatchdogEventGroup, TASK_SENSOR_BIT);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* Task 2: Motor PID Task (200 Hz) */
void MotorTask(void *argument)
{
    for (;;) {
        App_UpdateMotors();
        
        /* Report healthy completion */
        xEventGroupSetBits(xWatchdogEventGroup, TASK_MOTOR_BIT);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* Task 3: Telemetry Task (10 Hz) */
void TelemetryTask(void *argument)
{
    for (;;) {
        App_SendTelemetry();
        
        /* Report healthy completion */
        xEventGroupSetBits(xWatchdogEventGroup, TASK_TELEMETRY_BIT);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Supervisor Task: Runs every 250 ms (Watchdog hardware timeout is 500 ms) */
void WatchdogSupervisorTask(void *argument)
{
    for (;;) {
        /* Wait up to 300 ms for ALL tasks to report */
        EventBits_t bits = xEventGroupWaitBits(
            xWatchdogEventGroup,
            ALL_TASKS_HEALTHY,
            pdTRUE,  /* Clear bits on exit */
            pdTRUE,  /* Wait for ALL bits */
            pdMS_TO_TICKS(300)
        );

        if ((bits & ALL_TASKS_HEALTHY) == ALL_TASKS_HEALTHY) {
            /* All tasks alive and meeting timing! Pet the hardware watchdog */
            HAL_IWDG_Refresh(&hiwdg);
        } else {
            /* AT LEAST ONE TASK HAS DEADLOCKED OR CRASHED! */
            /* Do NOT refresh the watchdog. Hardware IWDG will reboot the robot in 200 ms */
        }
    }
}
```

---

## 10.4 Case Study 4: Battery-Powered Environmental Sensor Node

An autonomous environmental node measures temperature, humidity, and atmospheric pressure using an I²C sensor, transmits telemetry over UART to a radio module, and duty-cycles in deep **Stop Mode**.

### 10.4.1 Hardware Power Switching via High-Side P-MOSFET
Even in deep sleep, external sensors (such as an I²C IMU or GPS module) draw quiescent current ($1\text{ mA} - 15\text{ mA}$).
To eliminate this leakage:
- A logic-level **P-Channel MOSFET (e.g. AO3401)** is inserted between the $+3.3\text{ V}$ battery rail and the sensor $V_{DD}$ pin.
- When an STM32 GPIO (`PB0`) drives **LOW**, the P-FET turns ON, powering the sensor.
- When `PB0` drives **HIGH**, the P-FET turns OFF, cutting sensor power to **$0\text{ nA}$**!

```text
 +3.3V Battery ────[S] P-MOSFET (AO3401)
                   [G]──────┬───[ 10 kΩ Pullup to 3.3V ]
                   [D]      │
                    │      [PB0] GPIO Control Pin (Low = Power ON)
                    ▼
           Sensor VDD Rail (Powers I2C Sensor)
```

### 10.4.2 The Verified 10-Second Low-Power Operational Loop

**Listing 10.3 — Environmental node deep-sleep duty-cycling sequence**

```c
#include "main.h"

extern RTC_HandleTypeDef hrtc;
extern UART_HandleTypeDef huart1;

void Node_RunCycle(void)
{
    /* 1. Turn ON Sensor Power Rail via P-MOSFET */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    
    /* 2. Wait for sensor power-on reset settling (20 ms) */
    HAL_Delay(20);

    /* 3. Re-initialize I2C and read sensor registers */
    MX_I2C1_Init();
    float temp_c = Sensor_ReadTemperature();
    float pressure_hpa = Sensor_ReadPressure();
    
    /* 4. Transmit Telemetry over UART */
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "T=%.2f,P=%.1f\r\n", temp_c, pressure_hpa);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, 50);

    /* 5. De-initialize I2C and turn OFF Sensor Power Rail */
    HAL_I2C_DeInit(&hi2c1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); /* P-FET OFF */

    /* 6. Configure all non-essential GPIO pins to Analog to stop pin leakage */
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin  = GPIO_PIN_All & ~(GPIO_PIN_0); /* Keep PB0 high */
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 7. Set RTC Alarm to wake up in exactly 10 seconds */
    App_SetRTCAlarm_Seconds(10);

    /* 8. Enter Stop Mode (Consumes ~20 µA) */
    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* =========================================================================
       SYSTEM ASLEEP IN STOP MODE FOR 10 SECONDS...
       RTC ALARM FIRES -> HARDWARE WAKES UP!
       ========================================================================= */

    /* 9. CRITICAL: Restore high-speed PLL and HSE clock tree */
    SystemClock_Config();
    HAL_ResumeTick();
}
```

---

## 10.5 Case Study 5: Field Firmware Update Bootloader

A complete custom bootloader running on an STM32F103 checking for updates over `USART1` at $115{,}200\text{ baud}$.

```text
 Reset ──► Check Update Button (PB2 == LOW?) 
            ├──► YES: Enter Bootloader CLI Mode (Wait for XMODEM / UART binary)
            └──► NO:  Validate Metadata Header at 0x0800 4000
                       ├──► CRC32 Valid: JumpToApplication(0x0800 4400)
                       └──► Corrupted: Force Bootloader CLI Mode!
```
This architecture guarantees that even if a firmware update is cut off midway by a dead battery, the bootloader remains alive and allows the technician to re-flash without an ST-LINK.
