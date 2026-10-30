# STM32 Complete Professional Reference

**Definitive Technical Textbook and Practical Training Manual**

*From STM32CubeIDE 2.2.0 and STM32CubeMX 6.18.1 configuration to deterministic real-time data acquisition: HAL, interrupts, DMA, timers, UART/SPI/I²C, ADC, reusable sensor drivers, digital filtering and FreeRTOS.*

## Document Information

**Table 0.1 — Document information and baseline**

| Attribute | Specification | Scope and notes |
| --- | --- | --- |
| Title | STM32 Complete Professional Reference | Consolidated university-level textbook and field manual |
| Version | 5.0 (consolidated master edition) | Supersedes the seven source documents listed in Appendix A |
| Toolchain baseline | STM32CubeIDE 2.2.0, STM32CubeMX 6.18.1, GNU Arm Embedded toolchain (`arm-none-eabi-gcc`) | Menu names may differ slightly between tool releases; see Appendix B |
| Primary hardware | STM32F103C8T6 (Cortex-M3, 72 MHz max) | All worked numbers use this part unless stated otherwise |
| Secondary hardware | STM32F4 (Cortex-M4F), notes for G0/G4/L4/H7/U5 | Differences are called out explicitly |
| Audience | Embedded and robotics students, instructors, junior firmware engineers | Self-study or a 90-minute, 2.5-hour or 6-hour workshop |
| Method | Problem → concept → decision → CubeMX → code → verify → debug | Every topic follows this loop (Section 1.3) |

> [!IMPORTANT]
> **Every listing is a pattern, not a drop-in.** STM32 is a family of families. Pin mappings, clock trees, DMA request routing, ADC trigger sources, HAL function availability and register names differ between parts. Before wiring hardware or trusting a calculation, open the datasheet and reference manual for your exact part number. Items that must be checked on your device are marked **Verify** and collected in Appendix B.

<!-- TOC -->

<!-- LISTS -->

# 1. Introduction, Learning Objectives and the System View

## 1.1 What an STM32 Really Is

An STM32 is a **system on chip (SoC)**, not just a CPU. Inside one package you find:

- an ARM Cortex-M processor core (with NVIC interrupt controller and SysTick timer),
- on-chip Flash (program and constants) and SRAM (variables, stacks, buffers),
- a **bus matrix** that connects bus masters (CPU, DMA) to memories and peripherals,
- a clock system (oscillators, PLL, prescalers) that feeds every block,
- peripherals: GPIO, timers, ADC/DAC, UART/USART, SPI, I²C, CAN, USB and more,
- a debug port (SWD/JTAG) used by ST-LINK or J-Link.

Firmware is the code that configures these blocks and moves data between them. Almost every embedded problem is one of three problems: **configuring hardware correctly**, **moving data without losing it**, or **meeting a deadline**.

**Table 1.1 — STM32 families and cores (selected)**

| Core | Example families | Characteristics |
| --- | --- | --- |
| Cortex-M0 / M0+ | STM32C0, G0, L0, F0 | Low cost, low power, small footprint |
| Cortex-M3 | STM32F1, F2, L1 | Deterministic, no FPU; F103 is the classic 72 MHz part |
| Cortex-M4F | STM32F3, F4, G4, L4 | Hardware FPU and DSP instructions |
| Cortex-M7 | STM32F7, H7 | L1 caches, dual-issue pipeline, up to hundreds of MHz |
| Cortex-M33 | STM32U5, H5, L5 | TrustZone security, FPU/DSP |

**Reading a part number** (example `STM32F103C8T6`): `F1` = family and core generation, `03` = line/feature set, `C` = 48 pins, `8` = 64 KB Flash, `T` = LQFP package, `6` = −40 to 85 °C. Always confirm the exact variant on the chip marking and in the datasheet; Flash and SRAM sizes drive linker scripts.

## 1.2 Learning Objectives

After studying this reference you should be able to:

1. **Use the toolchain:** create, configure, generate, build, flash and debug an STM32 project in CubeMX/CubeIDE, and keep your own code safe across regenerations.
2. **Explain HAL, LL and register access** and choose the right level for a task.
3. **Choose between polling, interrupts and DMA** using CPU load, latency and data-rate arguments, and implement each.
4. **Configure timers** (periodic events, PWM, input capture, output compare, ADC triggering) and compute PSC/ARR/CCR values.
5. **Implement and select UART, SPI and I²C**, including framing, modes, addressing, pull-up sizing and recovery.
6. **Acquire analog and digital sensor data in real time**: ADC equations, sampling time, buffers, filtering and data integrity.
7. **Write reusable sensor drivers** with clean APIs, error handling and hardware abstraction.
8. **Decide when an RTOS is justified** and use FreeRTOS tasks, queues, semaphores and mutexes correctly alongside interrupts.
9. **Debug systematically** from power and clocks up to application policy, including HardFault analysis.

## 1.3 How to Use This Reference

### 1.3.1 The seven-step engineering loop

The central teaching idea of the session is that learners need **technical judgment**, not a vocabulary list. A representative student remark (Arabic, from the session overview) captures it: the goal is not to explain DMA and ADC from scratch, but to know *when to think about DMA and when plain ADC reads are enough*. Every topic therefore follows the same loop:

1. **Define the hardware problem**: what physical signal exists, what must firmware produce, how often, and what happens on failure.
2. **Understand the mechanism**: why the hardware block exists and which bottleneck it removes.
3. **Apply decision criteria**: polling vs. interrupt vs. DMA, UART vs. SPI vs. I²C, superloop vs. RTOS.
4. **Consult the datasheet and reference manual**, then configure in CubeMX.
5. **Implement** modular C that checks every status, respects buffer ownership and never blocks critical paths.
6. **Verify with instruments**: oscilloscope, logic analyzer, multimeter or a diagnostic GPIO. A calculation without a measurement is a hypothesis.
7. **Anticipate failure modes**: configuration traps, electrical faults, race conditions, bus lockups.

### 1.3.2 Suggested delivery schedules

**Table 1.2 — Six-hour evening workshop plan**

| Time | Block | Minutes | Key topics |
| --- | --- | --- | --- |
| 18:00–18:30 | Arrival and toolchain check | 30 | CubeIDE, CubeMX, MCU packages, ST-LINK drivers, hardware |
| 18:30–19:10 | Part 1: workflow and architecture | 40 | `.ioc`, clock tree, GPIO limits, code generation, HAL vs. LL vs. registers |
| 19:10–20:00 | Part 2: interrupts, DMA, timers | 50 | Polling vs. IRQ vs. DMA, NVIC, timer math, TRGO → ADC → DMA |
| 20:00–20:30 | Prayer and rest break | 30 | Adjust to local prayer times; informal Q&A |
| 20:30–21:10 | Part 3: UART, SPI, I²C | 40 | Framing, baud math, SPI modes, I²C pull-ups, protocol selection |
| 21:10–21:45 | Part 4: sensors, ADC, DSP | 35 | ADC equations, source impedance, MPU6050, throughput, filters |
| 21:45–22:20 | Part 5: reusable drivers | 35 | Layered architecture, dependency injection, status codes |
| 22:20–22:45 | Part 6: FreeRTOS | 25 | Superloop vs. RTOS, tasks, queues, semaphores, mutexes, ISR rules |
| 22:45–23:15 | Part 7: integrated capstone | 30 | Multi-sensor monitoring system |
| 23:15–00:00 | Debugging, labs, buffer | 45 | Live troubleshooting, logic analyzer, HardFault, Q&A |

**Table 1.3 — Condensed schedules**

| Module | 2.5-hour session | 90-minute session |
| --- | --- | --- |
| CubeIDE, CubeMX, `.ioc` | 15 min (workflow, USER CODE preservation) | 10 min (essential `.ioc`, clocks, codegen) |
| Timers, interrupts, DMA | 30 min (selection criteria, timer + ADC + DMA) | 25 min (polling vs. IRQ vs. DMA, timer math) |
| Communication protocols | 20 min (trade-offs, scenarios) | 15 min (decision matrix, pitfalls) |
| Sensors and acquisition | 25 min (ADC math, sampling, buffering) | 15 min (scaling, source impedance, ping-pong) |
| Reusable drivers | 25 min (architecture, C pattern) | 15 min (header/source API) |
| DSP and FreeRTOS | 25 min (10 min filters + 15 min RTOS) | 10 min (overview) |
| Integrated system and Q&A | 10 min | Included in wrap-up |

> [!NOTE]
> **Instructor rule.** Do not spend the session explaining every feature of every peripheral. Spend it explaining how to choose the right feature for a real engineering problem. End by giving the class a realistic requirement and asking them to design the STM32 architecture and justify each choice.

## 1.4 The System as a Ten-Layer Chain

Firmware does not “talk to a sensor” in one step. It configures analog and digital silicon so that a physical quantity becomes a conditioned electrical signal, then a code in a peripheral register, then a word in SRAM, then a validated value in SI units, and finally an actuation command or telemetry packet. **If you cannot sketch this chain on paper, you cannot debug it systematically.**

![Figure 1.1 — Ten-layer signal and ownership chain, and STM32 internal bus matrix](diagrams/fig1_system_architecture.png)

*Figure 1.1 — (A) Ten-layer physical-to-application chain; (B) STM32 internal multi-layer AHB bus matrix and clock distribution.*

**Table 1.4 — The ten layers, their questions and typical failures**

| Layer | Core engineering question | Failure if ignored | Concrete STM32 example |
| --- | --- | --- | --- |
| 1. Physical world | What quantity, range and bandwidth? | Sensor saturates; vibration aliases | NTC over 0–100 °C; 1 kHz motor-current ripple |
| 2. Electrical network | What impedance, voltage range, filtering, grounding? | ADC sample capacitor never settles; 5 V damages a 3.3 V pin | 100 kΩ divider without buffer; missing I²C pull-ups |
| 3. Pin and pad mux | Which pin, mode and alternate function? | Schmitt trigger loads an analog signal; peripheral not connected | ADC pin left in digital input mode instead of `GPIO_MODE_ANALOG` |
| 4. Peripheral engine | Which block converts or shifts the signal, at what clock? | Wrong baud, SPI phase or ADC clock | ADC channel 5 configured, sensor wired to channel 4 |
| 5. Data movement | Polling, ISR copy or DMA? | CPU misses deadlines; DMA writes the wrong width | 32-bit DMA words into a 16-bit array corrupt adjacent RAM |
| 6. Event notification | When is data valid and how is software told? | Callback never fires because the IRQ is disabled | DMA half-transfer (HT) and transfer-complete (TC) interrupts |
| 7. Buffer ownership | Which context owns the region until when? | Torn reads; circular DMA overwrites live data | Main loop reads `adc_buf[]` while DMA wraps |
| 8. Driver and DSP | How are raw codes validated, scaled, filtered? | Stale data after a NACK; raw 2048 treated as 20.48 °C | `WHO_AM_I` check, two’s-complement decode, EMA filter |
| 9. Application policy | What states, thresholds and recovery? | System hangs when a sensor is unplugged | Hysteresis alarm with bounded retries |
| 10. Output / telemetry | How is the result delivered; can the link keep up? | Blocking UART print in a 10 kHz ISR destroys timing | Framed CSV (`seq,t_ms,temp_c,flags`) via non-blocking DMA |

> [!WARNING]
> **Data ownership is a first-class design item.** Ownership defines which execution context (main loop, ISR, DMA controller, RTOS task) may read or write a buffer at any instant. A `volatile` ready flag is **not** a lock: it neither freezes a circular DMA controller nor makes multi-byte reads atomic. Use ping-pong buffers, single-writer/single-reader ring buffers, or RTOS queues and mutexes.

## 1.5 Conventions Used in This Reference

- **Reference part:** STM32F103C8T6 at 72 MHz (HSE 8 MHz crystal, PLL ×9). Numbers for other families are labeled.
- **Code style:** C99, STM32 HAL naming, `static` for file-local state, `volatile` only for ISR-shared flags, explicit return-status checks. Generated CubeMX names (`hadc1`, `htim3`, `huart2`, `hi2c1`, `hspi1`) are assumed; adapt them to your project.
- **Callouts:** *Key concept*, *Engineering note*, *Practical tip*, *Warning*, *Common mistake / danger*.
- **Units:** frequencies in Hz, times in seconds (µs, ms, ns), voltages in volts, data rates in bytes per second unless stated.
- **Verification policy:** statements about silicon that differ across families are flagged, never silently generalized.
