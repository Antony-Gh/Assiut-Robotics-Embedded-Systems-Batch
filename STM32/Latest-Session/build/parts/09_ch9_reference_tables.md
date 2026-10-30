# 9. Master Reference and Comparison Tables

This chapter compiles comprehensive reference tables summarizing STM32 hardware architecture, peripheral trade-offs, and communication buses.

---

## 9.1 STM32 Microcontroller Families At a Glance

**Table 9.1 — Major STM32 microcontroller series comparison**

| Family | ARM Core | Max Frequency | L1 Cache | DMA Controller | ADC Max Speed | Target Application |
|---|---|---|---|---|---|---|
| **STM32F1** (F103) | Cortex-M3 | 72 MHz | None | Channel-based (DMA1/2) | 1.0 MSPS (12-bit) | Cost-sensitive general purpose, educational robotics |
| **STM32F4** (F401/F407/F446) | Cortex-M4F | 84–180 MHz | ART Accelerator | Stream/FIFO-based (DMA1/2) | 2.4 MSPS (12-bit) | Flight controllers, audio DSP, advanced robotics |
| **STM32G4** (G474) | Cortex-M4F | 170 MHz | ART Accelerator | DMAMUX + Flexible Streams | 4.0 MSPS (12-bit) + HRTIM | Digital power supply, drone ESCs, motor FOC |
| **STM32L4** (L476) | Cortex-M4F | 80 MHz | ART Accelerator | Channel-based with DMAMUX | 5.0 MSPS (12-bit) | Ultra-low-power IoT, wearable medical devices |
| **STM32H7** (H743/H753) | Cortex-M7 (+ M4) | 480–550 MHz | 32 KB I-Cache + 32 KB D-Cache | MDMA + BDMA + DMA1/2 | 3.6 MSPS (16-bit) | Computer vision, AI edge processing, industrial gateways |

---

## 9.2 Comprehensive Communication Protocol Matrix

**Table 9.2 — Master comparison of serial, industrial, and networking protocols**

| Protocol | Physical Topology | Duplex | Max Data Rate | Wires (+ GND) | Max Distance | Addressing | Primary Advantages |
|---|---|---|---|---|---|---|---|
| **UART / USART** | Point-to-Point | Full | Up to 10–27 Mbps | 2 | 2–3 m (TTL) | None (Framed) | Ubiquitous, simple, PC Virtual COM port support. |
| **SPI** | Single Master / Multi-Slave | Full | Up to 40–80 Mbps | 3 + N (CS) | < 30 cm | Hardware CS pin | Extremely high speed, zero protocol overhead. |
| **I²C** | Multi-Master / Multi-Slave | Half | 100k / 400k / 1 Mbps | 2 | < 1 m | 7-bit / 10-bit | Only 2 wires for dozens of sensors; built-in flow control. |
| **CAN / CAN-FD** | Multi-Drop Differential | Half | 1 Mbps (FD: 5 Mbps) | 2 | Up to 1000 m | 11-bit / 29-bit ID | Non-destructive arbitration, hardware CRC, extreme EMI immunity. |
| **RS-485** | Multi-Drop Differential | Half | Up to 10–20 Mbps | 2 | Up to 1200 m | Application level | Robust long-range industrial Modbus networks. |
| **USB 2.0 (FS/HS)** | Tiered Star | Half | 12 Mbps / 480 Mbps | 2 (Diff D+/D-) | 5 m | Dynamic Enumeration | High-speed PC connectivity, plug-and-play, power delivery. |
| **Ethernet** | Star / Switch | Full | 100 Mbps / 1 Gbps | 4 / 8 | 100 m | MAC / IP Address | Global network routability, massive bandwidth. |

---

## 9.3 Execution and Data Transfer Paradigms

**Table 9.3 — Trade-offs between Polling, Interrupts, DMA, and RTOS**

| Paradigm | CPU Utilization | Software Complexity | Maximum Throughput | Latency & Jitter | Best Engineering Application |
|---|---|---|---|---|---|
| **Polling** | 100% blocked | Minimal | High (single task) | Extreme jitter | Boot initialization, HardFault crash reporting. |
| **Interrupts** | Medium (context switch overhead) | Moderate | Medium (< 100 kSPS) | Low latency, low jitter | Infrequent asynchronous events (Buttons, Fault pins). |
| **DMA** | **Near 0%** | Moderate | **Maximum silicon bandwidth** | **Zero jitter** | High-speed ADC, audio streaming, display SPI, UART logs. |
| **RTOS Multitasking** | Low (~1–3% scheduler load) | High | Balanced across threads | Deterministic priority-based | Complex systems with multiple asynchronous deadlines. |

---

# 10. Conclusion and Recommended Learning Path

Mastering STM32 embedded systems requires progressing through five distinct levels of engineering maturity.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │ Level 5: High-Performance Architecture & Optimization       │
 │ Cache maintenance, DSP pipelines, HRTIM motor FOC, Dual-core│
 ├─────────────────────────────────────────────────────────────┤
 │ Level 4: Real-Time Systems & Middleware                     │
 │ FreeRTOS multitasking, Mutexes, Queues, USB VCP, LwIP TCP/IP│
 ├─────────────────────────────────────────────────────────────┤
 │ Level 3: Modular Drivers & Hardware Abstraction             │
 │ 4-layer drivers, Function pointers, HIL unit testing on PC  │
 ├─────────────────────────────────────────────────────────────┤
 │ Level 2: Advanced Peripheral Interfacing                    │
 │ Timer TRGO, Circular DMA, Multi-channel ADC, SPI, I2C unjam │
 ├─────────────────────────────────────────────────────────────┤
 │ Level 1: Core Fundamentals & Workflow                       │
 │ CubeMX, Clock tree PLL, GPIO, Basic UART printf, HAL vs Reg │
 └─────────────────────────────────────────────────────────────┘
```

### The Engineering Mindset: Observe and Measure
A professional embedded systems engineer does not guess or assume. When a peripheral behaves unexpectedly:
1. **Probe the physical silicon**: Connect an oscilloscope to observe rise times, ringing, ground bounce, and clock frequencies.
2. **Read the registers**: Inspect peripheral SFRs in the debugger to verify that configuration bits match the datasheet.
3. **Verify mathematics**: Calculate prescalers, auto-reload values, baud rate dividers, and filter coefficients on paper before writing firmware.
4. **Isolate layers**: Test device drivers independently with mock functions on a host PC before integrating them into a robotic chassis.

---

# 11. Comprehensive Technical Glossary

This glossary defines essential technical terminology, including all 30 foundational STM32CubeMX concepts.

- **ADC (Analog-to-Digital Converter)**: An internal peripheral that quantizes continuous physical voltages into discrete digital integer codes (e.g., 12-bit SAR ADC yielding 0–4095).
- **AHB (Advanced High-performance Bus)**: The primary high-speed bus connecting the Cortex-M core, DMA controllers, and high-bandwidth memory (Flash, SRAM).
- **APB (Advanced Peripheral Bus)**: Peripheral buses (APB1 and APB2) bridged to the AHB, clocking timers, UART, I²C, and SPI peripherals.
- **ARR (Auto-Reload Register)**: The 16-bit or 32-bit timer register defining the maximum counter value before rollover and update event generation.
- **BFSR (Bus Fault Status Register)**: Hardware status register recording bus errors, invalid memory accesses, and whether `BFAR` holds a valid address.
- **BRR (Baud Rate Register)**: Peripheral register in USART containing the mantissa and fraction clock divider used to establish serial baud rates.
- **BSP (Board Support Package)**: A collection of software drivers tailored to a specific development board (e.g., Nucleo-F401RE, Discovery), managing onboard LEDs, buttons, and external sensors.
- **CFSR (Configurable Fault Status Register)**: Cortex-M 32-bit register combining MemManage, Bus Fault, and Usage Fault status flags.
- **Circular DMA**: An operating mode where the DMA controller automatically wraps its memory pointer back to the beginning upon reaching the buffer end, continuing transfers indefinitely.
- **CMSIS (Cortex Microcontroller Software Interface Standard)**: ARM's vendor-independent hardware abstraction layer providing standardized register definitions, core intrinsics, and DSP/RTOS APIs.
- **CPHA (Clock Phase)**: SPI configuration bit specifying whether data is sampled on the first or second clock transition.
- **CPOL (Clock Polarity)**: SPI configuration bit defining the idle electrical logic level (LOW or HIGH) of the SCLK line.
- **D-Cache (Data Cache)**: High-speed L1 cache on ARM Cortex-M7 cores accelerating memory reads and writes; requires explicit invalidation and cleaning during DMA operations.
- **DMA (Direct Memory Access)**: A specialized hardware bus master that transfers data directly between peripherals and memory without CPU intervention.
- **ENOB (Effective Number of Bits)**: A true measure of ADC dynamic range and accuracy accounting for thermal noise, quantization distortion, and non-linearities.
- **EXTI (Extended Interrupt and Event Controller)**: Hardware peripheral managing external pin edge detection and routing interrupt requests to the NVIC.
- **Flash Memory**: Non-volatile, electrically erasable on-chip storage where compiled executable firmware instructions and constant data reside.
- **FreeRTOS**: A market-leading open-source real-time operating system kernel providing preemptive scheduling, tasks, queues, semaphores, and mutexes.
- **GPIO (General-Purpose Input/Output)**: Software-configurable pins capable of operating as digital inputs, outputs, analog pins, or peripheral alternate functions.
- **HAL (Hardware Abstraction Layer)**: STMicroelectronics' high-level, portable C library providing standardized API calls and handle structures across all STM32 families.
- **HSE (High-Speed External)**: An accurate, temperature-stable external clock source provided by a quartz crystal or ceramic resonator (typically 4–26 MHz).
- **HSI (High-Speed Internal)**: An on-chip RC oscillator providing an immediate clock upon power-up (e.g., 8 MHz or 16 MHz); subject to thermal drift ($\pm 1\%$ to $\pm 3\%$).
- **I²C (Inter-Integrated Circuit)**: A synchronous, multi-master, open-drain two-wire serial bus (SDA, SCL) using pull-up resistors and 7-bit/10-bit software addressing.
- **IDE (Integrated Development Environment)**: Software suite combining text editing, compilation, linking, flashing, and source-level debugging (e.g., STM32CubeIDE).
- **INL / DNL**: Integral Non-Linearity and Differential Non-Linearity; metrics measuring the deviation of an ADC quantization transfer function from an ideal straight line.
- **IOC File**: Plain-text XML-like project configuration file used by STM32CubeMX to store hardware configuration settings and synthesize C initialization code.
- **ISR (Interrupt Service Routine)**: A dedicated function invoked automatically by the hardware NVIC when a peripheral or external interrupt event occurs.
- **IWDG (Independent Watchdog)**: A failsafe hardware countdown timer clocked by an independent low-speed oscillator (LSI) that resets the MCU if firmware locks up.
- **LL (Low-Layer Drivers)**: ST's lightweight, non-blocking, register-oriented C library providing direct, inline hardware access with zero abstraction overhead.
- **LSE (Low-Speed External)**: An accurate external 32.768 kHz crystal oscillator used exclusively to drive the Real-Time Clock (RTC).
- **LSI (Low-Speed Internal)**: An internal low-power ~32–40 kHz RC oscillator used to clock the Independent Watchdog (IWDG) and low-power wake-up timers.
- **MCU (Microcontroller Unit)**: A highly integrated single-chip computer combining a CPU core, volatile RAM, non-volatile Flash, and diverse hardware peripherals.
- **MPU (Memory Protection Unit)**: Hardware unit on ARM cores that monitors memory transactions and enforces access permissions, isolating OS kernels and disabling cache on DMA regions.
- **Mutex (Mutual Exclusion)**: An RTOS synchronization primitive featuring priority inheritance used to serialize access to shared hardware resources across tasks.
- **NVIC (Nested Vectored Interrupt Controller)**: The tightly coupled ARM Cortex-M interrupt controller providing low-latency, nested priority interrupt handling.
- **Open-Drain**: An output pin stage featuring an NMOS transistor that can actively pull a line LOW to ground but floats when driving HIGH, requiring an external pull-up resistor.
- **PendSV (Pended Servicing Call)**: A low-priority exception on Cortex-M processors dedicated to executing RTOS context switches when no other interrupts are active.
- **PLL (Phase-Locked Loop)**: An on-chip high-frequency clock synthesizer that multiplies lower-frequency reference clocks (HSE or HSI) up to maximum core operating limits (e.g. 72–480 MHz).
- **Priority Inheritance**: An algorithmic protocol implemented in RTOS mutexes that temporarily elevates the priority of a low-priority task holding a lock to prevent priority inversion.
- **Push-Pull**: An active output pin stage featuring complementary PMOS and NMOS transistors capable of driving strong logic HIGH and strong logic LOW.
- **PWM (Pulse Width Modulation)**: A digital square-wave signal generated by a timer with a fixed frequency and variable duty cycle, used for motor speed control and LED dimming.
- **Queue**: A thread-safe FIFO data structure in FreeRTOS that passes discrete data packets by value between tasks or from ISRs to tasks.
- **RCC (Reset and Clock Control)**: The foundational peripheral controlling reset generation, oscillator selection, PLL configuration, and peripheral clock gating.
- **RTC (Real-Time Clock)**: An ultra-low-power autonomous hardware calendar counter that tracks seconds, minutes, hours, days, months, and years.
- **RTOS (Real-Time Operating System)**: Software providing deterministic multitasking scheduling, thread synchronization, and resource management.
- **SAR (Successive Approximation Register)**: A high-speed, high-resolution ADC architecture that determines digital codes through binary search comparisons.
- **SPI (Serial Peripheral Interface)**: A synchronous, full-duplex, four-wire master-slave serial bus capable of multi-megahertz data transfers.
- **SysTick (System Tick Timer)**: A 24-bit down-counter timer built into the ARM Cortex-M core, used to generate periodic 1 ms tick interrupts for OS scheduling and `HAL_Delay()`.
- **TCB (Task Control Block)**: A FreeRTOS data structure storing all context, stack boundaries, priority, and state flags for an active task.
- **TIM (Timer)**: A versatile peripheral comprising a prescaler, counter, and capture/compare registers capable of generating PWM, measuring pulse widths, and triggering DMA.
- **TRGO (Trigger Output)**: An internal hardware signal generated by a timer upon counter update, compare match, or reset, used to trigger ADC conversions with zero CPU jitter.
- **UART / USART**: Universal Asynchronous/Synchronous Receiver Transmitter; a serial communication interface supporting framed byte transfers over TX and RX lines.
- **VDD / VREF**: $V_{DD}$ is the primary digital operating voltage supply rail (typically 3.3 V); $V_{\text{REF}}$ is the analog reference voltage governing ADC quantization full-scale limits.
- **WWDG (Window Watchdog)**: A safety watchdog timer clocked from the APB bus that resets the system if refreshed too late OR too early, catching runaway execution loops.

---

# 12. References and Official Documentation

This technical reference is built upon verified silicon specifications and official technical documentation.

### 12.1 STMicroelectronics Official Documentation
1. **STM32F103x8/B Datasheet**: *Medium-density performance line ARM-based 32-bit MCU with 64 or 128 KB Flash, USB, CAN, 7 timers, 2 ADCs, 9 com. interfaces*. Document ID: DS5319.
2. **RM0008 Reference Manual**: *STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx advanced ARM-based 32-bit MCUs*. STMicroelectronics.
3. **RM0090 Reference Manual**: *STM32F405/415, STM32F407/417, STM32F427/437 and STM32F429/439 advanced ARM-based 32-bit MCUs*. STMicroelectronics.
4. **UM1718 User Manual**: *STM32CubeMX for STM32 configuration and initialization C code generation*. STMicroelectronics.
5. **UM2609 User Manual**: *STM32CubeIDE user guide*. STMicroelectronics.
6. **UM1850 User Manual**: *Description of STM32F1 HAL and low-layer drivers*. STMicroelectronics.
7. **AN2586 Application Note**: *STM32F10xxx hardware development: getting started*. STMicroelectronics.
8. **AN2834 Application Note**: *How to get the best ADC accuracy in STM32 microcontrollers*. STMicroelectronics.
9. **AN4031 Application Note**: *Using the DMA controller on STM32F2, STM32F4 and STM32F7 series*. STMicroelectronics.

### 12.2 ARM Architecture Documentation
1. **ARM DDI 0403E.e**: *ARMv7-M Architecture Reference Manual*. ARM Limited.
2. **ARM DUI 0552A**: *Cortex-M3 Devices Generic User Guide*. ARM Limited.
3. **ARM DUI 0553A**: *Cortex-M4 Devices Generic User Guide*. ARM Limited.
4. **Yiu, Joseph**: *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors*. 3rd Edition, Newnes, 2013.

### 12.3 FreeRTOS Documentation
1. **Barry, Richard**: *Mastering the FreeRTOS Real Time Kernel — A Hands-On Tutorial Guide*. Real Time Engineers Ltd.
2. **FreeRTOS Kernel Reference Manual**: *API Functions and Configuration Options*. Official FreeRTOS Documentation (`https://www.freertos.org`).

### 12.4 Industry Standards and Sensor Specifications
1. **NXP Semiconductors UM10204**: *I2C-bus specification and user manual*. Rev. 7.0, 2021.
2. **InvenSense Inc.**: *MPU-6000 and MPU-6050 Product Specification*. Revision 3.4, Document Number: PS-MPU-6000A-00.
3. **InvenSense Inc.**: *MPU-6000 and MPU-6050 Register Map and Descriptions*. Revision 4.2, Document Number: RM-MPU-6000A-00.
