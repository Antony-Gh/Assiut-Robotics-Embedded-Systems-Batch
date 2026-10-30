# 8. Capstone Architecture, Debugging Playbook, and Bench Practice

Real-world embedded systems do not exercise peripherals in isolation. A modern robotic platform concurrently runs timer-triggered analog sampling, digital sensor bus polling, closed-loop PWM actuator control, and serial communications while handling asynchronous external interrupts. When a fault occurs in such an integrated environment, simplistic "trial-and-error" debugging fails.

This chapter synthesizes all previous modules into a cohesive engineering reference:
1. **The Multi-Sensor Robotic Capstone Architecture**: A complete system integrating ADC, DMA, Timers, I²C, SPI, UART, and FreeRTOS.
2. **The 10-Layer Root-Cause Debugging Playbook**: A structured, physics-to-firmware diagnostic methodology.
3. **HardFault Forensics on ARM Cortex-M**: Assembly-level exception decoding and register analysis.
4. **Per-Subsystem Failure Diagnostic Checklists**: Rapid bench reference tables for every peripheral.
5. **Seven Practical Hands-On Labs and Capstone**: Rigorous lab walkthroughs with quantifiable acceptance criteria.
6. **Master Engineering Formula Sheet**: Bench calculations for quick lookup.
7. **20-Question Technical Knowledge Check**: Diagnostic questions with comprehensive engineering answers.

---

## 8.1 The Multi-Sensor Robotic Capstone Architecture

The capstone system represents a high-reliability autonomous robotics node. The microcontroller coordinates five concurrent subsystems:

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                      STM32 System Architecture                         │
 │                                                                        │
 │  ┌────────────────┐     ┌────────────────┐     ┌────────────────────┐  │
 │  │ Analog Inputs  │     │  MPU6050 IMU   │     │  Optical Encoder   │  │
 │  │ (Pot / Dist)   │     │   (6-Axis)     │     │  (Speed / Dist)    │  │
 │  └───────┬────────┘     └───────┬────────┘     └─────────┬──────────┘  │
 │          │                      │                        │             │
 │   ADC1 (Regular)              I2C1                     TIM2            │
 │   External Trigger:       Standard Fast Mode       Encoder Mode        │
 │   TIM3 TRGO (1000 Hz)     (400 kHz, 2.2kΩ Pull)    (CH1/CH2 A/B)       │
 │          │                      │                        │             │
 │   DMA1 Channel 1                │                        │             │
 │   Circular Mode                 │                        │             │
 │          │                      │                        │             │
 │  ┌───────▼──────────────────────▼────────────────────────▼──────────┐  │
 │  │              FreeRTOS Task & Queue Pipeline                      │  │
 │  │                                                                  │  │
 │  │  [Sensor Task (Pri 3, 50 Hz)] ──► xSensorQueue ──► [Telemetry]   │  │
 │  │  • Reads MPU6050 via Mutex                          (Pri 2)      │  │
 │  │  • Unpacks ADC Ping-Pong Buffer                        │         │  │
 │  │  • Reads TIM2 Encoder Speed                            │         │  │
 │  └────────────────────────────────────────────────────────┼─────────┘  │
 │                                                           │            │
 │               ┌───────────────────────┐         USART2 / DMA           │
 │               │  TIM1 PWM (20 kHz)    │         115,200 Baud           │
 │               │  Motor H-Bridge Drive │                   │            │
 │               └───────────────────────┘         ┌─────────▼─────────┐  │
 │                                                 │ Host PC Telemetry │  │
 │                                                 └───────────────────┘  │
 └────────────────────────────────────────────────────────────────────────┘
```

### Architectural Guarantees
1. **Zero-Jitter Analog Conversion**: Analog sampling is triggered exclusively by hardware timer `TIM3 TRGO`. Software latency or RTOS context switches cannot cause sample jitter.
2. **Deterministic Bus Access**: The I²C bus is guarded by a recursive Mutex with a finite $10\text{ ms}$ timeout, preventing priority inversion.
3. **Lossless Telemetry Stream**: Telemetry packets flow through an RTOS queue into an interrupt- or DMA-driven UART transmitter. If the host PC stalls, an explicit queue overflow policy discards the oldest non-critical packet rather than locking the CPU.
4. **Hardware Fail-Safe**: A hardware Independent Watchdog (`IWDG`) enforces a $500\text{ ms}$ maximum execution window.

---

## 8.2 HardFault Forensics on ARM Cortex-M

A **HardFault** is an ARM Cortex-M exception triggered when the processor encounters an unrecoverable error during instruction execution, memory access, or interrupt servicing.

### 8.2.1 The Exception Stack Frame
When a HardFault occurs, the Cortex-M processor pushes 8 core registers onto the active stack pointer (**Process Stack Pointer `PSP`** if faulted in an RTOS task, or **Main Stack Pointer `MSP`** if faulted in an ISR or bare-metal):

```text
Stack Pointer (SP) ──► [ R0   ]  (Offset 0x00)
                       [ R1   ]  (Offset 0x04)
                       [ R2   ]  (Offset 0x08)
                       [ R3   ]  (Offset 0x0C)
                       [ R12  ]  (Offset 0x10)
                       [ LR   ]  (Offset 0x14) - Link Register (Caller function)
                       [ PC   ]  (Offset 0x18) - Program Counter (Crashed instruction!)
                       [ xPSR ]  (Offset 0x1C) - Program Status Register
```

- **Program Counter (`PC`)**: The exact address of the assembly instruction that caused the crash.
- **Link Register (`LR`)**: The address of the function that called the failing routine.

---

### 8.2.2 Decoding the Fault Status Registers

The System Control Block (`SCB`) provides hardware status registers that pinpoint the exact cause of the crash.

**Table 8.1 — Cortex-M fault status registers and bit definitions**

| Register | Key Bit Flags | Meaning and Root Cause |
|---|---|---|
| **CFSR** (Configurable Fault Status) | `Bits [7:0]: MMFSR`<br>`Bits [15:8]: BFSR`<br>`Bits [31:16]: UFSR` | Master 32-bit register containing Memory Management, Bus, and Usage fault flags. |
| **MMFSR** (Memory Management) | `DACCVIOL` (Bit 1)<br>`IACCVIOL` (Bit 0) | Data or instruction access violation. Attempted to read/write a memory address prohibited by the MPU. |
| **BFSR** (Bus Fault) | `PRECISERR` (Bit 1)<br>`IMPRECISERR` (Bit 2)<br>`BFARVALID` (Bit 7) | **PRECISERR**: Attempted to access an unmapped or unclocked peripheral register (`PC` points to offending instruction).<br>**IMPRECISERR**: Asynchronous write buffer error (crashed instruction occurred earlier).<br>**BFARVALID**: The address stored in `BFAR` is valid. |
| **UFSR** (Usage Fault) | `UNDEFINSTR` (Bit 0)<br>`INVSTATE` (Bit 1)<br>`UNALIGNED` (Bit 8)<br>`DIVBYZERO` (Bit 9) | **UNDEFINSTR**: Execution of corrupted flash/RAM memory.<br>**INVSTATE**: Attempted to execute ARM state instructions (Cortex-M only supports Thumb-2).<br>**UNALIGNED**: Unaligned 32-bit memory access (when trap enabled).<br>**DIVBYZERO**: Integer division by zero (when trap enabled). |
| **BFAR** (Bus Fault Address) | 32-bit Address | The exact memory address that caused the bus fault (e.g., accessing unpowered peripheral `0x40013800`). |
| **MMFAR** (MemManage Address) | 32-bit Address | The memory address that violated MPU permissions. |

---

### 8.2.3 Production HardFault Handler Implementation

**Listing 8.1 — Assembly stub and C diagnostic handler for HardFault forensics**

```c
/* =========================================================================
   Assembly Stub: Extracts correct stack pointer (MSP vs PSP)
   ========================================================================= */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4            \n" /* Check bit 2 of EXC_RETURN */
        "ite eq                \n"
        "mrseq r0, msp         \n" /* If 0, exception stack frame is on MSP */
        "mrsne r0, psp         \n" /* If 1, exception stack frame is on PSP */
        "ldr r1, =HardFault_Decoder \n"
        "bx r1                 \n"
    );
}

/* =========================================================================
   C Decoder: Traps register state and halts for debugger inspection
   ========================================================================= */
void HardFault_Decoder(uint32_t *fault_stack)
{
    volatile uint32_t stacked_r0  = fault_stack[0];
    volatile uint32_t stacked_r1  = fault_stack[1];
    volatile uint32_t stacked_r2  = fault_stack[2];
    volatile uint32_t stacked_r3  = fault_stack[3];
    volatile uint32_t stacked_r12 = fault_stack[4];
    volatile uint32_t stacked_lr  = fault_stack[5];
    volatile uint32_t stacked_pc  = fault_stack[6]; /* CRASHED INSTRUCTION ADDRESS */
    volatile uint32_t stacked_psr = fault_stack[7];

    volatile uint32_t cfsr = SCB->CFSR;
    volatile uint32_t hfsr = SCB->HFSR;
    volatile uint32_t bfar = SCB->BFAR;
    volatile uint32_t mmfar = SCB->MMFAR;

    /* Prevent compiler from optimizing away diagnostic variables */
    (void)stacked_r0; (void)stacked_r1; (void)stacked_r2; (void)stacked_r3;
    (void)stacked_r12; (void)stacked_lr; (void)stacked_pc; (void)stacked_psr;
    (void)cfsr; (void)hfsr; (void)bfar; (void)mmfar;

    /* Halt processor here for debugger attachment */
    __BKPT(0);
    while (1);
}
```

---

## 8.3 The 10-Layer Root-Cause Debugging Playbook

When embedded firmware fails to operate, apply this systematic bottom-up diagnostic playbook:

```text
┌─────────────────────────────────────────────────────────────┐
│ Layer 10: Application Logic & State Machines                │
├─────────────────────────────────────────────────────────────┤
│ Layer 9:  Concurrency, RTOS Priorities, Queues, & Race Cond │
├─────────────────────────────────────────────────────────────┤
│ Layer 8:  DMA Channel Routing, Buffer Ownership, Alignment  │
├─────────────────────────────────────────────────────────────┤
│ Layer 7:  NVIC Interrupt Enables, Preemption Groups & ISRs  │
├─────────────────────────────────────────────────────────────┤
│ Layer 6:  Peripheral Configuration Registers (Baud, Presc)  │
├─────────────────────────────────────────────────────────────┤
│ Layer 5:  Peripheral Clock Gating (RCC Enable Registers)    │
├─────────────────────────────────────────────────────────────┤
│ Layer 4:  Pin Multiplexing & Alternate Function (AF) Config │
├─────────────────────────────────────────────────────────────┤
│ Layer 3:  Clock Tree, PLL Lock, & Bus Prescalers            │
├─────────────────────────────────────────────────────────────┤
│ Layer 2:  Reset Circuit, NRST Line, BOOT0/BOOT1 Pins        │
├─────────────────────────────────────────────────────────────┤
│ Layer 1:  Power Rails (VDD, VDDA, VREF+), Decoupling, GND   │
└─────────────────────────────────────────────────────────────┘
```

1. **Layer 1: Power & Ground**: Measure $V_{DD}$ and $V_{DDA}$ with a DMM directly at the MCU pins. Verify $3.30\text{ V} \pm 5\%$. Check that all $100\text{ nF}$ decoupling capacitors are populated. Verify continuity between sensor GND and MCU GND.
2. **Layer 2: Reset & Boot State**: Verify `NRST` is logic HIGH ($3.3\text{ V}$). Confirm `BOOT0` is tied securely to GND (a floating `BOOT0` causes random boots into the system ROM bootloader).
3. **Layer 3: Clocks & PLL**: Verify the HSE external crystal is oscillating using an oscilloscope with a $10\times$ low-capacitance probe. Check if firmware is stuck in `Error_Handler()` inside `SystemClock_Config()`.
4. **Layer 4: Pinout & Alternate Functions**: Check the MCU package datasheet alternate function mapping table. Ensure the GPIO mode is set to `GPIO_MODE_AF_PP` (or `GPIO_MODE_AF_OD` for I²C), not standard GPIO input/output.
5. **Layer 5: Peripheral Clock Gating**: Verify that the peripheral clock enable bit is set in RCC (e.g., `RCC_APB1ENR_USART2EN`). Accessing any peripheral register while its clock is disabled causes an immediate Bus Fault!
6. **Layer 6: Peripheral Registers**: Inspect peripheral SFRs in the STM32CubeIDE debugger:
   - UART: Check `USART_BRR` matches baud rate math; verify `UE` and `TE`/`RE` bits are asserted.
   - Timers: Verify `TIMx_CR1_CEN` is set; verify `PSC` and `ARR` values.
7. **Layer 7: Interrupts & NVIC**: Verify the peripheral interrupt enable bit is set (e.g., `USART_CR1_RXNEIE`). Verify the global interrupt is enabled in the NVIC (`NVIC_EnableIRQ()`).
8. **Layer 8: DMA & Memory**: Confirm DMA channel mapping matches silicon hardware. Verify buffer pointers are located in valid SRAM and meet 32-byte cache alignment rules if on Cortex-M7.
9. **Layer 9: Concurrency & RTOS**: Verify stack high-water marks (`uxTaskGetStackHighWaterMark()`). Verify ISRs calling FreeRTOS APIs obey the `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` rule.
10. **Layer 10: Application Logic**: Verify data typing, signed two's complement decoding, endianness, and unit conversions.

---

## 8.4 Per-Subsystem Failure Diagnostic Checklists

### 8.4.1 GPIO and Smoke-Test Checklist
- [ ] Is `__HAL_RCC_GPIOx_CLK_ENABLE()` called before configuring the pin?
- [ ] For LEDs, is the drive mode Push-Pull (`GPIO_MODE_OUTPUT_PP`)?
- [ ] Is the output speed appropriate (`GPIO_SPEED_FREQ_LOW` for LEDs to minimize EMI)?
- [ ] If using an external button, is an internal pull-up/pull-down resistor enabled, or is there an external physical resistor?
- [ ] For buttons triggering interrupts, is the pin configured as `GPIO_MODE_IT_FALLING` / `RISING`, and is `HAL_NVIC_EnableIRQ(EXTIx_IRQn)` called?

### 8.4.2 UART / USART Checklist
- [ ] Are TX and RX swapped? (MCU TX must connect to Peripheral RX; MCU RX to Peripheral TX).
- [ ] Is there a common GND connection between both boards?
- [ ] Does the measured bit width on an oscilloscope match $\frac{1}{\text{Baud}}$ ($8.68\ \mu\text{s}$ for 115,200 baud)?
- [ ] Is the baud rate error strictly under $2.0\%$?
- [ ] If reception locks up permanently, are you clearing the Overrun Error (`ORE`) flag in `HAL_UART_ErrorCallback()`?
- [ ] In DMA mode, is `HAL_UARTEx_ReceiveToIdle_DMA()` re-armed inside the callback?

### 8.4.3 I²C Bus Checklist
- [ ] Are physical external pull-up resistors ($2.2\text{ k}\Omega$ to $4.7\text{ k}\Omega$) populated on both SDA and SCL?
- [ ] Is the 7-bit device address shifted left by 1 bit (`ADDR << 1`) when calling STM32 HAL functions?
- [ ] Does the sensor's `WHO_AM_I` register read the expected value?
- [ ] Has the sensor's power-on reset time ($t_{\text{POR}} \approx 50\text{ ms}$) elapsed before sending commands?
- [ ] If the bus is locked with SDA clamped LOW, does the startup code execute the 9-clock bit-bang unjam sequence?

### 8.4.4 SPI Bus Checklist
- [ ] Is the Chip Select (CS) pin configured as a standard GPIO output, driven LOW before the transfer and returned HIGH after?
- [ ] Do the Clock Polarity (`CPOL`) and Clock Phase (`CPHA`) settings in CubeMX match the sensor datasheet SPI Mode (Mode 0 vs Mode 3)?
- [ ] When reading data from an SPI slave, are you transmitting dummy bytes (`0x00` or `0xFF`) to generate clock pulses?
- [ ] Is the SPI clock frequency ($f_{\text{SCK}}$) within the maximum rating of the slowest peripheral on the bus?

### 8.4.5 ADC Analog Front-End Checklist
- [ ] Is the pin configured in Analog Mode (`GPIO_MODE_ANALOG`) with digital input buffers disabled?
- [ ] Is $f_{\text{ADC}}$ strictly below the family maximum ($14\text{ MHz}$ on STM32F1)?
- [ ] Is the sampling time $T_{\text{sample}}$ long enough for the external source impedance $R_{\text{AIN}}$?
- [ ] Has `HAL_ADCEx_Calibration_Start(&hadc1)` executed before the first conversion?
- [ ] In multi-channel Scan Mode, is DMA enabled? (Without DMA, the Data Register only retains the last converted channel).

### 8.4.6 Timers and PWM Checklist
- [ ] Did you account for the APB Timer $\times 2$ clock multiplier rule when computing input frequency?
- [ ] Did you remember that PSC and ARR divide by $(N + 1)$?
- [ ] Did you call `HAL_TIM_PWM_Start()` for the specific channel, not just `HAL_TIM_Base_Start()`?
- [ ] For advanced timers (TIM1/TIM8), is the main output enabled (`TIM1->BDTR |= TIM_BDTR_MOE` or `HAL_TIM_PWM_Start`)?

---

## 8.5 Seven Practical Hands-On Labs and Capstone

Every lab defines a specific engineering behavior and quantifiable acceptance test.

### Lab 1 — GPIO Heartbeat and Oscilloscope Clock Verification
- **Objective**: Prove power, reset, clock tree stability, and baseline execution loop.
- **Procedure**:
  1. In CubeMX, configure HSE 8 MHz crystal with PLL multiplier $\times 9$ to achieve 72 MHz SYSCLK.
  2. Route MCO (Microcontroller Clock Output) pin to PA8, selecting SYSCLK.
  3. Configure LED pin (PC13 on Blue Pill, PA5 on Nucleo-F401) as Push-Pull Output.
  4. In `main.c`, toggle LED with `HAL_Delay(250)`.
- **Acceptance Criteria**: Scope on PA8 measures exactly $72.00\text{ MHz} \pm 50\text{ ppm}$. LED blinks steadily at $2.00\text{ Hz}$ across cold power-on without debugger attached.

### Lab 2 — Interactive UART Telemetry Console with IDLE Line DMA
- **Objective**: Implement robust, variable-length, non-blocking serial communications.
- **Procedure**:
  1. Configure USART2 for 115,200 baud, 8N1. Enable DMA1 Channel 6 for RX.
  2. Implement `HAL_UARTEx_ReceiveToIdle_DMA()`.
  3. Support commands: `PING` (replies `PONG`), `LED 1` (turns on LED), `LED 0` (turns off LED).
  4. Flooding test: Send a continuous stream of 500 random characters without line breaks.
- **Acceptance Criteria**: The system successfully parses commands. Flooded inputs do not trigger an unhandled Overrun Error (`ORE`) or lock up the console.

### Lab 3 — Precision Timer Calculations and PWM Oscilloscope Verification
- **Objective**: Validate timer prescaler/auto-reload math against physical silicon waveforms.
- **Procedure**:
  1. Target: Exactly $1.000\text{ kHz}$ PWM with $25.0\%$ duty cycle on TIM2 Channel 1 (PA0).
  2. With $f_{\text{TIM}} = 72\text{ MHz}$, calculate: $\text{PSC} = 71 \implies 1\text{ MHz tick}$; $\text{ARR} = 999 \implies 1000\text{ ticks} = 1.000\text{ ms}$; $\text{CCR1} = 250$.
  3. Start PWM: `HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1)`.
- **Acceptance Criteria**: Oscilloscope cursor measures frequency $= 1000.0\text{ Hz} \pm 0.5\text{ Hz}$ and positive pulse width $= 250.0\ \mu\text{s} \pm 0.5\ \mu\text{s}$.

### Lab 4 — ADC Polling and Dynamic Supply Compensation
- **Objective**: Understand quantization math and eliminate supply voltage sag errors.
- **Procedure**:
  1. Configure ADC1 Channel 0 (PA0) and Channel 17 ($V_{\text{REFINT}}$).
  2. Connect a potentiometer to PA0. Measure wiper voltage with a calibrated 6.5-digit DMM.
  3. Compute $V_{\text{IN}}$ using uncompensated formula, then compute using $V_{\text{REFINT}}$ compensation.
  4. Lower board supply voltage from $3.3\text{ V}$ to $3.0\text{ V}$ using a lab power supply.
- **Acceptance Criteria**: As supply voltage varies, uncompensated reading drifts by $> 9\%$; $V_{\text{REFINT}}$-compensated calculation remains accurate within $\pm 0.8\%$ of DMM reading.

### Lab 5 — Deterministic Timer-Triggered ADC with Circular DMA
- **Objective**: Implement zero-CPU, zero-jitter multi-sample data acquisition.
- **Procedure**:
  1. Configure TIM3 TRGO to trigger ADC1 at exactly $10.0\text{ kSPS}$.
  2. Configure ADC1 in Circular DMA mode into a 1000-halfword buffer ($N = 1000$).
  3. In `HAL_ADC_ConvHalfCpltCallback()`, toggle PB0; in `HAL_ADC_ConvCpltCallback()`, toggle PB1.
- **Acceptance Criteria**: Oscilloscope probe on PB0 shows pulses at exactly $20.0\text{ Hz}$ ($T = \frac{N}{2 F_s} = \frac{500}{10{,}000} = 50.0\text{ ms}$). CPU utilization remains under $1\%$.

### Lab 6 — Reusable InvenSense MPU6050 Sensor Driver Integration
- **Objective**: Integrate a multi-layer, modular device driver with bus abstraction.
- **Procedure**:
  1. Connect MPU6050 to I2C1 (PB6/PB7) with $2.2\text{ k}\Omega$ pull-up resistors.
  2. Integrate `mpu6050.h` and `mpu6050.c`. Initialize with `MPU6050_Init()`.
  3. Execute offset calibration while the sensor rests flat on the lab bench.
  4. Stream real-time pitch and roll angles calculated from accelerometer vectors.
- **Acceptance Criteria**: With sensor stationary, $a_z = 9.81 \pm 0.1\text{ m/s}^2$, $a_x = 0.0 \pm 0.1\text{ m/s}^2$, $a_y = 0.0 \pm 0.1\text{ m/s}^2$. Disconnecting the sensor returns `MPU6050_ERR_BUS` without freezing the MCU.

### Lab 7 — FreeRTOS Dual-Task Producer-Consumer Telemetry Pipeline
- **Objective**: Multi-threaded real-time execution with queue buffering and priority enforcement.
- **Procedure**:
  1. In CubeMX, enable FreeRTOS. Switch HAL Timebase Source to TIM4.
  2. Create Sensor Task ($50\text{ Hz}$, Priority 3) and Telemetry Task (Priority 2).
  3. Measure task execution times and verify stack high-water marks.
- **Acceptance Criteria**: Both tasks run continuously. Artificially stalling the UART transmitter causes queue drops to increment safely without starving the sensor task.

---

## 8.6 Master Engineering Formula Sheet

**Table 8.2 — Master engineering formula reference sheet**

| Parameter | Governing Formula | Key Variables & Units | Critical Practical Rule |
|---|---|---|---|
| **Timer Frequency** | $f_{\text{update}} = \frac{f_{\text{TIM}}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$ | $f_{\text{TIM}}$: Timer clock (Hz)<br>$\text{PSC}$: 16-bit prescaler<br>$\text{ARR}$: Auto-reload register | Account for APB timer $\times 2$ multiplier if APB divider $> 1$. |
| **PWM Duty Cycle** | $\text{Duty (\%)} = \frac{\text{CCR}}{\text{ARR} + 1} \times 100\%$ | $\text{CCR}$: Capture/Compare register | In PWM Mode 1, output is HIGH while $\text{CNT} < \text{CCR}$. |
| **ADC LSB Size** | $\text{LSB} = \frac{V_{\text{REF}+} - V_{\text{REF}-}}{2^N}$ | $V_{\text{REF}}$: Reference voltage (V)<br>$N$: Resolution bits (12 bits $\implies 4096$) | Typical LSB for 3.3V 12-bit ADC is $0.80566\text{ mV}$. |
| **Code to Voltage** | $V_{\text{IN}} = \text{Code} \times \frac{V_{\text{REF}}}{2^N - 1}$ | $\text{Code}$: Raw integer reading | Divide by $4095$ so that full-scale code represents exactly $3.300\text{ V}$. |
| **Total ADC Conversion** | $T_{\text{conv}} = T_{\text{sample}} + 12.5\text{ cycles}$ | $T_{\text{sample}}$: Sampling time cycles | Ensure $T_{\text{sample}} \ge 8.3 \times R_{\text{AIN}} \times C_{\text{ADC}}$. |
| **Circular DMA Deadline** | $T_{\text{deadline}} = \frac{N}{2 \times F_s}$ | $N$: Total buffer length (samples)<br>$F_s$: Sampling rate (Hz) | Half-buffer processing time must be strictly less than $T_{\text{deadline}}$. |
| **UART Baud Divisor** | $\text{Div} = \frac{f_{\text{PCLK}}}{16 \times \text{Baud}}$ | $f_{\text{PCLK}}$: Bus clock (Hz) | Baud error must be strictly under $\pm 2.0\%$. |
| **UART Bandwidth** | $\text{Throughput} \approx \frac{\text{Baud}}{10}\text{ Bytes/sec}$ | Assumes 8N1 frame format | 115,200 baud yields maximum $11.52\text{ kB/s}$. |
| **I²C Max Pull-up** | $R_{p,\max} = \frac{t_{r,\max}}{0.8473 \times C_b}$ | $t_{r,\max}$: Max rise time ($300\text{ ns}$ for 400 kHz)<br>$C_b$: Bus capacitance (Farads) | Slower rise times violate maximum high-level timing. |
| **I²C Min Pull-up** | $R_{p,\min} = \frac{V_{DD} - V_{OL,\max}}{I_{OL}}$ | $V_{OL} \le 0.4\text{ V}$, $I_{OL} = 3\text{ mA}$ | $R_{p,\min} \approx 967\ \Omega$ at $3.3\text{ V}$. |

---

## 8.7 20-Question Technical Knowledge Check with Comprehensive Answers

Use these questions for oral technical interviews or written batch examinations.

#### Question 1: What does an `.ioc` file actually store, and what is its role?
**Answer**: An `.ioc` file is a plain-text configuration file maintained by STM32CubeMX. It stores all user-selected hardware parameters: target MCU part number, pin multiplexing (GPIO modes and Alternate Functions), clock tree configurations (PLL multipliers and dividers), peripheral parameter settings, and middleware configurations. It is used exclusively by the CubeMX code generation engine to synthesize C initialization source files (`main.c`, `stm32f4xx_hal_msp.c`, etc.). It is not compiled or flashed to the microcontroller.

#### Question 2: Why can regenerating code in CubeMX overwrite custom application logic, and how is this avoided?
**Answer**: When CubeMX regenerates code, its template engine rewrites the project files from scratch. To protect custom firmware, CubeMX parses the existing files and preserves only code enclosed within paired `/* USER CODE BEGIN <Name> */` and `/* USER CODE END <Name> */` comments. Any application logic placed outside these designated blocks is permanently deleted during code generation. The best practice is to place application logic in separate, dedicated `.c` and `.h` files that CubeMX never touches.

#### Question 3: What is the fundamental difference between STM32CubeMX and STM32CubeIDE?
**Answer**: STM32CubeMX is a graphical hardware configuration and initialization code generation tool. STM32CubeIDE is a full-featured integrated development environment built on Eclipse and the GNU ARM GCC toolchain that includes code editing, compiling, linking, flashing, and hardware debugging (GDB/OpenOCD/ST-LINK). STM32CubeIDE includes an embedded version of CubeMX internally, allowing `.ioc` editing directly within the IDE.

#### Question 4: When is Polling appropriate in professional embedded systems, and when is it forbidden?
**Answer**: Polling is appropriate during early power-on initialization, single-character debug transmissions inside a HardFault crash handler, and brief bounded delays where the timeout is short and no higher-priority deadlines can be missed. It is strictly forbidden in continuous multi-tasking loops, high-rate sensor acquisition, and battery-powered applications where blocking wastes vital CPU cycles and prevents power-saving sleep modes.

#### Question 5: Compare Interrupts and DMA in terms of CPU involvement.
**Answer**: An interrupt requests the CPU to immediately suspend its current thread, execute a hardware context switch, and run an Interrupt Service Routine (ISR) to process data. The CPU is directly involved in executing the transfer. DMA (Direct Memory Access) utilizes an independent hardware bus master to transfer data directly between peripherals and SRAM across the internal bus matrix with zero CPU involvement. In high-performance systems, DMA performs the continuous data transfers, while interrupts are generated only when the transfer reaches halfway (`HT`) or finishes (`TC`).

#### Question 6: Why is the C `volatile` keyword insufficient for synchronizing shared buffers between an ISR and `main()`?
**Answer**: The `volatile` keyword only instructs the compiler not to optimize away reads and writes to a variable by caching it in a CPU register; it forces memory access on every read. However, `volatile` provides **zero atomicity** for multi-byte variables on 32-bit cores, provides **no mutual exclusion** (it cannot prevent an ISR from preempting a read mid-structure), enforces **no memory barriers** against out-of-order CPU pipeline execution, and provides **no cache invalidation** on Cortex-M7 cores with D-Cache. Synchronization requires disabling interrupts, atomic primitives, or RTOS queues.

#### Question 7: Why can the timer clock frequency ($f_{\text{TIM}}$) be double the APB bus clock frequency?
**Answer**: On STM32 microcontrollers, internal timer clock multipliers automatically double the clock frequency feeding the timer peripherals whenever the APB prescaler is greater than $1$ ($\text{APB Prescaler} \ne 1$). This hardware feature ensures that even when peripheral bus frequencies are divided down to conserve dynamic power, the timers retain maximum high-resolution counting frequency.

#### Question 8: Why do timer formulas use $(\text{PSC} + 1)$ and $(\text{ARR} + 1)$ instead of $\text{PSC}$ and $\text{ARR}$?
**Answer**: The hardware prescaler and auto-reload registers are zero-indexed counters. When a register value of `0` is loaded into the prescaler, the input clock is divided by $1$ (no division). When a value of $N$ is loaded, the hardware counter must count through $N + 1$ discrete clock edges (from $0$ up to and including $N$) before rolling over and generating an update event.

#### Question 9: Explain Clock Polarity (CPOL) and Clock Phase (CPHA) in SPI.
**Answer**: `CPOL` defines the idle electrical logic level of the Serial Clock line (`SCLK`) when no communication is active ($0 = \text{LOW}$, $1 = \text{HIGH}$). `CPHA` defines which clock edge samples (captures) data and which edge toggles (shifts) data ($0 = \text{sample on first clock transition}$; $1 = \text{sample on second clock transition}$). Combined, they define SPI Modes 0 through 3.

#### Question 10: Why does an I²C bus require external pull-up resistors, and what governs their values?
**Answer**: I²C pins operate in an open-drain (wired-AND) configuration. Devices can only actively pull the bus lines LOW to ground; they cannot drive them HIGH. External pull-up resistors are required to pull the lines up to $V_{DD}$ when all devices release the bus. The maximum allowable resistance is governed by the total bus capacitance $C_b$ and the maximum rise time specification ($R_{p,\max} = \frac{t_{r,\max}}{0.8473 C_b}$). The minimum resistance is governed by the maximum allowable sink current of the output transistors ($I_{OL} = 3\text{ mA}$) to guarantee that $V_{OL}$ remains below $0.4\text{ V}$.

#### Question 11: What are the primary root causes of garbled characters on a UART console?
**Answer**:
1. Baud rate mismatch exceeding $\pm 2.5\%$, caused by incorrect clock tree configuration or relying on an inaccurate internal RC oscillator (HSI) over temperature.
2. Mismatched frame parameters (e.g., host set to 8N1 while MCU transmits 8E1 or 9-bit data).
3. Missing common ground (GND) wire between the microcontroller and the USB-UART adapter.
4. Swapped TX and RX wiring, or connecting TTL 3.3V UART directly to an RS-232 $\pm 12\text{ V}$ port.

#### Question 12: What is the difference between ADC Resolution and ADC Accuracy?
**Answer**: Resolution is the number of discrete digital intervals the converter can divide the input span into ($12\text{ bits} \implies 4096$ quantization codes, $\text{LSB} \approx 0.8\text{ mV}$). Accuracy is the real-world degree of conformity between the converted digital code and the true absolute physical voltage. Accuracy is degraded by offset error, gain error, integral non-linearity (INL), thermal noise, and reference voltage sag.

#### Question 13: What is Aliasing in sampled data systems, and how is it prevented?
**Answer**: Aliasing occurs when an analog signal contains frequency components higher than half the sampling frequency ($f > \frac{F_s}{2}$, the Nyquist frequency). During sampling, these high-frequency components fold back across the Nyquist threshold, appearing as indistinguishable, false low-frequency signals in the digitized data. Aliasing cannot be removed in software; it must be prevented before sampling using an analog hardware low-pass **anti-aliasing filter**.

#### Question 14: What causes data loss in circular DMA, and how is it detected?
**Answer**: In circular DMA, data loss occurs when the CPU consumer fails to finish processing the previous half of the buffer before the DMA hardware producer wraps around and begins overwriting that same memory space. This is detected by implementing a software processing flag or overrun counter inside the Half-Transfer (`HT`) and Transfer-Complete (`TC`) interrupt callbacks.

#### Question 15: What makes an embedded sensor driver truly "reusable"?
**Answer**: A reusable driver has zero hardcoded peripheral instances (`&hi2c1`), encapsulates all instance state inside a handle struct, returns typed error codes, enforces finite bus timeouts, outputs clean engineering SI units, isolates physical I/O behind function-pointer bus abstractions, and can be compiled and unit-tested on a host PC without hardware.

#### Question 16: When should an engineer transition from a bare-metal superloop to an RTOS?
**Answer**: When an application contains multiple independent execution threads with different, asynchronous deadlines; when integrating heavy third-party middleware stacks (USB, TCP/IP, SD-card file systems) that rely on blocking operations; and when non-blocking state machines become too complex to verify and maintain.

#### Question 17: What is the fundamental difference between a Queue, a Semaphore, and a Mutex in FreeRTOS?
**Answer**:
- **Queue**: A thread-safe FIFO buffer that passes data packets **by value** between tasks or from ISRs to tasks.
- **Semaphore**: A flag (binary) or counter (counting) used for **synchronization and event signaling**. Does not possess ownership.
- **Mutex**: A mutual exclusion lock used to protect a **shared resource**. Has ownership semantics and features **Priority Inheritance** to prevent priority inversion. Mutexes must never be accessed from an ISR.

#### Question 18: Why must Interrupt Service Routines (ISRs) be kept as short as possible?
**Answer**: While an ISR is executing, all interrupts of equal or lower priority are blocked. Lengthy ISRs increase interrupt latency across the entire system, induce severe timing jitter in periodic control loops, cause UART/SPI receiver overrun errors (`ORE`), and delay the execution of RTOS tasks.

#### Question 19: An I²C sensor fails to acknowledge its address upon boot. List the diagnostic sequence.
**Answer**:
1. Check $V_{DD}$ and GND at the sensor pins with a DMM.
2. Verify that physical $2.2\text{ k}\Omega$ to $4.7\text{ k}\Omega$ pull-up resistors are installed.
3. Confirm that the 7-bit datasheet address is shifted left by 1 bit in software (`ADDR << 1`).
4. Ensure the sensor's power-on reset time ($t_{\text{POR}}$) has elapsed before the first command.
5. Inspect SDA and SCL with an oscilloscope to verify that clock pulses are toggling and that the signal achieves sharp rise times to $3.3\text{ V}$.

#### Question 20: How should an engineer verify that a timer is generating the exact calculated frequency?
**Answer**: Configure a timer channel in Output Compare (Toggle) or PWM mode routed to an external GPIO pin. Measure the output waveform directly using a calibrated oscilloscope or hardware logic analyzer with a frequency counter. Never attempt to verify timer rates by toggling software flags and reading `HAL_GetTick()`, as software overhead and SysTick resolution introduce significant measurement error.
