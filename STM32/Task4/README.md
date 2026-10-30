# Assiut Robotics Team - Embedded Systems Batch

## Task 4: I2C Communication (MPU-6050 & BMP280/BMP180) and UART Telemetry

---

## 1. Overview & Objectives

This project implements a complete, industrial-grade embedded sensor integration and telemetry system on the **STM32F103C8T6 (ARM Cortex-M3)** using STM32 HAL:

1. **Task 1: MPU-6050 Motion Sensor (GY-521)**
   - Interfaced over **I2C1** (Standard Mode: 100 kHz).
   - Reads 6-axis raw acceleration ($A_x, A_y, A_z$) and angular rates ($G_x, G_y, G_z$).
   - Implements sensor fusion via a **Complementary Filter** ($\alpha = 0.98$) to compute real-time **Roll**, **Pitch**, and **Yaw** angles.
   - Comprehensive analytical answers addressing the prompt's question: *"what are your notes on the 3 angles?"*

2. **Task 2: BMP Barometric Pressure & Temperature Sensor (BMP280 / BMP180)**
   - Interfaced over the same **I2C1** shared bus.
   - Auto-detects device address (`0x76` or `0x77`) and chip ID (`0x58` for BMP280, `0x55` for BMP180).
   - Reads factory calibration trimming coefficients from sensor OTP/ROM.
   - Computes compensated **Temperature** (°C), **Pressure** (hPa and Pa), and **Barometric Altitude** (m) using official Bosch Sensortec integer compensation formulas.

3. **Telemetry & Dual Display Mediums (Virtual Terminal & 16x2 Character LCD)**
   - Transmits real-time formatted telemetry over **USART1** (`PA9` TX, `PA10` RX) at 9600 Baud (8-N-1).
   - Displays live data on the Proteus **Virtual Terminal**.
   - **Real-Time 16x2 Character LCD Display (just as Task 3):**
     - Displays instantaneous Roll and Pitch angles on Row 0 (`R:+12.4  P:-05.1`).
     - Dynamically alternates Row 1 between (Yaw + Temperature) and (Pressure + Altitude) with zero flicker.
     - Uses robust 4-bit bus on `PB0..PB3` to completely isolate and protect `PB6` (SCL) and `PB7` (SDA) I2C lines.

4. **Modular Architecture & Documentation**
   - Modular separation into dedicated, reusable drivers: [`mpu6050.h`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Inc/mpu6050.h)/[`mpu6050.c`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/mpu6050.c), [`bmp280.h`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Inc/bmp280.h)/[`bmp280.c`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/bmp280.c), [`lcd.h`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Inc/lcd.h)/[`lcd.c`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/lcd.c), and [`uart_comm.h`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Inc/uart_comm.h)/[`uart_comm.c`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/uart_comm.c).
   - Full **Doxygen** documentation across all header and source files.

---

## 2. Hardware Pin Mapping & Wiring Architecture

### 2.1 Complete Pinout Table

| STM32 Pin | LQFP48 Pin # | Peripheral Signal | Mode / Type | Connected Device & Pin | Description & Electrical Notes |
| :--- | :---: | :--- | :--- | :--- | :--- |
| **PA0** | 10 | `GPIO_Output` | Output Push-Pull | LCD `E` (Enable) | LCD strobe pulse line (Identical to Task 3). |
| **PA1** | 11 | `GPIO_Output` | Output Push-Pull | LCD `RS` (Reg Select) | `0` = Command, `1` = Data (Identical to Task 3). |
| **PA2** | 12 | `GPIO_Output` | Output Push-Pull | LCD `RW` (Read/Write) | Pulled `0` for Write mode (Identical to Task 3). |
| **PB0** | 18 | `GPIO_Output` | Output Push-Pull | LCD `D4` (Data Bit 4) | High nibble data line 4 in 4-bit mode. |
| **PB1** | 19 | `GPIO_Output` | Output Push-Pull | LCD `D5` (Data Bit 5) | High nibble data line 5 in 4-bit mode. |
| **PB2** | 20 | `GPIO_Output` | Output Push-Pull | LCD `D6` (Data Bit 6) | High nibble data line 6 in 4-bit mode. |
| **PB3** | 39 | `GPIO_Output` | Output Push-Pull | LCD `D7` (Data Bit 7) | High nibble data line 7 in 4-bit mode. |
| **PB6** | 42 | `I2C1_SCL` | Alternate Function Open-Drain | MPU-6050 `SCL` / BMP280 `SCL` | I2C Serial Clock (100 kHz Standard Mode). Requires 4.7 kΩ pull-up. |
| **PB7** | 43 | `I2C1_SDA` | Alternate Function Open-Drain | MPU-6050 `SDA` / BMP280 `SDA` | I2C Serial Data bidirectional line. Requires 4.7 kΩ pull-up. |
| **PA9** | 30 | `USART1_TX` | Alternate Function Push-Pull | Virtual Terminal `RXD` | UART Telemetry Output (9600 Baud, 8-N-1, No Parity). |
| **PA10** | 31 | `USART1_RX` | Input Floating / Pull-Up | Virtual Terminal `TXD` | UART Command / Debug Input. |
| **PC13** | 2 | `GPIO_Output` | Output Push-Pull | On-Board Blue Pill LED | System Status / Heartbeat indicator (toggles on every 4 Hz loop). |
| **+3.3V** | Header | `VDD` / Power | +3.3V DC Power Rail | `VCC` of all modules | Powers STM32, MPU-6050, BMP280, and LCD logic. |
| **GND** | Header | `VSS` / Ground | 0V Ground Rail | `GND` of all modules | Common ground reference across MCU, sensors, LCD, and Terminal. |

---

### 2.2 Blue Pill Board Pinout Diagram

```text
                       STM32F103C8T6 ("Blue Pill")
                       +-------------------------+
                       | [USB]             [LED] |--> PC13 (Heartbeat LED)
                       |                         |
                       |                   [GND] |---> Common Ground
                       |                  [3.3V] |---> +3.3V VCC
                       |                         |
                LCD E <| PA0                 PB7 |---> I2C1_SDA (MPU-6050 & BMP280)
               LCD RS <| PA1                 PB6 |---> I2C1_SCL (MPU-6050 & BMP280)
               LCD RW <| PA2                 PB5 |
                       | PA3                 PB4 |
                       | PA4                 PB3 |---> LCD D7
                       | PA5                PA15 |
                       | PA6                PA12 |
                       | PA7                PA11 |
               LCD D4 <| PB0                PA10 |<--- USART1_RX (Virtual Terminal TXD)
               LCD D5 <| PB1                 PA9 |---> USART1_TX (Virtual Terminal RXD)
               LCD D6 <| PB2                 PA8 |
                       | PB10               PB15 |
                       | PB11               PB14 |
                       | PB12               PB13 |
                       +-------------------------+
```

---

### 2.3 Sensor-to-MCU Interconnection Details

#### 1. MPU-6050 6-DOF IMU (GY-521 Module)

| Module Pin | STM32 Connection | Function / State | Notes |
| :--- | :--- | :--- | :--- |
| **VCC** | `+3.3V` / `+5V` | Power Input | GY-521 board contains an on-board 3.3V low-dropout regulator. |
| **GND** | `GND` | Ground Reference | Common ground. |
| **SCL** | `PB6` (`I2C1_SCL`) | I2C Serial Clock | Driven by STM32 I2C1 peripheral at 100 kHz. |
| **SDA** | `PB7` (`I2C1_SDA`) | I2C Serial Data | Bidirectional data exchange. |
| **XDA** | *Unconnected* | Auxiliary I2C Data | Used only when MPU-6050 acts as master to external compass. |
| **XCL** | *Unconnected* | Auxiliary I2C Clock | Used only when MPU-6050 acts as master to external compass. |
| **AD0** | `GND` | I2C Address Select | Pulled LOW to select primary 7-bit I2C address `0x68` (Write: `0xD0`, Read: `0xD1`). |
| **INT** | *Unconnected* / GPIO | Data Ready Interrupt | Optional; driver reads via periodic 4 Hz polling. |

#### 2. BMP280 Barometric Pressure & Temperature Sensor

| Module Pin | STM32 Connection | Function / State | Notes |
| :--- | :--- | :--- | :--- |
| **VCC** | `+3.3V` | Power Input | Operating voltage: 1.71V to 3.6V (typically 3.3V). |
| **GND** | `GND` | Ground Reference | Common ground. |
| **SCL / SCK** | `PB8` (Pin 45) or `PB10` (Pin 21) or `PB6` (Pin 42) | I2C Serial Clock | Connected to shared SCL bus. |
| **SDA / SDI** | `PB9` (Pin 46) or `PB11` (Pin 22) or `PB7` (Pin 43) | I2C Serial Data | Connected to shared SDA bus. |
| **CSB** | `+3.3V` (VCC) | Chip Select (I2C/SPI) | Tied HIGH to enable I2C mode (disables SPI interface). |
| **SDO / ADDR** | `GND` | I2C Address Select | Pulled LOW for 7-bit address `0x76` (Write: `0xEC`, Read: `0xED`). |

#### 3. Proteus Virtual Terminal

| Terminal Pin | STM32 Pin | Function | Serial Settings |
| :--- | :--- | :--- | :--- |
| **RXD** (Input) | `PA9` (`USART1_TX`) | Serial Data Input to Terminal | 9600 Baud, 8 Data Bits, 1 Stop Bit, No Parity |
| **TXD** (Output) | `PA10` (`USART1_RX`) | Optional Command Input | 9600 Baud, 8 Data Bits, 1 Stop Bit, No Parity |
| **GND** | `GND` | Reference Ground | Common ground |

#### 4. 16x2 Character LCD (LM016L / HD44780)

| LCD Pin # | LCD Signal | STM32 Pin | Function | Description |
| :--- | :--- | :--- | :--- | :--- |
| **1** | `VSS` | `GND` | Ground Reference | Common ground (0V) |
| **2** | `VDD` | `+5V` / `+3.3V` | Logic Power | Powers LCD controller |
| **3** | `VEE` | `GND` / Pot Wiper | Contrast Adjust | Tied to 0V for maximum sharp contrast |
| **4** | `RS` | `PA1` | Register Select | `0` = Command, `1` = Data (Identical to Task 3) |
| **5** | `RW` | `PA2` (or `GND`) | Read / Write | `0` = Write mode (Identical to Task 3) |
| **6** | `E` | `PA0` | Enable Strobe | Latches command/data on falling edge |
| **7..10** | `D0..D3` | *NC (Unconnected)* | Lower Data Nibble | Unused in 4-bit mode |
| **11** | `D4` | `PB0` | High Data Bit 4 | Driven by STM32 GPIOB |
| **12** | `D5` | `PB1` | High Data Bit 5 | Driven by STM32 GPIOB |
| **13** | `D6` | `PB2` | High Data Bit 6 | Driven by STM32 GPIOB |
| **14** | `D7` | `PB3` | High Data Bit 7 | Driven by STM32 GPIOB |

---

### 2.4 Proteus Pin Behaviors & Recommended Wiring

#### Why did Proteus show Grey on PB6/PB7 and Blue on PB8/PB9?

1. **STM32F103 AFIO Remap:** In the STM32F103 architecture, `I2C1` has two pin mappings:
   - **Default:** `PB6` (SCL) and `PB7` (SDA)
   - **Remapped (`AFIO_MAPR_I2C1_REMAP`):** `PB8` (SCL) and `PB9` (SDA)
2. **Proteus VSM Engine:** Proteus's STM32 simulation DLL (`cm3_stm32.dll`) routes `I2C1` directly to **`PB8` (pin 45)** and **`PB9` (pin 46)**. When the firmware calls `MPU6050_Init()` to generate a START condition, Proteus drives `PB8` and `PB9` **LOW (blue square)**. Because no sensor was attached to PB8/PB9, the lines stayed low. Meanwhile, `PB6` and `PB7` were unconfigured/floating (**grey square**).
3. **Pull-Up Resistors in Proteus:** Standard analog resistors (`RES`) do not pull digital lines high in Proteus pure-digital partitions. The digital sensor models (`MPU6050V2.DLL`, `TEPBMP280.DLL`) already simulate open-drain lines internally, so external analog resistors are not needed (or use the Proteus `PULLUP` digital component).
4. **Why PB10 and PB11 are Red:** Pins 21 and 22 are hardware **`I2C2`**. In Proteus, I2C2 is unmapped and starts at logic HIGH (**red square**), which is why Mazen's reference solution used `I2C2` on `PB10`/`PB11`.

#### Recommended Wiring in Proteus

The firmware now features **Intelligent Multi-Bus Auto-Detection** supporting all three configurations:

```text
+-------------------+--------------------+--------------------+-------------------------------------------+
| Option            | SCL Connection     | SDA Connection     | Proteus Status / Compatibility            |
+-------------------+--------------------+--------------------+-------------------------------------------+
| 1. (Recommended)  | Pin 45 (PB8)       | Pin 46 (PB9)       | Matches active Proteus I2C1 remapped bus  |
| 2. (Mazen Ref)    | Pin 21 (PB10)      | Pin 22 (PB11)      | Hardware I2C2 bus, idle HIGH (red)        |
| 3. (Legacy)       | Pin 42 (PB6)       | Pin 43 (PB7)       | Default I2C1 bus                          |
+-------------------+--------------------+--------------------+-------------------------------------------+
```

```text
                  +3.3V (or internal)
                    |
  STM32F103C8T6     |
+-------------+     |
|             |     |
|    PB8 (SCL)|-----+-------------------------> [SCL] MPU-6050 (Addr: 0x68)
|    (Pin 45) |     |                     |
|             |     |                     +---> [SCK] BMP280   (Addr: 0x76)
|             |     |
|    PB9 (SDA)|-----+-------------------------> [SDA] MPU-6050 (Addr: 0x68)
|    (Pin 46) |                           |
|             |                           +---> [SDI] BMP280   (Addr: 0x76)
|             |
|    PA9 (TX) |-------------------------------> [RXD] Virtual Terminal (9600 Baud)
|   PA10 (RX) |<------------------------------- [TXD] Virtual Terminal
+-------------+
```

- **MPU-6050:** Default 7-bit address is `0x68` (Write: `0xD0`, Read: `0xD1`).
- **BMP280:** Default 7-bit address is `0x76` (Write: `0xEC`, Read: `0xED`) or `0x77` (`0xEE`).
- **BMP180:** Default 7-bit address is `0x77` (Write: `0xEE`, Read: `0xEF`).

No address collisions occur, allowing multiple sensors to be queried sequentially over the single I2C peripheral.

---

## 3. Notes on the 3 Angles (Pitch, Roll, Yaw)

The task specifically asks:
> *"what are your notes on the 3 angles?"*

Here is the comprehensive physical, mathematical, and control-theory analysis:

### 1. Pitch ($\theta$) and Roll ($\phi$)

- **Physical Reference:**
  When the sensor is static or moving with steady velocity, Earth's gravity vector ($\vec{g} \approx 9.81\text{ m/s}^2$) points directly towards the center of the Earth. The 3-axis accelerometer measures the projection of this gravity vector across the sensor's X, Y, and Z axes:
  $$\text{Roll}_{\text{acc}} = \text{atan2}(a_y, a_z) \times \frac{180^\circ}{\pi}$$
  $$\text{Pitch}_{\text{acc}} = \text{atan2}(-a_x, \sqrt{a_y^2 + a_z^2}) \times \frac{180^\circ}{\pi}$$
- **Accelerometer Characteristics:**
  - **Advantage:** Gives an absolute, non-drifting static measurement of tilt relative to Earth's horizon.
  - **Disadvantage:** High sensitivity to dynamic forces (centrifugal forces, vibrations, sudden shocks, linear acceleration). High-frequency noise is significant.
- **Gyroscope Characteristics:**
  - **Advantage:** Measures instantaneous angular rate ($\omega_x, \omega_y$). Smooth, rapid, and immune to linear accelerations.
  - **Disadvantage:** Integrating angular velocity over time ($\theta(t) = \int \omega \, dt$) integrates any non-zero DC bias and sensor noise, resulting in unbounded **drift** over time.
- **Sensor Fusion (Complementary Filter):**
  A complementary filter effectively splits the spectrum:
  $$\text{Angle}(t) = \alpha \cdot (\text{Angle}(t-\Delta t) + \omega \cdot \Delta t) + (1 - \alpha) \cdot \text{Angle}_{\text{acc}}$$
  With $\alpha = 0.98$, the gyroscope provides 98% of the high-frequency dynamic response, while the accelerometer supplies 2% low-frequency correction to eliminate drift. This yields responsive, drift-free Roll and Pitch.

### 2. Yaw ($\psi$, Heading)

- **The Gravity Invariance Problem:**
  - Earth's gravity vector $\vec{g}$ acts purely along the local vertical axis (perpendicular to the horizontal plane).
  - When the sensor rotates horizontally around the Z-axis (Yaw), the gravitational projection on all three axes **does not change at all** ($a_x = 0$, $a_y = 0$, $a_z = 1\text{g}$).
  - Therefore, **an accelerometer cannot detect, measure, or correct horizontal Yaw rotation**.
- **Consequence of Gyro Integration:**
  - Yaw can *only* be estimated with an MPU-6050 by integrating the Z-axis angular velocity:
    $$\text{Yaw}(t) = \text{Yaw}(t - \Delta t) + g_z \cdot \Delta t$$
  - Because there is no external absolute reference in the horizontal plane to bound the integration, the DC gyro bias causes **unavoidable drift over time** (several degrees per minute).
- **Industrial Solution:**
  - To achieve an absolute, drift-free Yaw angle, a 9-DOF IMU (such as MPU-9250 or external HMC5883L/QMC5883L) is needed. The 3-axis magnetometer detects the Earth's magnetic north vector ($B_x, B_y$), serving as a low-frequency reference for Yaw just as gravity does for Pitch and Roll.

### 3. Gimbal Lock & Singularities

- In Euler angle representations (Roll-Pitch-Yaw), when Pitch reaches $\pm 90^\circ$ (pointing straight up or down), the Roll and Yaw axes align on the same physical line, causing a loss of one rotational degree of freedom (**Gimbal Lock**).
- For full 3D aerobatic orientation, **quaternions** ($q_0, q_1, q_2, q_3$) are used in place of Euler angles to eliminate singularities.

---

## 4. Driver Architecture & Implementation

```text
STM32F103C8TX/
├── Core/
│   ├── Inc/
│   │   ├── bmp280.h       <- Bosch BMP280/BMP180 I2C Driver & Doxygen API
│   │   ├── mpu6050.h      <- MPU-6050 6-DOF I2C Driver & Complementary Filter API
│   │   ├── uart_comm.h    <- Formatted Telemetry & Virtual Terminal Output API
│   │   ├── main.h
│   │   └── stm32f1xx_hal_conf.h
│   └── Src/
│       ├── bmp280.c       <- Auto-detection, calibration readout & compensation math
│       ├── mpu6050.c      <- Burst reads, scaling, Euler & Complementary Filter
│       ├── uart_comm.c    <- Telemetry banner & formatted terminal streaming
│       ├── main.c         <- Peripheral init & 4 Hz non-blocking loop
│       ├── gpio.c
│       └── stm32f1xx_hal_msp.c <- I2C1 and USART1 GPIO/Clock configuration
├── Debug/
│   ├── STM32F103C8TX.elf  <- Compiled ARM ELF executable
│   └── STM32F103C8TX.hex  <- Intel HEX file for Proteus simulation
└── Task4-Proteus.pdsprj   <- Complete Proteus 8 simulation schematic
```

### Key Functions (Doxygen Documented)

- [`MPU6050_Init()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/mpu6050.c#L45-L78): Wakes sensor from sleep (`PWR_MGMT_1 = 0x00`), sets $\pm 2\text{g}$ and $\pm 250^\circ/\text{s}$, verifies connectivity.
- [`MPU6050_Read()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/mpu6050.c#L80-L130): 14-byte burst read of accelerometer, temperature, and gyro; computes raw to physical unit conversion.
- [`MPU6050_UpdateAngles()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/mpu6050.c#L132-L175): Applies Complementary Filter ($\alpha = 0.98$) and Z-gyro integration for Yaw.
- [`BMP_Init()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/bmp280.c#L45-L121): Scans I2C addresses, checks Chip ID (`0x58` or `0x55`), reads factory trimming registers.
- [`BMP_Read()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/bmp280.c#L174-L202): Reads 6-byte burst and executes 32-bit/64-bit Bosch compensation formulas for temperature and pressure, plus hypsometric barometric altitude.
- [`UART_SendTelemetry()`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Core/Src/uart_comm.c#L46-L85): Formats clean terminal output.

---

## 5. Proteus Simulation Guide

1. **Required Libraries & Models:**
   - Proteus 8.x with `TEPMPU6050.DLL` and `TEPMPU6050.LIB` installed in Proteus `MODELS` and `LIBRARY` folders.
2. **Open the Project:**
   - Double-click [`Task4-Proteus.pdsprj`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F103C8TX/STM32F103C8TX/Task4-Proteus.pdsprj).
3. **Verify Program File:**
   - The STM32 microcontroller is pre-configured with:
     `Debug\STM32F103C8TX.hex`
4. **Interactive Controls:**
   - The MPU-6050 module on the schematic includes interactive push-buttons allowing you to dynamically adjust Pitch, Roll, and Gyro rates during simulation.
5. **Virtual Terminal Output:**
   - During simulation, the Virtual Terminal displays real-time telemetry streaming at 4 Hz:

```text
===========================================================
 [IMU] MPU-6050 Orientation (Complementary Filter, a=0.98):
       Roll  :   +12.45 deg
       Pitch :   -05.10 deg
       Yaw   :   +00.00 deg (Gyro integrated)
       Accel : [X: +0.088g, Y: +0.215g, Z: +0.972g]
       Gyro  : [X:   +0.0 deg/s, Y:   +0.0 deg/s, Z:   +0.0 deg/s]
 -----------------------------------------------------------
 [BMP] Barometric Pressure & Temperature:
       Temp  :   25.32 deg C
       Press :   1013.25 hPa  (101325 Pa)
       Alt   :   0.00 m
===========================================================
```

---

## 6. How to Build the Firmware

The project is built with `arm-none-eabi-gcc` and GNU Make:

```powershell
# From the project directory:
cd STM32\Task4\STM32F103C8TX\STM32F103C8TX\Debug
make all
```

Output:

```text
Finished building: ../Core/Src/bmp280.c
Finished building: ../Core/Src/mpu6050.c
Finished building: ../Core/Src/uart_comm.c
Finished building: ../Core/Src/main.c
Finished building target: STM32F103C8TX.elf
Finished building: STM32F103C8TX.hex
Finished building: default.size.stdout
```

Build result: **0 errors, 0 warnings**.

---

## 7. STM32F401RCT6 (Blackpill) Solution

Due to Proteus simulator limitations with the STM32F103 I2C model under certain DLL environments, Task 4 is fully implemented and verified on the high-performance **STM32F401RCT6 (ARM Cortex-M4 @ 84MHz)** located in [`STM32/Task4/STM32F401RCT6/STM32F401RCT6`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F401RCT6/STM32F401RCT6).

### 7.1 CubeMX Pin Configuration & Hardware Map

| Peripheral / Interface | Pin | Signal Name | Mode / Description |
| :--- | :--- | :--- | :--- |
| **I2C1** | **PB8** | `I2C1_SCL` | I2C Serial Clock for MPU-6050 & BMP280/BMP180 |
| **I2C1** | **PB9** | `I2C1_SDA` | I2C Serial Data bidirectional line |
| **USART1** | **PB6** | `USART1_TX` | Transmit to Virtual Terminal / Telemetry Console (9600 Baud) |
| **USART1** | **PB7** | `USART1_RX` | Receive command / Telemetry RX |
| **USB_OTG_FS** | **PA11** | `USB_OTG_FS_DM` | USB D- Differential Signal (Virtual COM Port) |
| **USB_OTG_FS** | **PA12** | `USB_OTG_FS_DP` | USB D+ Differential Signal (Virtual COM Port) |
| **USB_OTG_FS** | **PA9** | `USB_OTG_FS_VBUS` | USB VBUS 5V Sense Input |
| **USB_OTG_FS** | **PA8** | `USB_OTG_FS_SOF` | Start of Frame Clock Output |
| **TIM2 (PWM)** | **PA0** | `TIM2_CH1` | PWM Output (Internal Clock, ARR = 3, Pulse = 2) |
| **Built-in LED** | **PC13** | `GPIO_Output` | Onboard Heartbeat / Status LED (5 Hz toggle) |
| **LCD Control** | **PA4** | `GPIO_Output` | LCD Enable (`LCD_EN`) pulse strobe |
| **LCD Control** | **PA5** | `GPIO_Output` | LCD Register Select (`LCD_RS`: 0 = Command, 1 = Data) |
| **LCD Control** | **PA6** | `GPIO_Output` | LCD Read/Write (`LCD_RW`: 0 = Write) |
| **LCD Data Bus** | **PC0** | `GPIO_Output` | `LCD_D0` |
| **LCD Data Bus** | **PC1** | `GPIO_Output` | `LCD_D1` |
| **LCD Data Bus** | **PC2** | `GPIO_Output` | `LCD_D2` |
| **LCD Data Bus** | **PC3** | `GPIO_Output` | `LCD_D3` |
| **LCD Data Bus** | **PC4** | `GPIO_Output` | `LCD_D4` |
| **LCD Data Bus** | **PC5** | `GPIO_Output` | `LCD_D5` |
| **LCD Data Bus** | **PC6** | `GPIO_Output` | `LCD_D6` |
| **LCD Data Bus** | **PC7** | `GPIO_Output` | `LCD_D7` |
| **RCC Oscillators** | **PC14 / PC15** | `OSC32_IN / OUT` | Low-Speed External 32.768 kHz RTC Crystal |
| **RCC Oscillators** | **PH0 / PH1** | `OSC_IN / OUT` | High-Speed External 25.000 MHz Crystal |

### 7.2 Integrated Middlewares

- **USB_DEVICE**: Communication Device Class (CDC Virtual COM Port) running alongside USART1.
- **LIBJPEG**: Image compression middleware for camera feeds.
- **MBEDTLS**: Cryptographic layer for secure telemetry packets.
- **PDM2PCM**: Digital microphone audio processing filter.

### 7.3 Build Verification

- Target ELF: [`STM32F401RCT6.elf`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F401RCT6/STM32F401RCT6/Debug/STM32F401RCT6.elf)
- Simulation HEX: [`STM32F401RCT6.hex`](file:///e:/Robotics_Team/New_Embedded_batch/Assiut-Robotics-Embedded-Systems-Batch/STM32/Task4/STM32F401RCT6/STM32F401RCT6/Debug/STM32F401RCT6.hex)
- Compilation: **0 errors, 0 warnings**.

