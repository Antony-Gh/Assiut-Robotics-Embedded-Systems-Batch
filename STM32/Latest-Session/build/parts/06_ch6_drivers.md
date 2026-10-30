# 6. Part 5 — Sensor Drivers and Reusable Embedded Software

In amateur embedded code, peripheral registers, I²C transactions, sensor-specific math, and application logic are frequently mixed inside `main.c`. While this approach may function for a one-off laboratory demonstration, it is fatal in commercial engineering and robotics. When the microcontroller family changes, when a second identical sensor is added, or when the underlying bus changes from I²C to SPI, tightly coupled code must be completely rewritten.

Professional embedded engineering treats sensor code as a **modular, reusable device driver**. A well-architected sensor driver forms an independent software component that isolates physical bus mechanics from business logic, enforces explicit error handling, and presents a clean, hardware-agnostic API in SI engineering units.

![Figure 6.1 — Four-layer sensor driver architecture, bus abstraction, and FreeRTOS producer-consumer task pipeline](diagrams/fig6_driver_and_rtos.png)
*Figure 6.1 — Software architecture: (A) Four-layer reusable driver hierarchy showing complete decoupling of hardware bus from application logic; (B) FreeRTOS dual-task telemetry architecture with thread-safe queue buffering and mutex protection.*

---

## 6.1 What is a Sensor Driver and Why Decouple It?

A **sensor driver** is a dedicated software abstraction layer responsible for managing the complete lifecycle of a physical sensor:
1. Validating hardware presence via identification registers (`WHO_AM_I`).
2. Configuring internal operational modes, sampling rates, full-scale ranges, and filters.
3. Executing bus transactions (I²C, SPI, or UART) to retrieve raw data registers.
4. Assembling raw binary words and converting them into calibrated, floating-point physical quantities in SI units ($m/s^2$, $rad/s$, $^\circ\text{C}$, $Pa$).
5. Detecting and reporting hardware bus faults, timeouts, and out-of-range sensor readings.

### The Dangers of Tightly Coupled Code
When developers write raw `HAL_I2C_Mem_Read()` calls directly inside their main application loop:
- **Zero Portability**: Migrating from an STM32F103 to an STM32G474 or STM32H7 requires rewriting all register accesses across the codebase.
- **Hidden Dependencies**: Application logic becomes infected with low-level details (such as whether an MPU6050 address requires a 1-bit left shift).
- **Inability to Test**: The control algorithms (e.g., drone attitude estimation or PID motor loops) cannot be tested or simulated on a PC without physical hardware connected.

---

## 6.2 The Four-Layer Driver Architecture

To achieve commercial-grade reusability, embedded software architects structure sensor drivers into four distinct, non-overlapping layers:

```text
┌─────────────────────────────────────────────────────────────┐
│ Layer 4: Application Layer                                  │
│ (Flight Controller, Motor PID, Telemetry Task, GUI)        │
└──────────────────────────────┬──────────────────────────────┘
                               │ Clean SI units (m/s², °C)
┌──────────────────────────────▼──────────────────────────────┐
│ Layer 3: Sensor Core Logic (mpu6050.c / .h)                 │
│ (Registers, 2's Complement Assembly, Sensitivity Scaling)   │
└──────────────────────────────┬──────────────────────────────┘
                               │ Abstract Read/Write (Addr, Reg, Len)
┌──────────────────────────────▼──────────────────────────────┐
│ Layer 2: Bus Abstraction Layer (HAL / LL / Function Pointers)│
│ (I2C1 Handle, SPI2 DMA, or Mock Unit Test Interface)        │
└──────────────────────────────┬──────────────────────────────┘
                               │ Electrical Waveforms / Pins
┌──────────────────────────────▼──────────────────────────────┐
│ Layer 1: Physical Hardware & Silicon Registers              │
│ (InvenSense MPU6050 IC, SDA/SCL Pull-ups, STM32 Silicon)    │
└─────────────────────────────────────────────────────────────┘
```

1. **Layer 1 (Physical Hardware)**: The silicon sensor IC, printed circuit board traces, external pull-up resistors, and microcontroller pins.
2. **Layer 2 (Bus Abstraction Layer)**: Adapts generic read/write calls to the specific communication hardware. On an STM32, this calls HAL or LL functions; on a desktop unit test runner, this calls simulated memory arrays.
3. **Layer 3 (Sensor Core Logic)**: Encapsulates the sensor's register map, state machine, initialization sequence, sensitivity constants, and calibration offsets. **This layer contains zero references to specific STM32 hardware registers or peripheral handles**.
4. **Layer 4 (Application Layer)**: Consumes clean, validated engineering units to execute control laws, robotic navigation, or user-interface displays.

---

## 6.3 The 10 Golden Rules of Embedded Driver Design

Every production sensor driver should adhere to these 10 architectural principles:

**Table 6.1 — The 10 golden rules of embedded driver architecture**

| Rule | Principle | Engineering Justification |
|---|---|---|
| **Rule 1** | **Never hardcode global peripheral handles** | Passing `&hi2c1` directly inside the driver prevents using `hi2c2` or instantiating two identical sensors on separate buses. |
| **Rule 2** | **Encapsulate all state in a handle struct** | A driver must be completely re-entrant. All configuration, calibration, and bus handles belong in an instance structure (`mpu6050_t`). |
| **Rule 3** | **Return explicit status codes everywhere** | Functions must return an enumerated status (`STATUS_OK`, `STATUS_ERR_TIMEOUT`, `STATUS_ERR_BUS`), never void or raw data directly. |
| **Rule 4** | **Enforce finite bus timeouts** | Never execute an unbounded `while` loop or pass `HAL_MAX_DELAY`. A disconnected sensor must not hang the operating system. |
| **Rule 5** | **Verify hardware identity (`WHO_AM_I`)** | Always confirm the physical chip responds with its expected ID before attempting to write configuration registers. |
| **Rule 6** | **Output calibrated SI engineering units** | Application code should consume `accel_mps2` and `gyro_dps`, never unscaled raw integers like `0x0E2A`. |
| **Rule 7** | **Assemble bytes portably and safely** | Use explicit fixed-width integers (`int16_t`, `uint8_t`) and signed bit-shifts to prevent undefined behavior across compilers. |
| **Rule 8** | **Separate init from continuous read** | Never execute sensor mode changes, clock selection, or DLPF configuration inside the periodic high-rate sampling loop. |
| **Rule 9** | **Zero UI or console logging inside driver** | Never call `printf()` or display routines from within a driver. Telemetry formatting belongs strictly in the application layer. |
| **Rule 10** | **Decouple via Bus Interface Callbacks** | Abstract physical I²C/SPI operations behind function pointers to enable automated hardware-in-the-loop (HIL) testing and PC mocking. |

---

## 6.4 Professional Driver Implementation: InvenSense MPU6050

The following complete, production-grade driver demonstrates these architectural principles. It is family-agnostic, multi-instance capable, unit-testable, and provides robust error handling.

### 6.4.1 Public Header File: `mpu6050.h`

**Listing 6.1 — Production header file `mpu6050.h`**

```c
#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Typed error codes */
typedef enum {
    MPU6050_OK              = 0,
    MPU6050_ERR_PARAM       = -1,
    MPU6050_ERR_BUS         = -2,
    MPU6050_ERR_TIMEOUT     = -3,
    MPU6050_ERR_NOT_FOUND   = -4
} mpu6050_status_t;

/* Accelerometer Full Scale Range */
typedef enum {
    MPU6050_ACCEL_2G        = 0, /* ±2g  (16384 LSB/g) */
    MPU6050_ACCEL_4G        = 1, /* ±4g  (8192 LSB/g)  */
    MPU6050_ACCEL_8G        = 2, /* ±8g  (4096 LSB/g)  */
    MPU6050_ACCEL_16G       = 3  /* ±16g (2048 LSB/g)  */
} mpu6050_accel_fs_t;

/* Gyroscope Full Scale Range */
typedef enum {
    MPU6050_GYRO_250DPS     = 0, /* ±250 °/s  (131 LSB/°/s)  */
    MPU6050_GYRO_500DPS     = 1, /* ±500 °/s  (65.5 LSB/°/s) */
    MPU6050_GYRO_1000DPS    = 2, /* ±1000 °/s (32.8 LSB/°/s) */
    MPU6050_GYRO_2000DPS    = 3  /* ±2000 °/s (16.4 LSB/°/s) */
} mpu6050_gyro_fs_t;

/* Engineering Units Data Structure */
typedef struct {
    float accel_x_mps2; /* X acceleration in m/s² */
    float accel_y_mps2; /* Y acceleration in m/s² */
    float accel_z_mps2; /* Z acceleration in m/s² */
    float gyro_x_dps;   /* X angular rate in degrees/sec */
    float gyro_y_dps;   /* Y angular rate in degrees/sec */
    float gyro_z_dps;   /* Z angular rate in degrees/sec */
    float temp_c;       /* Die temperature in °C */
} mpu6050_data_t;

/* Abstract Bus Interface Function Pointers */
typedef int8_t (*mpu6050_write_fn)(void *intf_ptr, uint8_t dev_addr, 
                                  uint8_t reg_addr, const uint8_t *data, uint16_t len);
typedef int8_t (*mpu6050_read_fn)(void *intf_ptr, uint8_t dev_addr, 
                                 uint8_t reg_addr, uint8_t *data, uint16_t len);
typedef void (*mpu6050_delay_fn)(uint32_t ms);

/* Driver Handle Structure (Encapsulates all instance state) */
typedef struct {
    void               *intf_ptr;        /* Opaque pointer to HAL handle (e.g. &hi2c1) */
    uint8_t             dev_addr;        /* 7-bit I2C address (0x68 or 0x69) */
    mpu6050_write_fn    write_reg;       /* Low-level bus write pointer */
    mpu6050_read_fn     read_reg;        /* Low-level bus read pointer */
    mpu6050_delay_fn    delay_ms;        /* Delay hook */
    float               accel_scale;     /* Scale factor to convert LSB to m/s² */
    float               gyro_scale;      /* Scale factor to convert LSB to deg/s */
    float               accel_bias[3];   /* X, Y, Z zero-g offset calibration */
    float               gyro_bias[3];    /* X, Y, Z zero-rate offset calibration */
    bool                is_initialized;  /* State guard */
} mpu6050_t;

/* Public API */
mpu6050_status_t MPU6050_Init(mpu6050_t *dev, void *intf_ptr, uint8_t dev_addr,
                             mpu6050_write_fn write_fn, mpu6050_read_fn read_fn,
                             mpu6050_delay_fn delay_fn);
mpu6050_status_t MPU6050_ConfigRanges(mpu6050_t *dev, mpu6050_accel_fs_t accel_fs, 
                                     mpu6050_gyro_fs_t gyro_fs);
mpu6050_status_t MPU6050_CalibrateOffsets(mpu6050_t *dev, uint16_t sample_count);
mpu6050_status_t MPU6050_ReadSensors(mpu6050_t *dev, mpu6050_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H */
```

---

### 6.4.2 Driver Implementation: `mpu6050.c`

**Listing 6.2 — Production source file `mpu6050.c`**

```c
#include "mpu6050.h"

/* Internal MPU6050 Register Map */
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_ACCEL_XOUT_H    0x3B
#define REG_TEMP_OUT_H      0x41
#define REG_GYRO_XOUT_H     0x43
#define REG_PWR_MGMT_1      0x6B
#define REG_WHO_AM_I        0x75

#define EXPECTED_WHO_AM_I   0x68
#define GRAVITY_MPS2        9.80665f

mpu6050_status_t MPU6050_Init(mpu6050_t *dev, void *intf_ptr, uint8_t dev_addr,
                             mpu6050_write_fn write_fn, mpu6050_read_fn read_fn,
                             mpu6050_delay_fn delay_fn)
{
    uint8_t who_am_i = 0;

    if (!dev || !write_fn || !read_fn || !delay_fn) {
        return MPU6050_ERR_PARAM;
    }

    dev->intf_ptr       = intf_ptr;
    dev->dev_addr       = dev_addr;
    dev->write_reg      = write_fn;
    dev->read_reg       = read_fn;
    dev->delay_ms       = delay_fn;
    dev->accel_scale    = GRAVITY_MPS2 / 16384.0f; /* Default ±2g */
    dev->gyro_scale     = 1.0f / 131.0f;           /* Default ±250 °/s */
    dev->is_initialized = false;

    for (int i = 0; i < 3; i++) {
        dev->accel_bias[i] = 0.0f;
        dev->gyro_bias[i]  = 0.0f;
    }

    /* 1. Wait for sensor internal power-on reset */
    dev->delay_ms(50);

    /* 2. Validate device presence */
    if (dev->read_reg(dev->intf_ptr, dev->dev_addr, REG_WHO_AM_I, &who_am_i, 1) != 0) {
        return MPU6050_ERR_BUS;
    }
    if (who_am_i != EXPECTED_WHO_AM_I) {
        return MPU6050_ERR_NOT_FOUND;
    }

    /* 3. Wake up device: Reset sleep bit, select Auto-PLL clock */
    uint8_t pwr_mgmt = 0x01; /* Clock Source = PLL with X-axis Gyroscope */
    if (dev->write_reg(dev->intf_ptr, dev->dev_addr, REG_PWR_MGMT_1, &pwr_mgmt, 1) != 0) {
        return MPU6050_ERR_BUS;
    }
    dev->delay_ms(10);

    /* 4. Configure default ranges (±2g, ±250 deg/s) */
    mpu6050_status_t status = MPU6050_ConfigRanges(dev, MPU6050_ACCEL_2G, MPU6050_GYRO_250DPS);
    if (status != MPU6050_OK) {
        return status;
    }

    dev->is_initialized = true;
    return MPU6050_OK;
}

mpu6050_status_t MPU6050_ConfigRanges(mpu6050_t *dev, mpu6050_accel_fs_t accel_fs, 
                                     mpu6050_gyro_fs_t gyro_fs)
{
    uint8_t reg_val;

    if (!dev) return MPU6050_ERR_PARAM;

    /* Configure Accel Full-Scale */
    reg_val = (uint8_t)(accel_fs << 3);
    if (dev->write_reg(dev->intf_ptr, dev->dev_addr, REG_ACCEL_CONFIG, &reg_val, 1) != 0) {
        return MPU6050_ERR_BUS;
    }

    switch (accel_fs) {
        case MPU6050_ACCEL_2G:  dev->accel_scale = GRAVITY_MPS2 / 16384.0f; break;
        case MPU6050_ACCEL_4G:  dev->accel_scale = GRAVITY_MPS2 / 8192.0f;  break;
        case MPU6050_ACCEL_8G:  dev->accel_scale = GRAVITY_MPS2 / 4096.0f;  break;
        case MPU6050_ACCEL_16G: dev->accel_scale = GRAVITY_MPS2 / 2048.0f;  break;
    }

    /* Configure Gyro Full-Scale */
    reg_val = (uint8_t)(gyro_fs << 3);
    if (dev->write_reg(dev->intf_ptr, dev->dev_addr, REG_GYRO_CONFIG, &reg_val, 1) != 0) {
        return MPU6050_ERR_BUS;
    }

    switch (gyro_fs) {
        case MPU6050_GYRO_250DPS:  dev->gyro_scale = 1.0f / 131.0f; break;
        case MPU6050_GYRO_500DPS:  dev->gyro_scale = 1.0f / 65.5f;  break;
        case MPU6050_GYRO_1000DPS: dev->gyro_scale = 1.0f / 32.8f;  break;
        case MPU6050_GYRO_2000DPS: dev->gyro_scale = 1.0f / 16.4f;  break;
    }

    return MPU6050_OK;
}

mpu6050_status_t MPU6050_ReadSensors(mpu6050_t *dev, mpu6050_data_t *data)
{
    uint8_t raw_buf[14];

    if (!dev || !data || !dev->is_initialized) {
        return MPU6050_ERR_PARAM;
    }

    /* Single continuous 14-byte burst read across all sensor registers */
    if (dev->read_reg(dev->intf_ptr, dev->dev_addr, REG_ACCEL_XOUT_H, raw_buf, 14) != 0) {
        return MPU6050_ERR_BUS;
    }

    /* Assemble signed 16-bit integers */
    int16_t raw_ax = (int16_t)(((uint16_t)raw_buf[0]  << 8) | raw_buf[1]);
    int16_t raw_ay = (int16_t)(((uint16_t)raw_buf[2]  << 8) | raw_buf[3]);
    int16_t raw_az = (int16_t)(((uint16_t)raw_buf[4]  << 8) | raw_buf[5]);
    int16_t raw_t  = (int16_t)(((uint16_t)raw_buf[6]  << 8) | raw_buf[7]);
    int16_t raw_gx = (int16_t)(((uint16_t)raw_buf[8]  << 8) | raw_buf[9]);
    int16_t raw_gy = (int16_t)(((uint16_t)raw_buf[10] << 8) | raw_buf[11]);
    int16_t raw_gz = (int16_t)(((uint16_t)raw_buf[12] << 8) | raw_buf[13]);

    /* Convert to physical SI units and subtract calibrated zero-bias */
    data->accel_x_mps2 = ((float)raw_ax * dev->accel_scale) - dev->accel_bias[0];
    data->accel_y_mps2 = ((float)raw_ay * dev->accel_scale) - dev->accel_bias[1];
    data->accel_z_mps2 = ((float)raw_az * dev->accel_scale) - dev->accel_bias[2];

    data->temp_c       = ((float)raw_t / 340.0f) + 36.53f;

    data->gyro_x_dps   = ((float)raw_gx * dev->gyro_scale) - dev->gyro_bias[0];
    data->gyro_y_dps   = ((float)raw_gy * dev->gyro_scale) - dev->gyro_bias[1];
    data->gyro_z_dps   = ((float)raw_gz * dev->gyro_scale) - dev->gyro_bias[2];

    return MPU6050_OK;
}
```

---

### 6.4.3 Application Usage and HAL Binding

Notice how cleanly the driver binds to STM32 HAL inside `main.c` without contaminating the driver source with STM32 headers.

**Listing 6.3 — Application integration in `main.c`**

```c
#include "main.h"
#include "mpu6050.h"

extern I2C_HandleTypeDef hi2c1;
mpu6050_t imu;

/* HAL I2C Adapter Callbacks */
static int8_t STM32_I2C_Write(void *intf_ptr, uint8_t dev_addr, uint8_t reg_addr, 
                             const uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    /* Pass pre-shifted HAL address: (dev_addr << 1) */
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(hi2c, (uint16_t)(dev_addr << 1), reg_addr,
                                            I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, 100);
    return (st == HAL_OK) ? 0 : -1;
}

static int8_t STM32_I2C_Read(void *intf_ptr, uint8_t dev_addr, uint8_t reg_addr, 
                            uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)intf_ptr;
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(hi2c, (uint16_t)(dev_addr << 1), reg_addr,
                                           I2C_MEMADD_SIZE_8BIT, data, len, 100);
    return (st == HAL_OK) ? 0 : -1;
}

void App_Init(void)
{
    mpu6050_status_t status = MPU6050_Init(&imu, &hi2c1, 0x68,
                                          STM32_I2C_Write, STM32_I2C_Read, HAL_Delay);
    if (status != MPU6050_OK) {
        /* Error handling: Illuminate red fault LED and log error code */
        Error_Handler();
    }
}

void App_Loop(void)
{
    mpu6050_data_t sample;
    if (MPU6050_ReadSensors(&imu, &sample) == MPU6050_OK) {
        /* Consume clean SI units */
        ProcessAttitude(sample.accel_x_mps2, sample.accel_y_mps2, sample.accel_z_mps2,
                        sample.gyro_x_dps, sample.gyro_y_dps, sample.gyro_z_dps);
    }
    HAL_Delay(10); /* 100 Hz sampling */
}
```

---

## 6.5 Automated Unit Testing and Desktop Mocking

Because `mpu6050.c` relies exclusively on standard C types and function pointers, **it can be compiled and verified on a desktop computer (Linux/macOS/Windows) with GCC without an STM32 board**.

**Listing 6.4 — Desktop PC unit test mock runner**

```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "mpu6050.h"

/* Mock sensor register memory bank */
static uint8_t mock_registers[256];

static int8_t Mock_I2C_Write(void *intf_ptr, uint8_t dev_addr, uint8_t reg_addr, 
                            const uint8_t *data, uint16_t len)
{
    memcpy(&mock_registers[reg_addr], data, len);
    return 0;
}

static int8_t Mock_I2C_Read(void *intf_ptr, uint8_t dev_addr, uint8_t reg_addr, 
                           uint8_t *data, uint16_t len)
{
    memcpy(data, &mock_registers[reg_addr], len);
    return 0;
}

static void Mock_Delay(uint32_t ms) {}

int main(void)
{
    mpu6050_t dev;
    mpu6050_data_t data;

    /* Test Case 1: Unpopulated device should report NOT_FOUND */
    mock_registers[0x75] = 0x00; /* Wrong WHO_AM_I */
    assert(MPU6050_Init(&dev, NULL, 0x68, Mock_I2C_Write, Mock_I2C_Read, Mock_Delay) 
           == MPU6050_ERR_NOT_FOUND);

    /* Test Case 2: Valid device initialization */
    mock_registers[0x75] = 0x68; /* Correct WHO_AM_I */
    assert(MPU6050_Init(&dev, NULL, 0x68, Mock_I2C_Write, Mock_I2C_Read, Mock_Delay) 
           == MPU6050_OK);

    /* Test Case 3: Verify 1.0g on Z-axis converts accurately */
    /* 1.0g in ±2g mode = +16384 LSB = 0x4000 */
    mock_registers[0x3F] = 0x40; /* ACCEL_ZOUT_H */
    mock_registers[0x40] = 0x00; /* ACCEL_ZOUT_L */

    assert(MPU6050_ReadSensors(&dev, &data) == MPU6050_OK);
    /* Expect 9.80665 m/s² within floating point tolerance */
    assert(data.accel_z_mps2 > 9.80f && data.accel_z_mps2 < 9.82f);

    printf("ALL DRIVER UNIT TESTS PASSED SUCCESSFULLY.\n");
    return 0;
}
```
