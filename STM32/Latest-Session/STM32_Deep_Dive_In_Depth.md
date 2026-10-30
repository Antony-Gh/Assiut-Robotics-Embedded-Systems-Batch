# STM32 Deep-Dive — In-Depth Textbook

This file is the expanded companion to `STM32_Deep_Dive_Complete_Guide.md` (also copied as `STM32_Deep_Dive_Complete_Guide_Compact.md`). The compact guide is a field checklist. This document is the **why, how, and how-to-verify** behind every topic.

From CubeMX configuration to deterministic data acquisition and FreeRTOS.

| Configure | Implement | Engineer |
|-----------|-----------|----------|
| Pins, clocks, peripherals | HAL, interrupts, DMA | Timing, drivers, RTOS |

**Audience:** embedded-systems students and developers who already know C and basic digital electronics, and who need STM32 firmware that is correct, timed, and maintainable.

> **Family-specific warning.** STM32 is not one chip. Pin mappings, clock trees, timer TRGO routing, ADC architectures (ADC1 vs ADC3 vs oversampling vs dual-ADC), DMA (channel vs stream vs DMAMUX), interrupt vector names, HAL APIs, and middleware all vary. Every listing here is a **pattern**. Open the datasheet (electrical, pinout) and the reference manual (registers, clocks, peripherals) for **your exact part number** before wiring or trusting a period.

---

## How to use this textbook

Embedded work fails when people skip layers. A UART that “prints garbage” is often a clock problem. An ADC that “reads 0” is often analog mode, VREF, or the wrong channel. A DMA callback that “never fires” is often NVIC, the wrong request line, or start-order.

Use this loop for every peripheral:

1. **Define the hardware problem.** What physical signal exists? What must the firmware produce, how often, and what happens on failure?
2. **Read the MCU documents.** Datasheet: pins, voltage, AF table. Reference manual: clock tree, peripheral chapters, DMA request map. Board schematic: oscillators, USB-UART, LEDs, sensor buses.
3. **Configure one thing.** CubeMX pin, clock, then one peripheral. Do not enable ADC + DMA + UART + RTOS in the first attempt.
4. **Inspect generated code.** `main.c`, `*_hal_msp.c`, `stm32xxxx_it.c`, `system_stm32xxxx.c`, linker script. Confirm clocks, AF, NVIC, DMA.
5. **Test a minimal example.** LED, then UART TX, then one ADC poll. Observable success only.
6. **Measure.** Scope, logic analyzer, or a GPIO toggle around the event. Calculation without measurement is a hypothesis.
7. **Integrate.** Drivers, buffers, ownership, then application policy.

Keep a lab notebook: MCU part, board revision, HSE frequency, Cube package version, what you changed, what you measured.

---

## Contents

1. [Learning objectives and the system view](#1-learning-objectives-and-the-system-view)
2. [STM32 tools, projects, and code generation](#2-stm32-tools-projects-and-code-generation)
3. [GPIO, alternate functions, and electrical behavior](#3-gpio-alternate-functions-and-electrical-behavior)
4. [Clock tree and clock calculations](#4-clock-tree-and-clock-calculations)
5. [Peripheral configuration and initialization](#5-peripheral-configuration-and-initialization)
6. [HAL vs LL vs direct-register programming](#6-hal-vs-ll-vs-direct-register-programming)
7. [Polling, interrupts, callbacks, and DMA](#7-polling-interrupts-callbacks-and-dma)
8. [Timers and deterministic timing](#8-timers-and-deterministic-timing)
9. [Timer-triggered ADC + DMA](#9-timer-triggered-adc--dma)
10. [UART: framing and robust streams](#10-uart-framing-and-robust-streams)
11. [SPI: modes, chip select, and timing](#11-spi-modes-chip-select-and-timing)
12. [I²C: addressing, pull-ups, and recovery](#12-i²c-addressing-pull-ups-and-recovery)
13. [Choosing a communication interface](#13-choosing-a-communication-interface)
14. [ADC, analog front ends, and accuracy](#14-adc-analog-front-ends-and-accuracy)
15. [Sampling, throughput, and data integrity](#15-sampling-throughput-and-data-integrity)
16. [Reusable sensor drivers](#16-reusable-sensor-drivers)
17. [Bare-metal superloops and state machines](#17-bare-metal-superloops-and-state-machines)
18. [FreeRTOS fundamentals and patterns](#18-freertos-fundamentals-and-patterns)
19. [Debugging playbook](#19-debugging-playbook)
20. [Seven practical labs](#20-seven-practical-labs)
21. [Engineering checklists and formulas](#21-engineering-checklists-and-formulas)
22. [Knowledge check with answers](#22-knowledge-check-with-answers)

---

# 1. Learning objectives and the system view

*Think in data flow, timing, and ownership — not just peripheral names.*

An STM32 is a **system-on-chip**: Cortex-M CPU, flash, SRAM, clock/reset (RCC), nested vectored interrupt controller (NVIC), GPIO, timers, analog blocks, serial engines, and often DMA. Firmware does not “talk to a sensor” in one step. It configures analog and digital hardware so a **physical quantity** becomes **bits in memory**, then **meaning**, then **action**.

If you cannot draw that chain on paper, you are not ready to debug it.

## 1.1 The chain in detail

```text
Physical world
    │  voltage, current, edges, packets, temperature, mechanical motion
    ▼
Electrical network
    │  dividers, filters, level shifters, pull-ups, grounds, ESD
    ▼
Pin + GPIO / analog mux
    │  AF selection, analog switch, input buffer, output driver
    ▼
Peripheral
    │  ADC, UART, SPI, I2C, TIM, EXTI, …
    ▼
Data movement
    │  polling register, ISR copy, or DMA into RAM
    ▼
Notification
    │  flag, interrupt, DMA HT/TC, RTOS queue
    ▼
Software ownership
    │  who may read this buffer, until when, in which context
    ▼
Application meaning
    │  units, filters, thresholds, state machine
    ▼
Output / actuation / telemetry
        GPIO, PWM, UART packet, display, motor driver
```

| Layer | Question | Failure if ignored | Example |
|-------|----------|--------------------|---------|
| Physical / electrical | What signal exists, with what impedance, amplitude, and bandwidth? | MCU sees the wrong voltage or none. | Divider intended 1.8 V but source cannot drive the ADC sample capacitor. |
| Peripheral | Which block converts or transfers it, in which mode? | Correct voltage, wrong code. | ADC channel 5 configured, probe on channel 4. |
| Data movement | How do bits reach RAM, and how wide are they? | ISR copies too late; DMA writes 32-bit into 16-bit buffer. | DMA half-word vs word mismatch. |
| Timing / notification | When is data valid, and who is told? | Processing the previous sample, or a race. | Callback sets a flag; main loop reads while DMA overwrites. |
| Application | What SI unit, what valid range, what do we do? | “2048” treated as 20.48 °C. | Missing datasheet LSB scale. |
| Output | How is the result delivered, and can that path keep up? | UART blocks the sample loop. | `HAL_UART_Transmit` inside a 10 kHz callback. |

## 1.2 Ownership is a first-class design item

**Ownership** means: which context is allowed to read or write a variable or buffer, and until which event.

Examples of broken ownership:

- ISR writes `line[]` while `main` parses `line[]`.
- DMA circular buffer is processed after the half-complete flag, but processing takes longer than half the buffer period.
- Two tasks call `HAL_I2C_Mem_Read` on the same handle without a mutex.
- `HAL_UART_Transmit_IT` is given a local array that goes out of scope before the IRQ finishes.

A ready flag is **not** a lock. `volatile` is **not** a lock. A debugger halt is **not** proof of timing.

## 1.3 Engineering questions, expanded

**Correctness.** Does code 1876 mean 1.51 V, or is VREF 3.0 V not 3.3 V? Is the thermistor equation in kelvin? Is the I²C endianness MSB-first?

**Timing.** When does the ADC sample instant occur relative to the timer update? What is ISR entry latency? What is worst-case time from “byte arrived” to “command parsed”? Worst-case, not average.

**Throughput.** At 10 kS/s × 2 bytes × 4 channels you need 80 kB/s of **raw** movement, plus copies, plus UART framing if you stream out. If UART is 115200 8-N-1 (~11.5 kB/s payload), you **cannot** dump the raw stream. You must downsample, compress, or use a faster link.

**Resources.** Flash for code and tables, SRAM for stacks/buffers, CPU cycles for filters, APB/AHB bandwidth for DMA, power/thermal for always-on ADC + radio.

**Robustness.** Sensor unplugged, SDA stuck low, UART flood, DMA transfer error, HSE crystal fail, brown-out, stack overflow. Silent `Error_Handler` infinite loops are acceptable in a lab; they are not a product strategy.

**Maintainability.** Can you unit-test scaling math without hardware? Can another student change the I²C address in one header?

## 1.4 Worked example: temperature telemetry

Walk the chain explicitly.

1. **Physics.** An NTC thermistor changes resistance with temperature. It does not output “°C”.
2. **Electrical.** A divider `R_fixed` + NTC from 3.3 V to GND produces `V_node`. Choose resistors so `V_node` stays in the ADC range and source impedance is low enough for the sample time (chapter 14).
3. **Pin.** Analog mode, no pull-up, short analog ground return.
4. **Timing.** A timer update event (TRGO) triggers the ADC at 10 Hz if the plant is slow, or faster if you will filter.
5. **Conversion.** 12-bit code. Convert to voltage, then resistance, then temperature (Steinhart–Hart or lookup).
6. **Movement.** For 10 Hz, polling is fine. For 10 kHz vibration, use timer + ADC + DMA.
7. **Driver.** `TempSensor_ReadCelsius()` returns status + float (or fixed-point). On bus/ADC failure it does **not** return the last good value pretending it is new.
8. **Application.** Median filter, hysteresis around a trip point, ignore the first N samples after power-up.
9. **Telemetry.** UART line: `seq,t_ms,temp_c,raw,flags\r\n` so a dropped line is detectable.
10. **Diagnostics.** Count timeouts, overruns, out-of-range codes.

That is “STM32 work.” Toggling an LED is only proof that the CPU and one pin live.

## 1.5 Learning objectives

After this textbook you should be able to:

- Map a requirement onto pins, clocks, a peripheral, a data path, and a software module.
- Compute timer periods, UART throughput, ADC voltage, and buffer deadlines, then **measure** them.
- Choose polling vs interrupt vs DMA with a reason.
- Write a driver that fails visibly.
- Explain when a superloop is enough and when FreeRTOS is justified.
- Debug from power and clocks upward, not from random code edits.

---

# 2. STM32 tools, projects, and code generation

*Know which tool owns each part of the workflow.*

## 2.1 The toolchain map

| Item | Responsibility | Misconception |
|------|----------------|---------------|
| STM32CubeMX | Pinout, clocks, peripheral settings, middleware, code generation. | It does not write the application, protocol, or recovery policy. |
| STM32CubeIDE | Eclipse-based IDE: edit, GNU Arm toolchain, flash, debug, SFRs. | A green build does not mean the silicon does what you think. |
| STM32CubeCLT / Make / CMake | Headless or custom builds. | Same firmware, different wrapper. |
| `.ioc` | Serialized CubeMX configuration (MCU, pins, clocks, middleware). | It is not an object file and is not flashed. |
| CMSIS | Vendor-independent Cortex-M headers, startup patterns, core registers. | CMSIS is not “the HAL.” |
| HAL / LL package | ST’s peripheral drivers for a **family**. | `HAL_UART_Transmit` on F1 vs H7 can differ in options and handles. |
| Startup (`startup_stm32xxxx.s`) | Reset handler, copy `.data`, zero `.bss`, call `SystemInit`, `main`. | If the vector table is wrong, you may never reach `main`. |
| Linker script (`STM32xxxx_FLASH.ld`) | Flash/RAM origins and sizes, stack/heap symbols. | A link that fits a bigger chip will crash or wrap on a smaller one. |
| Debug probe (ST-LINK, J-Link) | SWD: halt, flash, breakpoints, live memory. | Breakpoints change timing; “works in debug” can hide races. |

CubeMX can run standalone or inside CubeIDE. CubeIDE **uses** CubeMX for the `.ioc`. You can also write register-level C in Keil, IAR, or a Makefile project. The silicon does not care which IDE you used.

## 2.2 What an STM32 project actually contains

A typical CubeIDE project:

```text
Core/Src/main.c              application + MX_*_Init calls
Core/Src/stm32xxxx_it.c      IRQ handlers (often call HAL_xxx_IRQHandler)
Core/Src/stm32xxxx_hal_msp.c low-level MSP: GPIO AF, DMA, NVIC for a peripheral
Core/Inc/stm32xxxx_hal_conf.h which HAL modules are compiled
Drivers/STM32xxxx_HAL_Driver  HAL/LL sources
Drivers/CMSIS                 core + device headers
startup_stm32xxxx.s
STM32xxxx_FLASH.ld
*.ioc
```

**MSP** means MCU Support Package in ST’s wording: clocks for the GPIO port, alternate function, DMA channel bind, NVIC enable. If a UART “does nothing,” `HAL_UART_MspInit` is a prime suspect.

## 2.3 User-code sections

CubeMX regenerates `main.c` and many others. It preserves regions:

```c
/* USER CODE BEGIN 2 */
App_Init();
/* USER CODE END 2 */
```

Edits **outside** those markers disappear on the next generate. That is the usual reason “my UART init vanished.”

Better architecture: keep Cube-generated init, put your code in `app.c` / `sensor.c`, and call them from a small USER CODE block. Then regeneration cannot eat a 400-line parser.

## 2.4 Recommended workflow (why each step exists)

1. **Identify MCU, package, board, oscillator.** Nucleo-F401RE is not “an F4.” HSE may be 8 MHz from the ST-LINK MCO, or a crystal, or nothing (HSI only). Wrong HSE in CubeMX → wrong PLL → wrong baud and timers, while the CPU still “runs.”
2. **Create the project for that target.** Wrong flash size in the linker script yields mysterious HardFaults when you grow the binary.
3. **Pins and AF conflicts.** CubeMX highlights conflicts. Two peripherals cannot own the same pin unless the MCU explicitly muxes that way.
4. **Clock tree and limits.** Flash wait states, VCORE range, USB 48 MHz, ADC clock max. CubeMX shows red when you violate a constraint — do not ignore it.
5. **One peripheral at a time.** Prove UART TX before UART RX DMA. Prove ADC poll before ADC+DMA.
6. **Inspect handles.** `huart2.Instance == USART2`. IRQ name in `stm32xxxx_it.c` must match the vector. DMA parent relationship must match CubeMX.
7. **Build, flash, minimal test.** LED or TX banner.
8. **Version-control `.ioc` with sources.** After regenerate, `git diff` is how you catch lost USER CODE and surprise NVIC changes.

## 2.5 What success at each stage actually means

| Stage | Proves | Does not prove |
|-------|--------|----------------|
| Compile | C is syntactically accepted; types roughly match. | The pin is wired. |
| Link | Symbols exist; sections fit **configured** memory. | The chip on the desk has that memory map. |
| Flash | Probe programmed the region it was told. | Option bytes, boot pin, or a second bank is not hijacking boot. |
| Run | Reset handler and `main` likely started (if you see an LED). | Interrupts, DMA, and analog are correct. |
| Debug halt | You can read registers **at a frozen instant**. | Real-time deadlines still hold. SysTick and peripherals may still run depending on DBGMCU freeze bits. |

## 2.6 CubeMX does not design your software

It will not:

- Define packet framing or CRC.
- Choose buffer sizes from your sample rate.
- Implement backpressure.
- Calibrate a thermistor.
- Decide mutex vs queue.
- Guarantee cache coherency on M7.

It **will** happily generate `HAL_ADC_Start_DMA` in `main` before your buffer logic exists. Treat generated `main` as a skeleton.

---

# 3. GPIO, alternate functions, and electrical behavior

*A pin is an electrical connection, not a Boolean in C.*

## 3.1 What a pin really is

Each package pin (except power/ground/oscillator-only pins) typically connects to:

- An analog switch into ADC/DAC/comparators (when analog mode is selected).
- A digital input Schmitt trigger (when input or AF input).
- A pull-up and pull-down (weak, optional).
- An output driver that can be push-pull or open-drain, with a programmable speed/slew class.
- An alternate-function multiplexer toward USART, TIM, SPI, I2C, etc.

**Analog mode** disconnects the digital Schmitt trigger so it does not chew analog current or disturb the voltage. If you leave the pin as GPIO input and “just read the ADC,” many families give garbage or load the source.

## 3.2 Modes

| Mode | Electrical behavior | Typical use |
|------|---------------------|-------------|
| Input floating | High-Z digital input. Undefined if nothing drives it. | Driven outputs from other ICs (still prefer a defined idle). |
| Input pull-up / pull-down | Weak resistor to VDD or GND. | Buttons to GND; unused pins. |
| Push-pull output | Actively drives 0 and 1. | LED, SPI CS, enable pins. |
| Open-drain output | Drives 0 or Hi-Z. Needs a pull-up to make 1. | I²C, wired-OR fault lines, some reset pins. |
| Alternate function PP/OD | Peripheral controls the pin. | UART TX, PWM, SPI SCK. |
| Analog | Digital path largely off; analog mux may connect. | ADC in, DAC out, analog comparator. |

**Speed** (LOW/MEDIUM/HIGH/VERY_HIGH on many families) is **slew-rate / output capability**, not “how fast software can toggle.” Faster edges mean more ringing, EMI, and overshoot. Use the slowest speed that meets the rise-time budget on that trace.

## 3.3 Alternate functions

The datasheet AF table is the law: PA2 might be USART2_TX AF7 on one die and something else on another. CubeMX encodes this in `GPIO_InitStruct.Alternate`.

Only one AF is selected at a time. Remap exists on older F1 (AFIO MAPR); newer families use a per-pin AFR register.

**Package bonding:** a 48-pin LQFP does not bring out every die pad. CubeMX knows this if you selected the correct package.

## 3.4 EXTI vs GPIO

GPIO input **samples** a level when you read `IDR`. EXTI **detects edges** (rising, falling, both) and can interrupt. The pin must be configured as input (or AF, depending on family rules). You must:

1. Enable SYSCFG clock where required to map EXTI lines to ports.
2. Map EXTI line *n* to port Pxn (EXTI0 is pin 0 of **one** port).
3. Unmask the line, set edge, enable NVIC `EXTIx_IRQn`.
4. Clear the pending bit in the handler (HAL does this in `HAL_GPIO_EXTI_IRQHandler`).

Button bounce: a mechanical switch can fire tens of edges. Debounce in software (ignore N ms) or with an RC + Schmitt, not with a 200-line ISR.

## 3.5 Electrical ratings (read the datasheet)

- **VDD** range (often 1.7–3.6 V; many boards are 3.3 V).
- **FT** (five-volt tolerant) **digital** pins: may accept 5 V **when analog is off and injection rules are met**. Not all pins are FT. ADC pins used as analog are **not** a 5 V analog input.
- **Injection current** if a pin is driven above VDD or below VSS: can corrupt adjacent ADC readings or latch up.
- **Source/sink current** per pin and per package thermal budget. An LED with 330 Ω on 3.3 V is ~7 mA — usually OK. A motor is not.

Never drive a relay coil or DC motor from a GPIO. Use a MOSFET/driver, flyback diode, and a separate power path.

## 3.6 Voltage translation and grounds

UART to a 5 V Arduino: check FT, or use a shifter. SPI to a 1.8 V sensor at 3.3 V MCU: you can destroy the sensor.

Single-ended logic needs a **common ground**. A “TX-only” cable without GND will glitch. Long cables pick up noise; use twisted pair, lower slew, or a differential transceiver (RS-485) instead of hoping GPIO survives.

## 3.7 Smoke-test LED

```c
/* After MX_GPIO_Init(). Polarity: many Nucleo user LEDs are active-high,
   some boards are active-low. Read the schematic. */
HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

while (1) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(250); /* SysTick-based; blocking; jitter of ~1 ms; not PWM */
}
```

If this fails, do not debug DMA. Check: GPIO clock (`__HAL_RCC_GPIOx_CLK_ENABLE` in MSP), pin number vs silkscreen, solder bridges on Nucleo, reset pin held, code not stuck in `SystemClock_Config` `Error_Handler`.

`HAL_Delay` requires SysTick (or another timebase) configured by `HAL_Init` / HAL timebase. If you steal SysTick for FreeRTOS without giving HAL another timebase, delays and timeouts break.

---

# 4. Clock tree and clock calculations

*Almost every “wrong baud” and “timer is 2× off” bug is a clock-tree bug.*

## 4.1 Why clocks dominate firmware

The CPU instruction rate is HCLK (on typical trees). Peripheral baud and sample clocks are **derived**. If CubeMX thinks HSE = 8 MHz but the board feeds 25 MHz, PLL output is wrong, `SystemCoreClock` is wrong, UART divisor is wrong, and your delay loops are wrong — yet the program “runs.”

## 4.2 Sources

| Source | Role | Typical properties |
|--------|------|--------------------|
| HSI | Internal RC high-speed | Always there; several % error unless trimmed/calibrated. Fine for LED, poor for UART to a PC without auto-baud. |
| HSE | Crystal, resonator, or bypass clock | ppm accuracy if the crystal and load caps are correct. Bypass if ST-LINK provides MCO. |
| LSI | Internal low-speed RC | IWDG, sometimes RTC; very inaccurate. |
| LSE | 32.768 kHz crystal | RTC calendar. |
| PLL | Multiply/divide HSI or HSE | Must keep VCO and outputs in the RM allowed range. |
| Kernel clocks (newer families) | Per-peripheral mux | UART can run from PLLQ while CPU runs from PLLR. |

**CSS (clock security system)** can switch back to HSI if HSE fails. Handle the NMI/CSS interrupt if you enable it; otherwise you may appear to “randomly reset clocks.”

## 4.3 Bus tree (classic F1/F4-style)

```text
SYSCLK
  └── AHB prescaler → HCLK (CPU, AHB peripherals, memory)
        ├── APB1 prescaler → PCLK1 (low-speed peripherals)
        └── APB2 prescaler → PCLK2 (high-speed peripherals)
```

Flash **wait states** (latency) must match HCLK and VCORE. Too few wait states → random HardFaults or silent corruption. CubeMX sets `FLASH_LATENCY_x` in `SystemClock_Config`. If you hack the PLL by hand and forget latency, you will chase ghosts.

## 4.4 The timer-clock ×2 rule

On many STM32F1/F2/F3/F4/F7 trees:

- If APB prescaler = 1, timer clock = PCLKx.
- If APB prescaler > 1, timer clock = **2 × PCLKx**.

Example: SYSCLK = 84 MHz, AHB = 1, APB1 = 4 → PCLK1 = 21 MHz, **TIM2/3/4/5 clock = 42 MHz**, not 21 MHz.

If you compute PSC/ARR from 21 MHz, your PWM is twice as fast as you think. This is in the RCC chapter, not a forum myth. **Newer G0/G4/H7/U5** often use `TIMxSEL` kernel clocks — do not apply F4 folklore blindly. Read **your** clock tree figure.

## 4.5 Worked timer math

Edge-aligned up-counter:

\[
f_{\text{cnt}} = \frac{f_{\text{tim}}}{\text{PSC}+1},\quad
f_{\text{update}} = \frac{f_{\text{cnt}}}{\text{ARR}+1} = \frac{f_{\text{tim}}}{(\text{PSC}+1)(\text{ARR}+1)}
\]

Registers store **period minus one**. PSC = 0 means divide by 1.

**Example A.** \(f_{\text{tim}} = 72\,\text{MHz}\), want 1 kHz update.

Pick \(f_{\text{cnt}} = 1\,\text{MHz}\) → PSC+1 = 72 → **PSC = 71**.  
ARR+1 = 1000 → **ARR = 999**.  
Period = 1 ms. 16-bit timers: ARR ≤ 65535.

**Example B.** Need 50 Hz PWM (20 ms) for analog servo, 1 µs ticks (convenient 1000–2000 µs pulse).

\(f_{\text{cnt}} = 1\,\text{MHz}\), ARR+1 = 20000 → ARR = 19999. CCR = 1500 → 1.5 ms pulse (~center).

**Example C.** 32-bit TIM2 on F4 can count a 1 MHz tick for 4295 s. A 16-bit timer overflows in 65.5 ms at 1 MHz — too short for a 1 s period without prescaling.

## 4.6 Procedure

1. Schematic: who provides HSE? Load capacitors? Bypass?
2. CubeMX clock view: set HSE value to **reality**, not the tutorial.
3. PLL inputs/outputs inside RM min/max (VCO range is easy to violate).
4. AHB/APB, voltage scale, flash latency.
5. USB needs a precise 48 MHz on many parts — PLLQ or a dedicated 48 MHz RC (HSI48) with CRS.
6. Read `SystemClock_Config`. If `HAL_RCC_OscConfig` fails, you sit in `Error_Handler` before `main` loop. Debug that first.
7. Optionally output SYSCLK/HSE on MCO pin and measure with a frequency counter.

## 4.7 After every clock change

Recompute: UART BRR, I2C timing registers, SPI baud divider, ADC clock and sampling, timer PSC/ARR, SDMMC, USB. `HAL_RCC_GetHCLKFreq()` and family-specific `HAL_RCC_GetPCLK1Freq()` help, but only if `SystemCoreClock` was updated (`SystemCoreClockUpdate()`).

---

# 5. Peripheral configuration and initialization

*A peripheral configuration is a contract among silicon, wire, and firmware.*

## 5.1 The contract

| Area | You must match | If you do not |
|------|----------------|---------------|
| Pinout | Schematic net, AF, pulls, speed, PP vs OD | You debug software forever |
| Clock | Kernel or APB clock actually feeding the block | Baud/sample 15% off or peripheral dead |
| Mode | Master/slave, 8/16-bit, LSB/MSB, UART 8N1 vs 9E1 | Silent mismatch, “almost” works |
| Timing | Datasheet tSU, tHD, sample time, baud error < ~2% for UART | Intermittent CRC/NACK |
| NVIC | Vector, enable, priority vs other IRQs | Callback never runs |
| DMA | Request mux, direction, width, increment, circular | First sample only, or HardFault |
| Electrical | VDD of the sensor, CS idle, RESET released | NACK, MISO stuck |
| Errors | Timeouts, bus-error flags, DMA TE | Watchdog reset in the field |

## 5.2 Typical init order

CubeMX’s `main` often looks like:

```c
HAL_Init();                 /* Flash prefetch, SysTick 1 ms, NVIC group */
SystemClock_Config();
MX_GPIO_Init();
MX_DMA_Init();              /* often MUST be before peripherals that use DMA */
MX_USART2_UART_Init();
MX_ADC1_Init();
MX_TIM2_Init();
MX_FREERTOS_Init();         /* or osKernelStart */
```

**DMA init before the peripheral** is a classic CubeMX footgun if you reorder functions. The HAL UART/ADC init calls MSP which expects DMA handles already registered.

External devices:

1. Power valid (maybe a load switch).
2. Wait t_POR from the sensor datasheet (can be 50–100 ms).
3. Release RESET.
4. Optional I²C/SPI `WHO_AM_I`.
5. Write config registers.
6. Only then start continuous DMA.

If you `HAL_ADC_Start_DMA` before the application buffer and flags exist, the first callbacks race your constructors.

## 5.3 HAL status

```c
HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c1, addr, reg,
                                        I2C_MEMADD_SIZE_8BIT, &id, 1, 100);
if (st != HAL_OK) {
    uint32_t err = HAL_I2C_GetError(&hi2c1);
    sensor_online = false;
    /* id may be 0x00 leftover; do not publish it */
}
```

`100` is milliseconds **if** the HAL timebase runs. In an ISR, a 100 ms timeout is forbidden (blocking). In `main` before SysTick works, timeouts misbehave.

`HAL_BUSY` means the handle is still in a state machine (IT/DMA in progress). Retrying without aborting can livelock.

## 5.4 NVIC priority

On Cortex-M, **smaller number = higher urgency**. Priority grouping splits preemption vs subpriority. CubeMX defaults are often all equal — then tail-chaining order is hardware-defined, and a slow UART ISR can delay your timer trigger handler.

Do not set a priority numerically “more important” by increasing the number.

If you use FreeRTOS, ISR priorities that call `FromISR` APIs must be **at or below** `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (numerically greater than or equal, in ST/FreeRTOS numbering). Violating this is a famous HardFault.

---

# 6. HAL vs LL vs direct-register programming

*Abstraction is a tool. Ideology is not a timing requirement.*

## 6.1 Three layers

| Level | What you write | What you gain | What you pay |
|-------|----------------|---------------|--------------|
| HAL | `HAL_UART_Receive_IT` | Portable-ish, state, callbacks | Flash size, call overhead, hidden flags |
| LL | `LL_USART_TransmitData8` | Close to RM, still generated | You own state machines |
| Registers | `USART2->TDR = c` | Exact control, education | Family-specific, easy to miss a bit |

HAL maintains `hxyz.State`, `Lock`, and error codes. If you poke `USART2->CR1` while HAL thinks a transfer is ongoing, the next `HAL_UART_Receive_IT` returns `HAL_BUSY` forever.

LL does not own that state. Mixing LL and HAL on the **same** peripheral instance is allowed only if you understand both.

## 6.2 When HAL is the right default

Student labs, application firmware, USB/FatFS stacks ST ships as HAL-centric, anything where the peripheral analog time (ADC conversion, UART bits) dwarfs a few extra function calls.

Measure: toggle a GPIO around `HAL_SPI_Transmit`. If 8 µs overhead on a 10 ms transaction, stop optimizing.

## 6.3 When to go lower

- Cycle-counted bit banging (usually you should not).
- A mode HAL does not expose (rare, but dual-ADC tight alignment, exotic timer combined modes).
- ISR that must be a few cycles (often LL flag clear + buffer store).
- Bringing up a new family by reading the RM with a debugger on SFRs.

## 6.4 Register hygiene

- **Reserved bits:** write 0 unless the RM says otherwise.
- **Write-1-to-clear:** `SR = USART_SR_RXNE` may clear **all** w1c bits if you assign the whole register on some peripherals. Prefer `USART2->ICR = USART_ICR_ORECF` style (family-specific).
- **Read-modify-write:** `CR1 |= bit` in thread and ISR races. Use critical sections or bit-band/atomic where available.
- **Clock gating:** writing a peripheral that is not clocked HardFaults (bus fault) on many designs.

## 6.5 Recommended learning path

1. CubeMX + HAL minimal example works on **your** board.
2. Step into HAL with the debugger; watch registers change.
3. Read the RM chapter for those registers.
4. Profile; optimize a measured hot path behind a 20-line API.
5. Keep HAL for USB and the rest of the chip if it is already working.

---

# 7. Polling, interrupts, callbacks, and DMA

*Three ways to wait, with three different failure modes.*

## 7.1 Polling

```c
if (HAL_UART_Receive(&huart2, buf, 1, 10) == HAL_OK) {
    /* got a byte, or timed out after 10 ms of blocking */
}
```

The CPU is occupied. Response latency is “whenever you next poll.” Perfect for:

- `WHO_AM_I` during init
- 1 Hz temperature
- Scripts and labs

Terrible for: 115200 burst RX while also running a 10 kHz control loop in the same thread without a state machine.

## 7.2 Interrupts

Hardware sets a flag; NVIC vectors to `USARTx_IRQHandler`, which ST writes as:

```c
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
```

HAL reads SR/ISR, copies data, and calls `HAL_UART_RxCpltCallback` in **ISR context**.

**ISR rules:**

- Keep it short: set a flag, push to a queue (`FromISR`), copy one byte into a ring buffer.
- No `printf`, no `malloc`, no `HAL_Delay`, no blocking `HAL_I2C_Master_Transmit` with 100 ms timeout.
- Clear the interrupt source (HAL usually does).
- Watch reentrancy: a callback that starts another IT on the same handle is a common HAL pattern (re-arm RX) but must not nest unbounded work.

**Latency budget:** IRQ tail-chain on Cortex-M is short (a dozen cycles) if nothing equal/higher is running. A 50 µs flash wait or a 200 µs other ISR blows a 10 µs SPI CS timing.

## 7.3 `volatile` and atomics

```c
static volatile uint8_t sample_ready;
```

`volatile` means: the compiler must not cache this in a register across loops. It does **not** mean:

- 32-bit access is atomic on all cores (M0 vs M3/M4 32-bit aligned is usually atomic for load/store)
- `sample_ready++` is atomic
- DMA will not tear a 16-bit value
- you have a memory barrier vs DMA/cache on M7

For a flag between ISR and main on M3/M4, `uint8_t` volatile load/store is the usual pattern. For a 64-bit timestamp, disable IRQ or use a properly ordered double-read.

## 7.4 DMA

DMA is a second master on the bus. You program: source, dest, length, widths, increment, circular/normal, request handshake with a peripheral.

**CPU still must:**

- Configure peripheral to issue requests (ADC EOC, UART RXNE)
- Enable NVIC for DMA HT/TC/TE if you use callbacks
- Own the buffer until TC (normal mode) or until you have processed a half (circular)

| Method | CPU role | Typical failure |
|--------|----------|-----------------|
| Polling | Spins on a flag | Misses real-time work |
| Interrupt | Runs ISR per event | ISR too fat; priority inversion |
| DMA | Setup + completion | Wrong channel; width; cache; lifetime |

## 7.5 Choosing a path

| Requirement | Starting point |
|-------------|----------------|
| Button | EXTI; debounce in main |
| 1 Hz I²C sensor | Blocking HAL + timeout |
| Command console | RX IT or DMA-to-idle + parser in main |
| 10 kS/s ADC | Timer TRGO + ADC + circular DMA |
| 4096-byte SPI flash page | DMA if CPU has better work |
| 20 kHz PWM | Timer hardware; not bit-bang |

## 7.6 DMA family notes

- **F1:** DMA channels, fixed peripheral mapping.
- **F4:** DMA streams + channels; more flexible but easier to mis-click in CubeMX.
- **G4/H7/U5/C0:** DMAMUX routes almost any request to any channel.

CubeMX must match the RM “DMA request mapping” table. ADC1 on the wrong stream is the “callback never fires” bug.

---

# 8. Timers and deterministic timing

*A timer is a hardware counter with compare, capture, and trigger logic. It is not `HAL_Delay`.*

## 8.1 Register mental model

| Element | Role |
|---------|------|
| PSC | Input clock divider minus one |
| CNT | Live counter |
| ARR | Auto-reload (period minus one in up mode) |
| CR1 DIR/CMS | Up, down, center-aligned |
| CCR1..4 | Compare (PWM) or capture value |
| DIER | Interrupt/DMA on update, CC, trigger |
| SMCR / CR2 MMS | Slave mode, TRGO generation for ADC |

**Update event (UEV)** fires when the counter wraps (and when you generate UG). That event can toggle a pin (PWM), trigger ADC, request DMA, or interrupt.

## 8.2 PWM duty

Edge-aligned PWM1, active high, typical:

\[
\text{duty} \approx \frac{\text{CCR}}{\text{ARR}+1}
\]

CCR = 0 → 0% (or a sliver, depending on family and preload). CCR = ARR+1 may be 100%. Center-aligned modes count up and down; frequency halves relative to a naive ARR formula. **Always** check RM PWM mode 1 vs 2 and measure.

Preload (`ARPE`, `OCxPE`) means ARR/CCR take effect at the next update — you avoid glitches mid-period. For a servo pulse change, preload is what you want.

## 8.3 Input capture

On an edge, hardware copies CNT into CCR. Two captures give a period. Use timer overflow interrupts to extend 16-bit capture to 32-bit software time. Watch overflow-between-edges.

Encoder mode uses two channels in quadrature — a hardware feature, not GPIO polling.

## 8.4 One-pulse, output compare, TRGO

- **Output compare:** IRQ or pin toggle at CCR match — precise scheduler for “do this in 350 µs.”
- **One-pulse:** delayed pulse after a trigger (for example, ADC after a motor PWM edge, avoiding switching noise).
- **TRGO:** `MMS` in CR2 selects reset, enable, update, CC1, etc. ADC `ExternalTrigConv` must list **that** timer’s TRGO. CubeMX dropdowns are family-specific; a TIM3 TRGO that does not exist on ADC1 will never sample.

## 8.5 Starting the timer

```c
if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
    Error_Handler();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        tick_count++; /* microseconds of work, not milliseconds */
    }
}
```

PWM: `HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1)` — base start is **not** enough for a channel output.

Pitfalls: wrong `f_tim`, forgot `+1`, IRQ not enabled, callback filters the wrong `Instance`, 16-bit overflow, using `HAL_Delay` for a 20 kHz loop.

---

# 9. Timer-triggered ADC + DMA

*This is the backbone of data acquisition: timebase → convert → store → process.*

## 9.1 Why three blocks

Software-triggered ADC in a loop has jitter (ISR, flash wait, branches). A timer TRGO starts each conversion at a hardware interval. DMA stores `DR` into RAM so the CPU is not copying every sample.

```text
TIM update / TRGO  →  ADC start  →  EOC  →  DMA request  →  SRAM buffer
                                              └→ HT / TC IRQ  →  software
```

## 9.2 Configuration sequence (do not skip)

1. Pin analog, channel number matches CubeMX **and** the pad.
2. Resolution 12/10/8-bit; right or left aligned (HAL usually right-aligned in the 16-bit buffer).
3. Sampling time vs source impedance (chapter 14).
4. Scan vs single; discontinuous; number of conversions = DMA length semantics (some ADCs pack multi-channel into one DMA sequence).
5. Trigger: rising edge of TIMx TRGO, **not** software start for each sample.
6. ADC clock prescaler within RM max (too fast → linearity death).
7. DMA: peripheral-to-memory, no periph increment, mem increment, half-word for 12-bit in `uint16_t[]`, circular for continuous.
8. NVIC: ADC error optional; DMA HT/TC for ping-pong.
9. Start **order:** often `HAL_ADC_Start_DMA` then `HAL_TIM_Base_Start`. Reverse can miss the first edges or fault. Confirm RM/app note for your ADC.
10. Calibrate first on families that require `HAL_ADCEx_Calibration_Start` (STM32F0/F3/G0/G4/H7/L4/U5, etc.).

## 9.3 Buffer deadline (the law of circular DMA)

Buffer `N` samples, rate \(F_s\):

\[
T_{\text{full}} = \frac{N}{F_s},\quad T_{\text{half}} = \frac{N}{2F_s}
\]

**Example.** \(F_s = 10\,\text{kHz}\), \(N = 200\) → 20 ms full, 10 ms half. You have **10 ms worst-case** to process 100 samples **and** leave margin for other ISRs.

If processing is 12 ms, you will silently mix old and new samples. Count `overrun` when a half flag is still set when the next half arrives.

```c
#define ADC_BUFFER_SAMPLES 200

static uint16_t adc_buffer[ADC_BUFFER_SAMPLES];
static volatile uint8_t adc_half_ready;
static volatile uint8_t adc_full_ready;
static uint32_t adc_overruns;

/* Start after MX_ADC1_Init, MX_DMA_Init, MX_TIM2_Init */
if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer,
                      ADC_BUFFER_SAMPLES) != HAL_OK) {
    Error_Handler();
}
if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
    Error_Handler();
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (adc_half_ready) {
            adc_overruns++;
        }
        adc_half_ready = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        if (adc_full_ready) {
            adc_overruns++;
        }
        adc_full_ready = 1;
    }
}
```

In `main`:

```c
if (adc_half_ready) {
    adc_half_ready = 0;
    process(&adc_buffer[0], ADC_BUFFER_SAMPLES / 2);
}
if (adc_full_ready) {
    adc_full_ready = 0;
    process(&adc_buffer[ADC_BUFFER_SAMPLES / 2], ADC_BUFFER_SAMPLES / 2);
}
```

Process **copies or reduces** data; do not run a UART dump of every sample at 10 kHz on 115200 baud.

## 9.4 Code to voltage

\[
V_{\text{in}} \approx \text{code} \times \frac{V_{\text{ref}}}{2^{N}-1}
\]

12-bit, VREF = 3.3 V, code 2048 → ~1.650 V. Some app notes use \(2^{N}\) as full scale (code 4096 would be VREF, but max code is 4095). For precision, use factory calibration (`VREFINT_CAL`) and the RM transfer curve.

## 9.5 Cache (Cortex-M7 / STM32H7 and similar)

DMA writes SRAM that the D-cache may still consider clean/old. Symptoms: first packet OK, then stale blocks, or OK with debugger (cache disabled in some debug scripts).

Fixes (pick the documented one):

- Place the buffer in a non-cacheable MPU region or `DMA` section in RAM_D2/D3 as ST’s examples do.
- `SCB_InvalidateDCache_by_Addr` before CPU reads DMA data (alignment 32-byte).
- Do not ignore this on H7.

## 9.6 Validation

- Ground the pin: codes near 0 (offset error remains).
- 1.65 V from a divider: codes near mid-scale.
- Scope the timer TRGO pin (if you output a compare) vs ADC sampling — or toggle GPIO in the half callback and measure period = \(N/(2F_s)\).
- Ramp sample rate until `adc_overruns` increments; that is your CPU budget.

---

# 10. UART: framing and robust streams

*UART is a shift register plus a baud clock. It has no files, no lines, and no sockets.*

## 10.1 The frame

Common 8-N-1:

```text
idle=1 | START=0 | b0 | b1 | … | b7 | STOP=1 | (next start or idle)
```

10 bit-times per byte. Throughput:

\[
\text{bytes/s} \approx \frac{\text{baud}}{10}
\]

115200 → 11520 bytes/s ≈ 11.25 KiB/s **ideal**, before your `\r\n` and JSON.

Bit time at 115200 ≈ 8.68 µs. If a scope shows ~9.6 µs, you are at 10400 baud (classic 8 MHz vs 7.37 / wrong HSE).

**Baud error:** BRR is an integer (with oversampling 16 or 8). Error should stay well under 2–3% vs the peer. HSI without trim can exceed that.

## 10.2 Wiring

- TX → RX, RX → TX, **GND**.
- MCU UART is **LVCMOS**, not RS-232 (±6 V). A PC COM port needs a transceiver or a USB-UART dongle.
- Nucleo VCP: ST-LINK virtual COM; pins are documented as USART2 on many F4 Nucleo boards — **not all**.
- Flow control RTS/CTS if the peer can stall; otherwise drop bytes.

## 10.3 Transmit

```c
const uint8_t msg[] = "STM32 ready\r\n";
HAL_UART_Transmit(&huart2, (uint8_t *)msg, sizeof(msg) - 1, 100);
```

Blocking. Fine for a banner. In a 1 kHz control loop, blocking 1 ms for 13 characters at 115200 (~1.1 ms) **overruns** the loop.

`HAL_UART_Transmit_IT` / `_DMA`: buffer must remain valid until `TxCpltCallback`. Never pass a stack array.

## 10.4 Receive is a byte stream

There is no “message.” Bytes arrive whenever. You impose structure:

| Framing | Pros | Cons |
|---------|------|------|
| `\n`-terminated text | Easy to type in a terminal | Binary 0x0A in data; overflow lines |
| Fixed length | Simple | Inflexible |
| Length prefix | Binary-friendly | Desync needs resync strategy |
| COBS / SLIP | Robust binary | More code |
| Idle-line (RXFT / IDLE IRQ) | DMA-friendly packets | Idle time is a timeout, not a CRC |

Byte-by-byte IT (teaching pattern):

```c
static uint8_t rx_byte;
static char line[64];
static size_t line_len;
static volatile uint8_t command_ready;
static uint32_t rx_overflow_count;

void start_rx(void)
{
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    char c = (char)rx_byte;
    if (c == '\n') {
        line[line_len] = '\0';
        command_ready = 1;
        line_len = 0;
    } else if (c != '\r') {
        if (line_len < sizeof(line) - 1) {
            line[line_len++] = c;
        } else {
            line_len = 0;
            rx_overflow_count++;
        }
    }
    (void)HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}
```

**Race:** `main` must copy `line` before the next `\n`, or use a double buffer. This ISR must not parse JSON.

**Production RX:** circular DMA + `HAL_UARTEx_ReceiveToIdle_DMA` (where available) + parse in thread. Re-arm rules differ by HAL version — read the HAL comment for your package.

## 10.5 Errors

- **Overrun (ORE):** CPU/DMA did not read DR before the next byte. You lose stream sync.
- **Framing (FE):** stop bit wrong — baud or noise.
- **Noise (NE):** sampling disagreed.
- **Parity (PE):** if enabled.

HAL error callbacks must clear ORE or RX dies forever on some families.

---

# 11. SPI: modes, chip select, and timing

*SPI is a shared shift register. The clock is the truth; MOSI/MISO are just companions.*

## 11.1 Signals

| Signal | Role |
|--------|------|
| SCK | Master-generated clock |
| MOSI | Master → slave |
| MISO | Slave → master (high-Z when unselected — needs pull if you care about idle) |
| CS/NSS | Usually active-low; one per slave |
| GND | Required |

Multi-slave: either separate CS lines, or daisy-chain (rare for sensors).

## 11.2 CPOL and CPHA (modes 0–3)

| Mode | CPOL | CPHA | Idle clock | Sample |
|------|------|------|------------|--------|
| 0 | 0 | 0 | Low | First edge (rising) |
| 1 | 0 | 1 | Low | Second edge (falling) |
| 2 | 1 | 0 | High | First edge (falling) |
| 3 | 1 | 1 | High | Second edge (rising) |

**The sensor datasheet timing diagram wins.** “Mode 0” in one PDF is sometimes drawn as mode 3. If you read `0xFF` or `0x00`, try the other CPHA before rewriting the driver.

MSB vs LSB, 8 vs 16-bit frames: an IMU that wants 16-bit transfers will look “byte swapped” if you send two 8-bit frames with CS high in between.

## 11.3 Full duplex and dummy clocks

Every SCK edge shifts **both** directions. To read a register you still **write** something (command + dummy).

```c
uint8_t tx[2] = { READ_COMMAND, 0x00 };
uint8_t rx[2] = { 0, 0 };

HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);
HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

if (st != HAL_OK) {
    /* timeout, MODF, overrun */
}
```

Hardware NSS often pulses per byte — **wrong** for devices that want CS held across command+payload. **Software CS** (GPIO) is the default professional choice.

## 11.4 Timing budget

Start at a low SCK (e.g. 1 MHz). Check t_CSS (CS setup), t_CSH, t_CLK high/low, MISO delay vs your mode. Long wires + 20 MHz = ringing; series 22–33 Ω on SCK helps.

Logic analyzer: **four channels** CS, SCK, MOSI, MISO. If MISO changes while CS is high, you have a bus fight.

DMA: useful for displays and flash pages. CS must stay low for the whole DMA; complete callback raises CS.

---

# 12. I²C: addressing, pull-ups, and recovery

*Two open-drain wires. Electrically simple, operationally the fussiest bus in student labs.*

## 12.1 Open-drain physics

Devices only **sink** current. Pull-ups create logic 1. Rise time:

\[
t_r \approx 0.8 R_{\text{pu}} C_{\text{bus}}
\]

I²C Fast-mode (400 kHz) has a max rise time (typically 300 ns in the spec). If \(C_{\text{bus}}\) is 200 pF (long jumper spaghetti), \(R_{\text{pu}} = 10\,\text{k}\Omega\) is too weak. 2.2 kΩ is a common 3.3 V starting point; **measure** SCL with a scope.

Both ends must share GND and the same VDD logic (or a level translator). Mixing 5 V pull-ups into a non-FT 3.3 V pin is a reliability crime.

## 12.2 Addressing

7-bit address `0x48` in the datasheet. The wire sends 7 bits + R/W.

**STM32 HAL:** `DevAddress` is typically `0x48 << 1` (`0x90`). Shift twice and you address the wrong device (NACK). 10-bit addressing is a different API path.

Some vendors write “address 0x90” meaning the 8-bit write address. Always state “7-bit 0x48” in your header.

```c
#define SENSOR_ADDR_7BIT  0x48u
#define SENSOR_ADDR_HAL   (SENSOR_ADDR_7BIT << 1)

uint8_t value = 0;
HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
    &hi2c1, SENSOR_ADDR_HAL, REG_STATUS,
    I2C_MEMADD_SIZE_8BIT, &value, 1, 100);

if (st != HAL_OK) {
    uint32_t err = HAL_I2C_GetError(&hi2c1);
    (void)err; /* AF = NACK, timeout, bus error, size */
}
```

`Mem_Read` generates START, device write, register, RESTART, device read, data, STOP — which many sensors require. Two separate transmit/receive calls without a restart can fail those parts.

## 12.3 Symptom table

| Symptom | Checks |
|---------|--------|
| AF / NACK | Address shift, unpowered, WRONG WHOAMI, RESET low, 5 V vs 3.3 V, another master |
| SDA stuck low | Slave holding ACK, CPU reset mid-byte, short to GND |
| SCL stuck low | Clock stretch forever, slave crashed, short |
| Intermittent | Rise time, 400 kHz too fast, breadboard C, missing GND |
| Wrong data | Endian, 16-bit register, not using repeated start, auto-increment off |
| HAL BUSY forever | Previous transfer aborted badly; call `HAL_I2C_DeInit` / abort |

## 12.4 Bus recovery (concept)

If SDA is stuck low, the I²C spec allows the master to toggle SCL up to 9 times until SDA releases, then a STOP. Implementation: reconfigure pins as GPIO OD, bit-bang, restore AF. **Only** if the RM and board allow those pins as GPIO. This can confuse devices mid-EEPROM write — use as a last resort and then re-init the sensor.

STM32 analog filters and digital filters (`ANFOFF`, DNF) exist on newer I2C IP; CubeMX timing wizard computes `TIMINGR` from kernel clock. If you change I2C kernel clock, **recompute** timing; do not copy `0x10909CEC` from a blog.

---

# 13. Choosing a communication interface

*The sensor datasheet chooses the interface more often than you do.*

## 13.1 Comparison

| Interface | Wires | Strength | Weakness | Throughput intuition |
|-----------|-------|----------|----------|----------------------|
| UART | 2 + GND | Human logs, GPS, BLE modules | Point-to-point, no addressing | baud/10 bytes/s |
| SPI | 4+ GND | Fast, simple silicon | CS per device, more pins | ~SCK/8 bytes/s minus gaps |
| I²C | 2 + GND | Many devices, cheap sensors | Pull-ups, capacitance, NACK mysteries | Far below SCL/9 due to addr/ack |
| USB | Transceiver | PC native | Stack, enumerations, timing of 48 MHz | High but software-heavy |
| CAN | Transceiver | Multi-node, industrial noise | Transceiver + IDs + filters | 1 Mbit typical classic CAN |
| RS-485 | Transceiver on UART | Long cable, differential | Direction DE pin, protocol | Same as UART baud |

## 13.2 Decision sequence

1. What does the part support? (You cannot I²C a part that is SPI-only.)
2. Burst size and latency (IMU at 1 kHz vs config EEPROM once).
3. Pin count and whether devices share a bus.
4. Cable length and EMI (robot chassis vs 3 cm PCB).
5. Debug tools you actually have (logic analyzer with I²C decode saves days).
6. Start slow, verify ACK/WHOAMI, then raise speed.

**Clock is not payload.** 400 kHz I²C reading a 2-byte register with address overhead might yield a few kHz of samples, not 50 kHz.

---

# 14. ADC, analog front ends, and accuracy

*The ADC is a comparator-based (usually SAR) machine that outputs an integer. Physics is your problem.*

## 14.1 Vocabulary

| Term | Meaning |
|------|---------|
| Resolution | 12-bit → 4096 codes (0–4095) |
| LSB size | Often \(V_{\text{REF}}/4096\) **or** \(V_{\text{REF}}/4095\); be consistent with RM |
| Accuracy | Error vs true voltage: offset, gain, INL, DNL, VREF error, analog front end |
| Repeatability / noise | Std dev of a DC reading; averaging helps **this**, not a 5% gain error |
| ENOB | Effective bits after noise and distortion |
| VREF+ | Full-scale analog reference. If VREF = VDD, supply ripple **is** your measurement error |

A 12-bit converter on a 5% unregulated USB 5 V → 3.3 V LDO still might have **tens of millivolts** of uncertainty. Do not quote 0.8 mV precision because \(3.3/4096 \approx 0.8\,\text{mV}\).

## 14.2 Sampling capacitor and impedance

SAR ADCs sample onto \(C_{\text{in}}\) (a few pF). Time constant \(R_{\text{source}} C_{\text{in}}\) must settle within the programmed **sampling time**.

High \(R_{\text{source}}\) (100 kΩ divider, unbuffered) → the code depends on the **previous channel** (crosstalk) and reads low.

Fixes: longer sample time, lower source R, op-amp buffer, or a large C at the pin **with** understanding of charge sharing (sometimes used, sometimes worse).

## 14.3 Front ends

**Divider:**  
\[
V_{\text{node}} = V_{\text{in}} \frac{R_b}{R_t + R_b}
\]
Tolerance stack: 1% resistors → ~1% ratio error before the ADC speaks. Loading: ADC input current and following stages.

**Thermistor:** nonlinear. Use Steinhart–Hart or a calibrated lookup. Do not pretend it is linear near one point unless the range is tiny.

**Anti-alias filter:** analog RC/R-C-R before the pin. Digital filters **cannot** remove aliases already folded into the band.

Nyquist: content above \(F_s/2\) folds. 50 Hz mains can appear as a slow wander if you sample 10 Hz without analog filtering.

## 14.4 Internal channels

VREFINT, temperature sensor, VBAT: enable analog paths, wait tS, use the calibration constants in system memory as the RM describes. These are slow and noisy; they are not a lab-grade thermometer.

## 14.5 Safe bring-up

1. DMM on the **MCU pin**, not only on the sensor module pin (series resistors lie).
2. Polling ADC, long sample time, print raw codes.
3. Three voltages: 0, mid, near VREF (stay within abs max).
4. Plot code vs DMM; estimate offset/gain.
5. Then DMA, then filters, then “temperature.”

---

# 15. Sampling, throughput, and data integrity

*A stream is a contract: rate in, bytes stored, deadline out, policy on overflow.*

## 15.1 Numbers you should compute on paper

\[
T_s = \frac{1}{F_s}
\]

10 kS/s → 100 µs per sample.  
Buffer duration \(N/F_s\). 1024 samples at 8 kS/s → 128 ms of history.

Bandwidth:

\[
B = F_s \times \text{bytes/sample} \times N_{\text{ch}}
\]

4 ch × 20 kS/s × 2 B = 160 kB/s raw. UART 115200 cannot carry it. USB FS (theoretical ~1.2 MB/s payload, real less) might. SPI flash logging might. SD cards have latency spikes — they are not magically real-time.

## 15.2 Buffer strategies

| Strategy | Idea | Failure mode |
|----------|------|--------------|
| Single buffer | Fill, stop, process | Cannot be continuous |
| Ping-pong / DMA HT-TC | Process half while DMA fills the other | Process too slow |
| Byte ring + indices | UART RX | Index tearing if not ISR-safe |
| Queue of blocks | RTOS | Queue full policy undefined → deadlock or silent drop |
| Copy out | Extra RAM/CPU | Simplifies ownership |

**Define overflow:** drop newest (keep history), drop oldest (keep latest), pause producer, or enter fail-safe. **Count it.**

## 15.3 Cache and alignment

M7: 32-byte cache lines. DMA buffer alignment matters for clean/invalidate. M4 without D-cache: much simpler.

DMA length vs peripheral data size: transferring 200 ADC half-words is `Length = 200` in HAL for many ADCs, not 400 bytes — **read the HAL docstring** for that function.

## 15.4 Integrity extras

- Sequence numbers in telemetry so the PC detects drops.
- CRC on stored blocks.
- Never use uninitialized `id` after NACK.
- Timestamp from a free-running timer (CNT), not only `HAL_GetTick()` (1 ms granularity).

---

# 16. Reusable sensor drivers

*Hide registers. Do not hide errors.*

## 16.1 Layers

| Layer | Owns | Must not own |
|-------|------|----------------|
| Bus | Start/stop, timeouts, DMA optional | “What is overheating” |
| Sensor driver | WHOAMI, endian, LSB scale, config sequence | Global `hadc1` baked in without injection |
| Acquisition | Rate, buffers, health flags | Copy-pasted register maps |
| Application | Alarms, UI, networks | `I2C1->CR1` |

Dependency injection:

```c
typedef struct {
    I2C_HandleTypeDef *i2c;
    uint16_t address_hal;
} TempSensor;
```

Two sensors on one bus = two structs, one handle, **mutex** if RTOS.

## 16.2 API rules

- Status enum, not `bool` (you need BUS vs ID vs ARG).
- Output pointer written **only** on success.
- Units in the name: `ReadCelsius`, `temperature_c`.
- Bounded timeouts.
- No `malloc` in the driver unless the project already decided a heap policy.
- Header documents 7-bit vs HAL address.

```c
typedef enum {
    TEMP_OK = 0,
    TEMP_ERR_ARGUMENT,
    TEMP_ERR_BUS,
    TEMP_ERR_ID,
    TEMP_ERR_CONFIG
} TempStatus;

TempStatus TempSensor_Init(TempSensor *dev);
TempStatus TempSensor_ReadCelsius(TempSensor *dev, float *temperature_c);

TempStatus TempSensor_ReadCelsius(TempSensor *dev, float *temperature_c)
{
    uint8_t b[2];

    if (dev == NULL || dev->i2c == NULL || temperature_c == NULL) {
        return TEMP_ERR_ARGUMENT;
    }

    if (HAL_I2C_Mem_Read(dev->i2c, dev->address_hal, TEMP_REG_MSB,
                         I2C_MEMADD_SIZE_8BIT, b, 2, 100) != HAL_OK) {
        return TEMP_ERR_BUS;
    }

    int16_t raw = (int16_t)(((uint16_t)b[0] << 8) | b[1]);
    *temperature_c = (float)raw * TEMP_LSB_C;
    return TEMP_OK;
}
```

`TEMP_LSB_C` comes from the datasheet, not from hope. Sign extension for 12-bit-in-16 values needs a shift, not a naive cast, when the format is left-aligned.

## 16.3 Tests without the full robot

- NULL arguments.
- Fake I2C that NACKs.
- Byte order `{0x80, 0x00}` → negative temperature if two’s complement.
- Init fails if WHOAMI ≠ expected.
- Application still compiles if you stub the driver.

---

# 17. Bare-metal superloops and state machines

*An RTOS is not a moral upgrade. A blocking `HAL_Delay(1000)` in the only loop is a moral downgrade.*

## 17.1 A honest main loop

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    App_Init();

    while (1) {
        App_ProcessCommands();     /* bounded: parse at most one line */
        App_ProcessSensorResults();
        App_UpdateStateMachine();
        App_ServiceTelemetry();
    }
}
```

Each function has a **worst-case execution time**. Sum them; add ISR load; compare to the tightest period you care about.

## 17.2 State machines beat delays

Blocking:

```c
HAL_I2C_Mem_Read(...); /* 1 ms */
HAL_Delay(10);
```

Non-blocking:

```text
IDLE → START_WRITE → WAIT_IT_DONE → WAIT_SENSOR_MS → READ → PUBLISH
         ↘ ERROR → backoff → IDLE
```

Use `HAL_GetTick()` differences for the 10 ms wait **without** parking the CPU. UART and buttons still run.

## 17.3 When the superloop is enough

Few rates, all harmonic or slow, no USB stack demanding its own thread, you can still meet deadlines with ISRs + flags.

## 17.4 When it starts to hurt

Independent 1 kHz control, 100 Hz telemetry, blocking file system, USB, and a UI. Then tasks + queues express the design more clearly — if you budget stacks and priorities.

---

# 18. FreeRTOS fundamentals and patterns

*The scheduler multiplexes the CPU. It does not invent extra MHz or fix a 20 ms I²C timeout in a 1 ms task.*

## 18.1 Objects

| Object | Use |
|--------|-----|
| Task | A loop with a stack and priority |
| Queue | Copy N bytes to a waiter (or zero-copy patterns in newer APIs) |
| Semaphore | Event or resource count |
| Mutex | Mutual exclusion **with priority inheritance** (in FreeRTOS mutexes) |
| Task notify | Fast “one event / one value” to a specific task |
| Event group | Many boolean flags |

ISR: `xQueueSendFromISR`, never `xSemaphoreTake` on a mutex, never `vTaskDelay`.

## 18.2 Priorities and starvation

Higher priority **always** runs if ready (preemptive config). A high-priority task in a tight loop without delay or wait **starves** the rest, including idle (which may service the heap).

Rate monotonic intuition: faster deadlines → higher priority, **if** they block on waits. A fast task that also does flash erase will still wreck a slow task.

## 18.3 Producer / consumer

```c
typedef struct {
    uint32_t timestamp_ms;
    int16_t temperature_centi_c; /* 2501 => 25.01 °C, no float in the queue */
    uint16_t flags;
} SensorSample;

void SensorTask(void *argument)
{
    SensorSample sample;
    (void)argument;
    for (;;) {
        if (Sensor_ReadSample(&sample) == SENSOR_OK) {
            if (xQueueSend(sensor_queue, &sample, pdMS_TO_TICKS(10)) != pdPASS) {
                dropped_samples++; /* full-queue policy: drop newest after 10 ms */
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void TelemetryTask(void *argument)
{
    SensorSample sample;
    (void)argument;
    for (;;) {
        if (xQueueReceive(sensor_queue, &sample, portMAX_DELAY) == pdPASS) {
            Telemetry_SendSample(&sample);
        }
    }
}
```

Fixed-point in the queue avoids float in ISRs and saves RAM. `vTaskDelay(20)` is period **plus** run time unless you use `vTaskDelayUntil` for a stable period.

## 18.4 Shared I²C

Two tasks, one bus: a mutex around all transactions. Take mutex, entire write-read, give mutex. Do not hold it during a 200 ms sensor conversion wait if you can avoid it (release, delay, retake).

## 18.5 Stacks and heap

`configMINIMAL_STACK_SIZE` is not enough for `printf` float. Use watermark `uxTaskGetStackHighWaterMark`. Overflow → silent corruption → HardFault later.

FreeRTOS heap (`heap_4`) can fragment. Prefer static task/queue allocation (`xTaskCreateStatic`) in safety-conscious firmware.

## 18.6 HAL + RTOS

- `HAL_Delay` in a task blocks **that task** only if HAL timebase is SysTick **and** SysTick still runs. ST’s recommended approach: TIM timebase for HAL, SysTick for FreeRTOS.
- `osDelay` vs busy wait.
- CubeMX “FREERTOS” middleware generates `cmsis_os` wrappers; know whether you call CMSIS-RTOS or native API.

## 18.7 Justification table

| Good reason | Poor reason |
|-------------|-------------|
| Independent deadlines and blocking IO | “Professionals always use RTOS” |
| USB + control + logging | Wrapping `HAL_Delay` in a task to hide it |
| Vendor middleware assumes CMSIS-RTOS | One task per function (death by stacks) |

---

# 19. Debugging playbook

*Change one variable. Observe a fact. The chip is not offended by a scope probe.*

## 19.1 Layered attack

1. **Power:** 3.3 V, 5 V, GND continuity, reset pulled, BOOT0, supply current sane.
2. **Clocks:** HSE present, `SystemCoreClock`, MCO, CSS.
3. **Pins:** AF vs schematic, solder jumpers, level.
4. **Peripheral registers:** EN bit, baud BRR, ADC CHSEL, DMA EN.
5. **Software:** framing, units, races, timeouts.

## 19.2 Symptom → first checks

| Symptom | First checks |
|---------|--------------|
| Never `main` | Reset vector, `SystemInit`, clock fail `Error_Handler`, watchdog, BOOT pins, debugger “run to main” broken |
| LED dead | Pin, polarity, GPIOx clock, stuck before loop |
| UART garbage | Baud vs measured bit time, TX/RX swap, GND, VCP vs shield USART |
| I²C NACK | 7-bit shift, power, pull-ups, RESET, WHOAMI |
| SPI 0x00/0xFF | CS, mode, SCK idle, disconnected MISO, 3.3 vs 1.8 V |
| ADC stuck 0 / 4095 | Analog mode, channel, VREF, pin short, alignment |
| No DMA callback | Stream/channel, NVIC, length 0, peripheral not requesting, HT vs TC |
| Timer 2× fast | APB ×2 rule |
| Works only in debug | Race; uninitialized; IWDG stopped in debug (`DBGMCU`); extra delays from breakpoints |
| Random HardFault | Stack overflow, execute of 0xFFFFFFFF, unaligned access on M0, peripheral unclocked |

## 19.3 HardFault on Cortex-M

When halted in `HardFault_Handler`:

- Note stacked PC/LR from the fault stack frame.
- `CFSR` (Configurable Fault Status): IACCVIOL, DACCVIOL, UNDEFINSTR, UNALIGNED, IMPRECISERR.
- `BFAR`/`MMFAR` if valid bits set.

Imprecise bus faults on M3/M4 can make BFAR useless — look at recent peripheral accesses.

## 19.4 Tools

- **SWD debugger:** live expressions, SFR view, SWV/ITM printf (needs SWO pin and clock).
- **Logic analyzer:** decode UART/SPI/I2C; **measure** clock stretching and CS gaps.
- **Oscilloscope:** analog noise, PWM edges, I²C rise, power ripple at VREF.
- **DMM:** still the right tool for “is this 3.3 V.”
- **Compiler warnings:** `-Wall`; unused status returns are bugs.

`Error_Handler` and `assert_failed`: breakpoint in lab; in field, log `file:line`, reset reason (`RCC_CSR`), then controlled recovery.

---

# 20. Seven practical labs

Each lab has a **behavior** and an **acceptance test**. If you skip measurement, you did not finish the lab.

## Lab 1 — GPIO heartbeat

**Goal.** Prove reset, clock, GPIO, and `main` loop.

Steps: schematic LED pin; CubeMX PP output; toggle + `HAL_Delay(250)`.

**If fail:** pin, polarity, GPIOx clock, `Error_Handler`, board SB solder bridges.

**Accept:** visible blink after power-on without the debugger attached.

## Lab 2 — UART console

**Goal.** Bidirectional framed text with overflow handling.

TX banner at known baud. RX interrupt or DMA-idle. Commands: `help`, `led on`, unknown → error. Flood 100-byte line → overflow counter increments, system still responds.

**Accept:** valid and invalid input cannot wedge RX.

## Lab 3 — Timer timing

**Goal.** Calculation matches silicon.

Compute `f_tim` from RCC (include APB ×2 if applicable). Choose PSC/ARR for 1 kHz. Toggle GPIO in update ISR **or** better: PWM/OC on a pin.

**Measure** period with a scope. Tolerance: crystal ppm + your probe. 15% error means clock tree, not “scope calibration.”

**Accept:** measured frequency matches formula.

## Lab 4 — ADC polling

**Goal.** Codes track voltage.

Analog pin, long sample time. Print raw + computed volt. Apply 0 / mid / high (resistive divider, **current-limited**).

**Accept:** monotonic, not railing unexpectedly; DMM vs formula explained (offset OK).

## Lab 5 — ADC + DMA

**Goal.** Deterministic \(F_s\) and ownership.

Timer TRGO, circular DMA, HT/TC flags, overrun counter. GPIO pulse in HT callback: period must be \(N/(2F_s)\).

**Accept:** stable rate, DC test still correct, overrun = 0 at the chosen \(F_s\). Then raise \(F_s\) until overruns appear.

## Lab 6 — Sensor driver

**Goal.** API, not register soup in `main`.

WHOAMI, init, read, disconnect test (unplug → `TEMP_ERR_BUS`, no fake °C).

**Accept:** application file does not include the sensor register map.

## Lab 7 — FreeRTOS producer/consumer

**Goal.** Two tasks, queue, explicit full policy, stack watermarks.

Sensor task period with `vTaskDelayUntil`. Telemetry blocking receive. Fill the queue (slow telemetry on purpose) and **see** drops increment.

**Accept:** both tasks alive; error path observable; watermarks not zero.

## Capstone

Timer ADC DMA → convert units → ring/queue → UART telemetry with `seq`, `t_us`, `value`, `overruns`. Demonstrate a fault (unplug analog or stall the consumer) without silent corruption.

---

# 21. Engineering checklists and formulas

Print this page for the lab bench.

## Before enabling a peripheral

- [ ] Exact MCU + package + board revision
- [ ] Schematic net vs CubeMX pin vs AF table
- [ ] Clock source **measured or read from schematic**, not from memory
- [ ] Mode and timing from **that** device datasheet
- [ ] Voltage, GND, pulls, RESET
- [ ] NVIC + DMA request mapping
- [ ] Timeout and a visible failure

## Before DMA

- [ ] Init order: DMA MSP before start
- [ ] Direction, width, increment
- [ ] Circular vs normal
- [ ] Buffer lifetime, alignment, cache
- [ ] Deadline \(N/(2F_s)\) vs measured process time
- [ ] TE/overrun counters

## Before shipping a driver

- [ ] Units in names
- [ ] WHOAMI
- [ ] Every bus call has a timeout
- [ ] Endian and signedness
- [ ] Failure does not return stale data as OK
- [ ] No hidden globals
- [ ] Tests for NACK and bad ID

## Formula sheet

| Quantity | Rule | Caution |
|----------|------|---------|
| Timer update | \(f_{\text{tim}}/((\text{PSC}+1)(\text{ARR}+1))\) | Confirm \(f_{\text{tim}}\) vs PCLK |
| PWM duty (typical edge) | \(\text{CCR}/(\text{ARR}+1)\) | Mode 1/2, center-aligned |
| Sample period | \(1/F_s\) | Jitter if software-triggered |
| Half-buffer time | \(N/(2F_s)\) | Your ISR+process budget |
| Raw bandwidth | \(F_s \times\) bytes \(\times\) ch | Copies extra |
| ADC (common) | \(V = \text{code} \times V_{\text{ref}}/(2^N-1)\) | VREF error dominates |
| UART 8N1 payload | baud/10 | Not JSON throughput |
| UART bit time | \(1/\text{baud}\) | Measure on TX pin |
| I²C rise (approx) | \(0.8 R C\) | Compare to spec t_r |

## Hygiene

- `timeout_ms`, `rate_hz`, `temp_c` — units in identifiers.
- `uint16_t` for ADC codes, not `int`.
- Named constants for PSC, ARR, addresses.
- Overflow: `uint32_t` for `samples * bytes`.
- Diagnostics that do not `printf` 10 kHz.

---

# 22. Knowledge check with answers

Use orally or as a written quiz. Short answers follow each question; the chapters above are the full explanation.

**1. What does `.ioc` store?**  
CubeMX’s target MCU, pins, clocks, peripherals, middleware, and generation options. It is source, not firmware.

**2. Why can regeneration overwrite code?**  
CubeMX rewrites files and keeps only `USER CODE BEGIN/END` regions. Logic in `main.c` outside those blocks is deleted.

**3. CubeMX vs CubeIDE?**  
CubeMX configures and generates. CubeIDE edits, compiles, flashes, debugs. CubeMX also exists standalone.

**4. When is polling appropriate?**  
Init, low-rate, short bounded waits where blocking cannot miss a harder deadline.

**5. Interrupt vs DMA?**  
IRQ: CPU is notified. DMA: a bus master moves data. ADC+DMA still uses IRQs for HT/TC/errors.

**6. Why is `volatile` not enough for shared buffers?**  
No atomicity for multi-word objects, no exclusion, no DMA/cache coherency, no “this half is mine until I finish.”

**7. Why can \(f_{\text{tim}}\) ≠ SYSCLK?**  
AHB/APB prescalers, the ×2 timer rule, or a separate kernel clock mux.

**8. Why PSC+1 and ARR+1?**  
The hardware counts a divider of \(N+1\); 0 means “divide by 1.”

**9. CPOL vs CPHA?**  
Idle clock level vs which edge samples. Combined as SPI modes 0–3.

**10. Why I²C pull-ups?**  
Open-drain drivers cannot create a high; resistors do. Rise time depends on R and C.

**11. UART garbage — first causes?**  
Baud (clock tree), 8N1 vs 8E1, TX/RX swap, missing GND, RS-232 vs 3.3 V, HSI inaccuracy.

**12. Resolution vs accuracy?**  
4096 codes vs closeness to the true volt/temperature.

**13. Aliasing?**  
Energy above \(F_s/2\) appears as a fake lower frequency. Analog filter first.

**14. Circular DMA data loss?**  
Producer wraps onto a region still being read.

**15. Reusable driver?**  
Injected bus handle, units, timeouts, status codes, no app policy, tested failures.

**16. Superloop vs RTOS?**  
Superloop when work is bounded and coupled. RTOS when independent blocking deadlines and stacks are justified.

**17. Queue vs semaphore vs mutex?**  
Queue: data. Semaphore/notify: event. Mutex: shared resource, not from ISR.

**18. Short ISR?**  
Added latency to every lower-or-equal priority handler; jitter on control and UART ORE.

**19. I²C device missing?**  
Power, GND, pull-ups, 7-bit vs 8-bit address, AF pins, speed/rise, RESET, WHOAMI.

**20. Verify a timer rate?**  
Compute from **actual** timer clock; measure a pin. Do not trust `HAL_Delay` as a frequency reference.

---

# Final engineering mindset

You are not done when a number appears on a UART. You are done when you can answer:

| Question | Evidence |
|----------|----------|
| Configuration correct? | Datasheet + RM + CubeMX + generated MSP |
| Timing correct? | Formula **and** scope/logic analyzer |
| Data valid? | Known input, raw codes, units, error bounds |
| Keeps up? | Bandwidth math, WCET, overrun counters |
| Recovers? | Timeouts, visible states, bounded retries |
| Maintainable? | Small APIs, ownership comments, `.ioc` in git |

Adapt every pattern to **your** STM32 family. The compact guide (`STM32_Deep_Dive_Complete_Guide.md`) is the checklist. This file is the reasoning. Use both.
